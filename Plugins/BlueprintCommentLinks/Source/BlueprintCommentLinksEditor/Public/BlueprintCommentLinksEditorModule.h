#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"

class FUICommandList;
class FToolBarBuilder;
class FExtender;
class IBlueprintEditor;
class SGraphEditor;
class SGraphPanel;
class SCommentLinkOverlay;
class FBlueprintCommentLinksGraphPanelNodeFactory;
class FBlueprintCommentLinkCommentDetails;

/**
 * Editor module that:
 * - Registers a Blueprint Editor toolbar toggle for "Comment Link Mode".
 * - Injects SCommentLinkOverlay above each SGraphPanel in Blueprint editors.
 */
class FBlueprintCommentLinksEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void ExtendBlueprintEditorToolbar();
    void AddToolbarExtension(FToolBarBuilder& Builder);

    /** Command handler: toggle global comment link mode. */
    void ToggleCommentLinkMode();
    bool IsCommentLinkModeActive() const;

    /** Attach an overlay to a given SGraphEditor / SGraphPanel. */
    void AttachOverlayToGraphEditor(const TSharedRef<IBlueprintEditor>& BlueprintEditor,
                                    const TSharedRef<SGraphEditor>& GraphEditor);

    /** Attach overlays to any open Blueprint editors. */
    void AttachOverlaysToOpenBlueprintEditors();

    /** Called after the engine has finished initializing (GEditor is valid). */
    void OnPostEngineInit();

    /** Register / unregister Blueprint editor toolbar and related extensions. */
    void RegisterBlueprintEditorExtensions();
    void UnregisterBlueprintEditorExtensions();

    /** Called whenever a Blueprint editor is opened so we can attach overlays. */
    void HandleBlueprintEditorOpened(EBlueprintType InBlueprintType);
    bool HandleBlueprintEditorOpenedDeferred(float DeltaTime);

private:
    TSharedPtr<FUICommandList> CommandList;
    TSharedPtr<FExtender> BlueprintToolbarExtender;

    bool bCommentLinkModeActive = false;

    // Engine lifecycle hook.
    FDelegateHandle PostEngineInitHandle;
    FDelegateHandle BlueprintEditorOpenedHandle;

    // We keep weak references so overlays can be GC'd when graphs close.
    TArray<TWeakPtr<SCommentLinkOverlay>> ActiveOverlays;

    // Graph panel node factory for custom comment nodes.
    TSharedPtr<FBlueprintCommentLinksGraphPanelNodeFactory> CommentNodeFactory;
};
