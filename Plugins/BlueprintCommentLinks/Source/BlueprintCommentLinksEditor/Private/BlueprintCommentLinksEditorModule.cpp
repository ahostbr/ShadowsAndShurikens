#include "BlueprintCommentLinksEditorModule.h"

#include "SCommentLinkOverlay.h"
#include "BlueprintCommentLinksCacheFile.h"
#include "BlueprintCommentLinksGraphPanelNodeFactory.h"
#include "BlueprintCommentLinkCommentDetails.h"

#include "BlueprintEditorModule.h"
#include "EdGraphUtilities.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Containers/Ticker.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SOverlay.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FBlueprintCommentLinksEditorModule"

//////////////////////////////////////////////////////////////////////////
// Command definition

class FBlueprintCommentLinksEditorCommands : public TCommands<FBlueprintCommentLinksEditorCommands>
{
public:
    FBlueprintCommentLinksEditorCommands()
        : TCommands<FBlueprintCommentLinksEditorCommands>(
              TEXT("BlueprintCommentLinksEditor"),
              LOCTEXT("BlueprintCommentLinksEditor", "Blueprint Comment Links"),
              NAME_None,
              FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override
    {
        UI_COMMAND(
            ToggleCommentLinkMode,
            "Comment Link Mode",
            "Toggle comment link creation mode.",
            EUserInterfaceActionType::ToggleButton,
            FInputChord());
    }

public:
    TSharedPtr<FUICommandInfo> ToggleCommentLinkMode;
};

//////////////////////////////////////////////////////////////////////////
// Module

void FBlueprintCommentLinksEditorModule::StartupModule()
{
    FBlueprintCommentLinksEditorCommands::Register();

    CommandList = MakeShared<FUICommandList>();
    const FBlueprintCommentLinksEditorCommands& Commands = FBlueprintCommentLinksEditorCommands::Get();

    CommandList->MapAction(
        Commands.ToggleCommentLinkMode,
        FExecuteAction::CreateRaw(this, &FBlueprintCommentLinksEditorModule::ToggleCommentLinkMode),
        FCanExecuteAction::CreateLambda([]() { return true; }),
        FIsActionChecked::CreateRaw(this, &FBlueprintCommentLinksEditorModule::IsCommentLinkModeActive));

    // Ensure the global comment link cache is created and listening for events.
    UBlueprintCommentLinkCacheFile::Get();

    // Register the graph panel node factory for comment nodes.
    CommentNodeFactory = MakeShared<FBlueprintCommentLinksGraphPanelNodeFactory>();
    FEdGraphUtilities::RegisterVisualNodeFactory(CommentNodeFactory);

    // Register details customization for comment nodes.
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.RegisterCustomClassLayout(
            "EdGraphNode_Comment",
            FOnGetDetailCustomizationInstance::CreateStatic(
                &FBlueprintCommentLinkCommentDetails::MakeInstance));
    }

    // Defer Blueprint editor toolbar and overlay setup until the engine is fully initialized.
    if (!IsRunningCommandlet())
    {
        PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
            this,
            &FBlueprintCommentLinksEditorModule::OnPostEngineInit);
    }

    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: StartupModule complete"));
}

void FBlueprintCommentLinksEditorModule::ShutdownModule()
{
    if (PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
        PostEngineInitHandle.Reset();
    }

    UnregisterBlueprintEditorExtensions();

    ActiveOverlays.Empty();

    if (CommentNodeFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualNodeFactory(CommentNodeFactory);
        CommentNodeFactory.Reset();
    }

    // Unregister details customization.
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout("EdGraphNode_Comment");
    }

    UBlueprintCommentLinkCacheFile::TearDown();

    FBlueprintCommentLinksEditorCommands::Unregister();
}

void FBlueprintCommentLinksEditorModule::ExtendBlueprintEditorToolbar()
{
    FBlueprintEditorModule& BPEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

    if (!BlueprintToolbarExtender.IsValid())
    {
        BlueprintToolbarExtender = MakeShared<FExtender>();
        BlueprintToolbarExtender->AddToolBarExtension(
            "Settings",
            EExtensionHook::After,
            CommandList,
            FToolBarExtensionDelegate::CreateRaw(this, &FBlueprintCommentLinksEditorModule::AddToolbarExtension));

        BPEditorModule.GetMenuExtensibilityManager()->AddExtender(BlueprintToolbarExtender);
    }
}

void FBlueprintCommentLinksEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    const FBlueprintCommentLinksEditorCommands& Commands = FBlueprintCommentLinksEditorCommands::Get();

    Builder.AddToolBarButton(
        Commands.ToggleCommentLinkMode,
        NAME_None,
        LOCTEXT("CommentLinkMode_Label", "Comment Links"),
        LOCTEXT("CommentLinkMode_Tooltip", "Toggle Comment Link creation mode."),
        // Use the Blueprint "Class Defaults" icon for familiarity.
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "FullBlueprintEditor.EditClassDefaults"));
}

void FBlueprintCommentLinksEditorModule::ToggleCommentLinkMode()
{
    bCommentLinkModeActive = !bCommentLinkModeActive;

    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: ToggleCommentLinkMode -> %s"),
        bCommentLinkModeActive ? TEXT("ON") : TEXT("OFF"));

    // Ensure overlays exist for any open editors.
    AttachOverlaysToOpenBlueprintEditors();

    // Update existing overlays.
    for (TWeakPtr<SCommentLinkOverlay>& OverlayWeak : ActiveOverlays)
    {
        if (TSharedPtr<SCommentLinkOverlay> Overlay = OverlayWeak.Pin())
        {
            Overlay->SetLinkModeActive(bCommentLinkModeActive);
        }
    }
}

bool FBlueprintCommentLinksEditorModule::IsCommentLinkModeActive() const
{
    return bCommentLinkModeActive;
}

void FBlueprintCommentLinksEditorModule::AttachOverlayToGraphEditor(
    const TSharedRef<IBlueprintEditor>& BlueprintEditor,
    const TSharedRef<SGraphEditor>& GraphEditor)
{
    SGraphPanel* GraphPanel = GraphEditor->GetGraphPanel();
    if (!GraphPanel)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: AttachOverlayToGraphEditor -> GraphPanel is null"));
        return;
    }

    // Avoid adding duplicate overlays for the same panel.
    for (const TWeakPtr<SCommentLinkOverlay>& ExistingWeak : ActiveOverlays)
    {
        if (TSharedPtr<SCommentLinkOverlay> Existing = ExistingWeak.Pin())
        {
            if (Existing->IsAttachedTo(GraphPanel))
            {
                Existing->SetLinkModeActive(bCommentLinkModeActive);
                UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: Reusing existing overlay for panel %p"), GraphPanel);
                return;
            }
        }
    }

    // Derive the blueprint from the focused graph's outer Blueprint.
    UEdGraph* FocusedGraph = BlueprintEditor->GetFocusedGraph();
    UBlueprint* Blueprint = FocusedGraph
        ? Cast<UBlueprint>(FocusedGraph->GetTypedOuter<UBlueprint>())
        : nullptr;
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: No blueprint found for graph editor"));
        return;
    }

    // Create the overlay widget bound to this graph panel + blueprint.
    TSharedRef<SCommentLinkOverlay> OverlayWidget =
        SNew(SCommentLinkOverlay)
        .GraphPanel(GraphPanel)
        .Blueprint(Blueprint);

    OverlayWidget->RefreshCachedLinks();
    OverlayWidget->SetLinkModeActive(bCommentLinkModeActive);

    // Attach overlay above the graph panel by finding a parent SOverlay.
    {
        TSharedPtr<SOverlay> OverlayContainer;
        TSharedPtr<SWidget> ParentWidget = GraphPanel->AsShared();

        // Walk up the widget hierarchy until we find an actual SOverlay.
        while (ParentWidget.IsValid())
        {
            // Compare Slate widget type names using the built-in type info.
            if (ParentWidget->GetType() == SOverlay::StaticWidgetClass().GetWidgetType())
            {
                OverlayContainer = StaticCastSharedPtr<SOverlay>(ParentWidget);
                break;
            }

            ParentWidget = ParentWidget->GetParentWidget();
        }

        if (OverlayContainer.IsValid())
        {
            OverlayContainer->AddSlot()[OverlayWidget];
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintCommentLinks: Could not find parent SOverlay for graph panel %p"), GraphPanel);
        }
    }

    ActiveOverlays.Add(OverlayWidget);

    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: Added overlay %p to graph panel %p for blueprint %s"),
        &OverlayWidget.Get(),
        GraphPanel,
        *GetNameSafe(Blueprint));
}

void FBlueprintCommentLinksEditorModule::AttachOverlaysToOpenBlueprintEditors()
{
    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: AttachOverlaysToOpenBlueprintEditors"));

    FBlueprintEditorModule& BPEditorModule =
        FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

    TArray<TSharedRef<IBlueprintEditor>> Editors = BPEditorModule.GetBlueprintEditors();
    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: Found %d blueprint editors"), Editors.Num());

    for (TSharedRef<IBlueprintEditor> EditorRef : Editors)
    {
        UEdGraph* FocusedGraph = EditorRef->GetFocusedGraph();
        if (!FocusedGraph)
        {
            continue;
        }

        TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(FocusedGraph);
        if (!GraphEditor.IsValid())
        {
            continue;
        }

        AttachOverlayToGraphEditor(EditorRef, GraphEditor.ToSharedRef());
    }
}

void FBlueprintCommentLinksEditorModule::OnPostEngineInit()
{
    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnPostEngineInit"));

    if (!GIsEditor || GEditor == nullptr)
    {
        return;
    }

    RegisterBlueprintEditorExtensions();
    AttachOverlaysToOpenBlueprintEditors();
}

void FBlueprintCommentLinksEditorModule::RegisterBlueprintEditorExtensions()
{
    if (!GIsEditor)
    {
        return;
    }

    FBlueprintEditorModule& BPEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

    ExtendBlueprintEditorToolbar();

    if (!BlueprintEditorOpenedHandle.IsValid())
    {
        BlueprintEditorOpenedHandle =
            BPEditorModule.OnBlueprintEditorOpened().AddRaw(
                this,
                &FBlueprintCommentLinksEditorModule::HandleBlueprintEditorOpened);
    }
}

void FBlueprintCommentLinksEditorModule::UnregisterBlueprintEditorExtensions()
{
    if (FModuleManager::Get().IsModuleLoaded("Kismet"))
    {
        FBlueprintEditorModule& BPEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");

        if (BlueprintToolbarExtender.IsValid())
        {
            BPEditorModule.GetMenuExtensibilityManager()->RemoveExtender(BlueprintToolbarExtender);
            BlueprintToolbarExtender.Reset();
        }

        if (BlueprintEditorOpenedHandle.IsValid())
        {
            BPEditorModule.OnBlueprintEditorOpened().Remove(BlueprintEditorOpenedHandle);
            BlueprintEditorOpenedHandle.Reset();
        }
    }
}

void FBlueprintCommentLinksEditorModule::HandleBlueprintEditorOpened(EBlueprintType InBlueprintType)
{
    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: HandleBlueprintEditorOpened for type %d"),
        static_cast<int32>(InBlueprintType));

    // The event is fired before the new editor is added to the internal list.
    // Defer attaching overlays until the next tick so GetBlueprintEditors()
    // can see the freshly created editor.
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FBlueprintCommentLinksEditorModule::HandleBlueprintEditorOpenedDeferred),
        0.0f);
}

bool FBlueprintCommentLinksEditorModule::HandleBlueprintEditorOpenedDeferred(float DeltaTime)
{
    AttachOverlaysToOpenBlueprintEditors();

    // Ensure all overlays reflect the current cache and global mode.
    for (TWeakPtr<SCommentLinkOverlay>& OverlayWeak : ActiveOverlays)
    {
        if (TSharedPtr<SCommentLinkOverlay> Overlay = OverlayWeak.Pin())
        {
            Overlay->RefreshCachedLinks();
            Overlay->SetLinkModeActive(bCommentLinkModeActive);
        }
    }

    // One-shot ticker.
    return false;
}

IMPLEMENT_MODULE(FBlueprintCommentLinksEditorModule, BlueprintCommentLinksEditor)

#undef LOCTEXT_NAMESPACE
