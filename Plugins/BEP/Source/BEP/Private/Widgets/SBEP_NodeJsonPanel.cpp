#include "SBEP_NodeJsonPanel.h"

#include "BEPExportSettings.h"
#include "BEP_NodeJsonExport.h"
#include "BEP_NodeJsonExportSettings.h"
#include "BEP_NodeJsonUI.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "IDetailsView.h"
#include "EdGraphNode_Comment.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "SGraphPanel.h"
#include "Widgets/SWindow.h"
#include "Misc/Paths.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "UObject/UnrealType.h"
#include "Styling/CoreStyle.h"
#include "Internationalization/Text.h"

namespace
{
struct FParsedCsvRow
{
    FString GuidStr;
    FString Comment;
};

static FString PresetToString(const EBEP_NodeJsonPreset Preset)
{
    if (const UEnum* Enum = StaticEnum<EBEP_NodeJsonPreset>())
    {
        return Enum->GetNameStringByValue(static_cast<int64>(Preset));
    }
    return TEXT("Unknown");
}

static void GatherGraphPanels(const TSharedRef<SWidget>& Root, TArray<TSharedPtr<SGraphPanel>>& Out)
{
    if (Root->GetType() == FName(TEXT("SGraphPanel")))
    {
        Out.Add(StaticCastSharedRef<SGraphPanel>(Root));
    }
    if (FChildren* Children = Root->GetChildren())
    {
        const int32 Num = Children->Num();
        for (int32 Index = 0; Index < Num; ++Index)
        {
            GatherGraphPanels(Children->GetChildAt(Index), Out);
        }
    }
}

static TSharedPtr<SGraphPanel> FindActiveGraphPanel()
{
    const TArray<TSharedRef<SWindow>>& Windows = FSlateApplication::Get().GetTopLevelWindows();
    TArray<TSharedPtr<SGraphPanel>> Panels;
    for (const TSharedRef<SWindow>& Win : Windows)
    {
        if (TSharedPtr<SWidget> Content = Win->GetContent())
        {
            GatherGraphPanels(Content.ToSharedRef(), Panels);
        }
    }

    for (const TSharedPtr<SGraphPanel>& Panel : Panels)
    {
        if (Panel.IsValid())
        {
            if (UEdGraph* Graph = Panel->GetGraphObj())
            {
                if (Panel->GetSelectedGraphNodes().Num() > 0)
                {
                    return Panel;
                }
            }
        }
    }

    for (const TSharedPtr<SGraphPanel>& Panel : Panels)
    {
        if (Panel.IsValid() && Panel->GetGraphObj())
        {
            return Panel;
        }
    }

    return nullptr;
}

static TArray<FString> TokenizeCsvRow(const FString& Line)
{
    TArray<FString> Cells;
    FString Current;
    bool bInQuotes = false;

    for (int32 i = 0; i < Line.Len(); ++i)
    {
        const TCHAR Ch = Line[i];
        if (Ch == '"')
        {
            if (bInQuotes && i + 1 < Line.Len() && Line[i + 1] == '"')
            {
                Current.AppendChar('"');
                ++i;
            }
            else
            {
                bInQuotes = !bInQuotes;
            }
        }
        else if (Ch == ',' && !bInQuotes)
        {
            Cells.Add(Current);
            Current.Empty();
        }
        else
        {
            Current.AppendChar(Ch);
        }
    }

    Cells.Add(Current);
    for (FString& Cell : Cells)
    {
        Cell = Cell.TrimStartAndEnd();
    }
    return Cells;
}

static bool ParseCsv(const FString& Csv, TArray<FParsedCsvRow>& OutRows, FString& OutError)
{
    OutRows.Reset();
    TArray<FString> Lines;
    Csv.ParseIntoArrayLines(Lines, true);
    if (Lines.Num() == 0)
    {
        OutError = TEXT("CSV is empty");
        return false;
    }

    int32 GuidCol = 0;
    int32 CommentCol = 1;
    bool bHeader = false;

    TArray<FString> FirstCells = TokenizeCsvRow(Lines[0]);
    auto EqualsCI = [](const FString& A, const TCHAR* B)
    {
        return A.Equals(B, ESearchCase::IgnoreCase);
    };

    if (FirstCells.Num() >= 2 && EqualsCI(FirstCells[0], TEXT("node_guid")) && EqualsCI(FirstCells[1], TEXT("comment")))
    {
        bHeader = true;
    }
    else if (FirstCells.Num() >= 2)
    {
        // detect any order of the required headers
        for (int32 i = 0; i < FirstCells.Num(); ++i)
        {
            if (EqualsCI(FirstCells[i], TEXT("node_guid")))
            {
                GuidCol = i;
            }
            else if (EqualsCI(FirstCells[i], TEXT("comment")))
            {
                CommentCol = i;
            }
        }

        if (EqualsCI(FirstCells[GuidCol], TEXT("node_guid")) && EqualsCI(FirstCells[CommentCol], TEXT("comment")))
        {
            bHeader = true;
        }
    }

    const int32 StartIndex = bHeader ? 1 : 0;
    for (int32 LineIdx = StartIndex; LineIdx < Lines.Num(); ++LineIdx)
    {
        const TArray<FString> Cells = TokenizeCsvRow(Lines[LineIdx]);
        if (Cells.Num() == 0)
        {
            continue;
        }
        if (!Cells.IsValidIndex(GuidCol) || !Cells.IsValidIndex(CommentCol))
        {
            continue;
        }

        FParsedCsvRow Row;
        Row.GuidStr = Cells[GuidCol];
        Row.Comment = Cells[CommentCol];
        Row.Comment.TrimStartAndEndInline();
        OutRows.Add(MoveTemp(Row));
    }

    if (OutRows.Num() == 0)
    {
        OutError = TEXT("No CSV rows parsed");
        return false;
    }

    return true;
}
} // namespace

void SBEP_NodeJsonPanel::Construct(const FArguments& InArgs)
{
    Settings = GetMutableDefault<UBEP_NodeJsonExportSettings>();

    FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bShowOptions = false;
    DetailsArgs.bShowPropertyMatrixButton = false;
    DetailsArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

    SettingsView = PropertyEditor.CreateDetailView(DetailsArgs);
    if (SettingsView.IsValid() && Settings)
    {
        SettingsView->SetObject(Settings);
    }

    ChildSlot
    [
        SNew(SBorder)
        .Padding(10.f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Selected Nodes -> JSON")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 2.f, 0.f, 8.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(BuildHeaderSubtitle()))
                .WrapTextAt(480.f)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 0.f, 0.f, 6.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Preset:")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(6.f, 0.f, 6.f, 0.f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (!Settings)
                        {
                            return FText::FromString(TEXT("(no settings)"));
                        }
                        return FText::FromString(PresetToString(Settings->DefaultPreset));
                    })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Apply Preset")))
                    .OnClicked(this, &SBEP_NodeJsonPanel::OnApplyPresetClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 0.f, 0.f, 8.f)
            [
                SettingsView.IsValid() ? SettingsView.ToSharedRef() : SNullWidget::NullWidget
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 0.f, 0.f, 4.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 4.f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Quick Presets:")))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4.f, 0.f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("SuperCompact")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnQuickPresetClicked, EBEP_NodeJsonPreset::SuperCompact)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4.f, 0.f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Compact")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnQuickPresetClicked, EBEP_NodeJsonPreset::Compact)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4.f, 0.f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("AI Minimal")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnQuickPresetClicked, EBEP_NodeJsonPreset::AIMinimal)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4.f, 0.f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("AI Audit")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnQuickPresetClicked, EBEP_NodeJsonPreset::AIAudit)
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SUniformGridPanel)
                    .SlotPadding(FMargin(4.f, 0.f))
                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Export JSON (Selection)")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnExportClicked)
                    ]
                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Copy JSON (Selection)")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnCopyClicked)
                    ]
                    + SUniformGridPanel::Slot(2, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Export JSON + Copy Path")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnExportCopyPathClicked)
                    ]
                    + SUniformGridPanel::Slot(3, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Preview / Dry Run")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnPreviewClicked)
                    ]
                    + SUniformGridPanel::Slot(0, 1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Export Comment JSON")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnExportCommentsClicked)
                    ]
                    + SUniformGridPanel::Slot(1, 1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Export Comment JSON + Template")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnExportCommentsTemplateClicked)
                    ]
                    + SUniformGridPanel::Slot(2, 1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Import Comments CSV…")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnImportCommentsClicked)
                    ]
                    + SUniformGridPanel::Slot(3, 1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("BEP Golden Samples")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnGoldenSamplesClicked)
                    ]
                    + SUniformGridPanel::Slot(0, 2)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Open Output Folder")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnOpenFolderClicked)
                    ]
                    + SUniformGridPanel::Slot(1, 2)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Open Last File")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnOpenLastFileClicked)
                    ]
                    + SUniformGridPanel::Slot(2, 2)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Copy Last Saved Path")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnCopyLastPathClicked)
                    ]
                    + SUniformGridPanel::Slot(3, 2)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Copy Summary")))
                        .OnClicked(this, &SBEP_NodeJsonPanel::OnCopySummaryClicked)
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 4.f)
            [
                SNew(SSeparator)
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SAssignNew(StatusTextBox, SMultiLineEditableTextBox)
                .IsReadOnly(true)
                .AutoWrapText(true)
                .AlwaysShowScrollbars(false)
                .Text(FText::FromString(TEXT("Ready. Select Blueprint graph nodes and export or copy.")))
            ]
        ]
    ];
}

FReply SBEP_NodeJsonPanel::OnApplyPresetClicked()
{
    if (Settings)
    {
        Settings->ApplyPresetToSettings(Settings->DefaultPreset);
        Settings->SaveConfig();
        UpdateStatus(TEXT("Applied default preset."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Preset applied"), true);
    }
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnQuickPresetClicked(EBEP_NodeJsonPreset Preset)
{
    if (Settings)
    {
        Settings->ApplyPresetToSettings(Preset);
        Settings->DefaultPreset = Preset;
        Settings->SaveConfig();
        UpdateStatus(FString::Printf(TEXT("Applied preset: %s"), *PresetToString(Preset)));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Preset applied"), true);
    }
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnExportClicked()
{
    RunExport(false);
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnCopyClicked()
{
    RunExport(true);
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnExportCopyPathClicked()
{
    RunExportWithPathCopy();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnPreviewClicked()
{
    RunPreview();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnExportCommentsTemplateClicked()
{
    RunCommentExportWithTemplate();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnGoldenSamplesClicked()
{
    RunGoldenSamples();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnOpenLastFileClicked()
{
    const FString PathToOpen = !LastSavedPath.IsEmpty()
        ? LastSavedPath
        : (!LastCommentPath.IsEmpty() ? LastCommentPath : LastCommentTemplatePath);
    if (PathToOpen.IsEmpty())
    {
        UpdateStatus(TEXT("No file to open."));
        return FReply::Handled();
    }
    FPlatformProcess::LaunchFileInDefaultExternalApplication(*PathToOpen);
    UpdateStatus(FString::Printf(TEXT("Opened file: %s"), *PathToOpen));
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnCopySummaryClicked()
{
    if (LastSummaryPath.IsEmpty())
    {
        UpdateStatus(TEXT("No summary available."));
        return FReply::Handled();
    }

    FString SummaryContent;
    if (FFileHelper::LoadFileToString(SummaryContent, *LastSummaryPath))
    {
        FPlatformApplicationMisc::ClipboardCopy(*SummaryContent);
        UpdateStatus(TEXT("Copied summary to clipboard."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Copied summary"), true);
    }
    else
    {
        UpdateStatus(TEXT("Failed to read summary."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Copy summary failed"), false);
    }
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnExportCommentsClicked()
{
    RunCommentExport();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnImportCommentsClicked()
{
    RunCommentImport();
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnOpenFolderClicked()
{
    const FString Folder = !LastSavedPath.IsEmpty()
        ? FPaths::GetPath(LastSavedPath)
        : (!LastCommentPath.IsEmpty() ? FPaths::GetPath(LastCommentPath) : GetOutputFolderForStatus());
    if (Folder.IsEmpty())
    {
        UpdateStatus(TEXT("No output folder to open."));
        return FReply::Handled();
    }

    FPlatformProcess::ExploreFolder(*Folder);
    UpdateStatus(FString::Printf(TEXT("Opened folder: %s"), *Folder));
    return FReply::Handled();
}

FReply SBEP_NodeJsonPanel::OnCopyLastPathClicked()
{
    const FString PathToCopy = !LastSavedPath.IsEmpty()
        ? LastSavedPath
        : (!LastCommentPath.IsEmpty() ? LastCommentPath : LastCommentTemplatePath);
    if (PathToCopy.IsEmpty())
    {
        UpdateStatus(TEXT("No saved path available."));
        return FReply::Handled();
    }

    FPlatformApplicationMisc::ClipboardCopy(*PathToCopy);
    UpdateStatus(FString::Printf(TEXT("Copied path: %s"), *PathToCopy));
    BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Copied last saved path"), true);
    return FReply::Handled();
}

bool SBEP_NodeJsonPanel::RunExport(bool bCopyOnly)
{
    if (Settings)
    {
        Settings->SaveConfig();
    }

    LastSummaryPath.Empty();

    FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();

    FString Json;
    FString SuggestedStem;
    FString Error;
    int32 NodeCount = 0;
    int32 EdgeCount = 0;
    FBEP_NodeJsonExportStats Stats;

    if (!BEP_NodeJson::ExportActiveSelectionToJson(Opt, Json, SuggestedStem, NodeCount, EdgeCount, &Error, &Stats, false))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("No selected nodes in active Blueprint graph.") : Error;
        UpdateStatus(FString::Printf(TEXT("Export failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    bHasLastStats = true;
    LastStats = Stats;
    LastPreset = Opt.Preset;

    if (bCopyOnly)
    {
        FPlatformApplicationMisc::ClipboardCopy(*Json);
        LastSavedPath.Empty();
        LastSummaryPath.Empty();
        LastJsonPreview = Json.Left(4000);

        const FString Summary = FString::Printf(TEXT("Copied Node JSON to clipboard.\nSeed: %d\nExpanded: %d\nFinal Nodes: %d\nEdges: %d (Exec %d / Data %d)\nPreset: %s\nCaps: NodeHit=%s EdgeHit=%s"),
            Stats.SeedSelectionCount,
            Stats.ExpandedNodeCount,
            Stats.FinalNodeCount,
            EdgeCount,
            Stats.ExecEdgeCount,
            Stats.DataEdgeCount,
            *PresetToString(Opt.Preset),
            Stats.bHitNodeCap ? TEXT("true") : TEXT("false"),
            Stats.bHitEdgeCap ? TEXT("true") : TEXT("false"));

        UpdateStatus(Summary);
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Copied Node JSON"), true);
        return true;
    }

    FString SavedPath;
    FString SummaryPath;
    if (!BEP_NodeJson::SaveJsonToFile(Json, SuggestedStem, Opt, &Stats, SavedPath, &SummaryPath, &Error))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
        UpdateStatus(FString::Printf(TEXT("Save failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    LastSavedPath = SavedPath;
    LastSummaryPath = SummaryPath;
    LastJsonPreview = Json.Left(4000);

    const FString Summary = FString::Printf(TEXT("Saved Node JSON.\nSeed: %d\nExpanded: %d\nFinal Nodes: %d\nEdges: %d (Exec %d / Data %d)\nExecDepth: %d DataDepth: %d\nPreset: %s\nCaps: NodeHit=%s EdgeHit=%s\nSaved: %s\nSummary: %s"),
        Stats.SeedSelectionCount,
        Stats.ExpandedNodeCount,
        Stats.FinalNodeCount,
        EdgeCount,
        Stats.ExecEdgeCount,
        Stats.DataEdgeCount,
        Stats.ExecDepthUsed,
        Stats.DataDepthUsed,
        *PresetToString(Opt.Preset),
        Stats.bHitNodeCap ? TEXT("true") : TEXT("false"),
        Stats.bHitEdgeCap ? TEXT("true") : TEXT("false"),
        *SavedPath,
        *SummaryPath);

    UpdateStatus(Summary);
    BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Saved Node JSON (%d nodes, %d edges)"), NodeCount, EdgeCount), true);
    return true;
}

bool SBEP_NodeJsonPanel::RunExportWithPathCopy()
{
    const bool bOk = RunExport(false);
    if (bOk && !LastSavedPath.IsEmpty())
    {
        FPlatformApplicationMisc::ClipboardCopy(*LastSavedPath);
        UpdateStatus(FString::Printf(TEXT("Saved and copied path: %s"), *LastSavedPath));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Saved JSON + copied path"), true);
    }
    return bOk;
}

bool SBEP_NodeJsonPanel::RunPreview()
{
    FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
    FString DummyJson;
    FString Stem;
    FString Error;
    int32 Nodes = 0;
    int32 Edges = 0;
    FBEP_NodeJsonExportStats Stats;
    if (!BEP_NodeJson::ExportActiveSelectionToJson(Opt, DummyJson, Stem, Nodes, Edges, &Error, &Stats, true))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("No selected nodes in active Blueprint graph.") : Error;
        UpdateStatus(FString::Printf(TEXT("Preview failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Preview failed"), false);
        return false;
    }

    bHasLastStats = true;
    LastStats = Stats;
    LastPreset = Opt.Preset;

    const FString Summary = FString::Printf(TEXT("Preview only.\nSeed: %d\nExpanded: %d\nFinal Nodes: %d (cap %d hit=%s)\nEdges: %d (Exec %d / Data %d, cap %d hit=%s)\nExecDepth: %d DataDepth: %d"),
        Stats.SeedSelectionCount,
        Stats.ExpandedNodeCount,
        Stats.FinalNodeCount,
        Stats.MaxNodeCap,
        Stats.bHitNodeCap ? TEXT("true") : TEXT("false"),
        Edges,
        Stats.ExecEdgeCount,
        Stats.DataEdgeCount,
        Stats.MaxEdgeCap,
        Stats.bHitEdgeCap ? TEXT("true") : TEXT("false"),
        Stats.ExecDepthUsed,
        Stats.DataDepthUsed);

    UpdateStatus(Summary);
    BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Preview complete"), true);
    return true;
}

bool SBEP_NodeJsonPanel::RunCommentExport()
{
    if (Settings)
    {
        Settings->SaveConfig();
    }

    FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
    FString Json;
    FString SuggestedStem;
    FString Error;
    if (!BEP_NodeJson::ExportActiveSelectionToCommentJson(Opt, Json, SuggestedStem, nullptr, &Error))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("No selected nodes in active Blueprint graph.") : Error;
        UpdateStatus(FString::Printf(TEXT("Comment export failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    FString SavedPath;
    if (!BEP_NodeJson::SaveCommentJsonToFile(Json, SuggestedStem, Opt, SavedPath, nullptr, &Error, nullptr, false))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
        UpdateStatus(FString::Printf(TEXT("Save failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    LastCommentPath = SavedPath;
    LastCommentTemplatePath.Empty();
    LastSummaryPath.Empty();
    UpdateStatus(FString::Printf(TEXT("Saved Comment JSON: %s"), *SavedPath));
    BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Saved Comment JSON"), true);
    return true;
}

bool SBEP_NodeJsonPanel::RunCommentExportWithTemplate()
{
    if (Settings)
    {
        Settings->SaveConfig();
    }

    FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
    FString Json;
    FString SuggestedStem;
    FString Error;
    TArray<FString> Guids;
    if (!BEP_NodeJson::ExportActiveSelectionToCommentJson(Opt, Json, SuggestedStem, &Guids, &Error))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("No selected nodes in active Blueprint graph.") : Error;
        UpdateStatus(FString::Printf(TEXT("Comment export failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    FString SavedPath;
    FString TemplatePath;
    if (!BEP_NodeJson::SaveCommentJsonToFile(Json, SuggestedStem, Opt, SavedPath, &TemplatePath, &Error, &Guids, false))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
        UpdateStatus(FString::Printf(TEXT("Save failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    LastCommentPath = SavedPath;
    LastCommentTemplatePath = TemplatePath;
    LastSummaryPath.Empty();
    UpdateStatus(FString::Printf(TEXT("Saved Comment JSON + template.\nJSON: %s\nTemplate: %s\nGUIDs: %d"), *SavedPath, *TemplatePath, Guids.Num()));
    BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Saved Comment JSON + template"), true);
    return true;
}

bool SBEP_NodeJsonPanel::RunGoldenSamples()
{
    if (Settings)
    {
        Settings->SaveConfig();
    }

    FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
    FString Json;
    FString Stem;
    FString Error;
    int32 Nodes = 0;
    int32 Edges = 0;
    FBEP_NodeJsonExportStats Stats;

    if (!BEP_NodeJson::ExportActiveSelectionToJson(Opt, Json, Stem, Nodes, Edges, &Error, &Stats, false))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("No selected nodes in active Blueprint graph.") : Error;
        UpdateStatus(FString::Printf(TEXT("Golden sample export failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    FString SavedPath;
    FString SummaryPath;
    if (!BEP_NodeJson::SaveJsonToFile(Json, Stem, Opt, &Stats, SavedPath, &SummaryPath, &Error, true))
    {
        const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
        UpdateStatus(FString::Printf(TEXT("Golden sample save failed: %s"), *Reason));
        BEPNodeJsonUI::ShowNodeJsonToast(Reason, false);
        return false;
    }

    FString CommentJson;
    FString CommentStem;
    FString CommentError;
    TArray<FString> CommentGuids;
    FString CommentPath;
    FString TemplatePath;
    bool bCommentOk = false;
    if (BEP_NodeJson::ExportActiveSelectionToCommentJson(Opt, CommentJson, CommentStem, &CommentGuids, &CommentError))
    {
        bCommentOk = BEP_NodeJson::SaveCommentJsonToFile(CommentJson, CommentStem, Opt, CommentPath, &TemplatePath, &CommentError, &CommentGuids, true);
    }
    else if (!CommentError.IsEmpty())
    {
        UpdateStatus(FString::Printf(TEXT("Golden sample JSON saved, but comment export skipped: %s"), *CommentError));
    }

    LastSavedPath = SavedPath;
    LastSummaryPath = SummaryPath;
    LastCommentPath = CommentPath;
    LastCommentTemplatePath = TemplatePath;
    LastJsonPreview = Json.Left(4000);

    const FString Summary = FString::Printf(TEXT("Golden sample saved.\nNodes: %d Edges: %d\nSaved: %s\nSummary: %s\nComment JSON: %s\nTemplate: %s"),
        Nodes,
        Edges,
        *SavedPath,
        *SummaryPath,
        bCommentOk ? *CommentPath : TEXT("(comment export skipped)"),
        bCommentOk ? *TemplatePath : TEXT("(template skipped)"));

    UpdateStatus(Summary);
    BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Golden samples written"), true);
    return true;
}

bool SBEP_NodeJsonPanel::RunCommentImport()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        UpdateStatus(TEXT("Desktop platform module unavailable."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Import failed: no desktop platform"), false);
        return false;
    }

    void* ParentWindowHandle = nullptr;
    if (TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr))
    {
        ParentWindowHandle = ParentWindow->GetNativeWindow().IsValid() ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
    }

    TArray<FString> OutFiles;
    const bool bOpened = DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        TEXT("Select Comments CSV"),
        FPaths::ProjectDir(),
        TEXT(""),
        TEXT("CSV Files (*.csv)|*.csv|All Files (*.*)|*.*"),
        EFileDialogFlags::None,
        OutFiles);

    if (!bOpened || OutFiles.Num() == 0)
    {
        UpdateStatus(TEXT("Import cancelled."));
        return false;
    }

    const FString CsvPath = OutFiles[0];
    FString CsvContent;
    if (!FFileHelper::LoadFileToString(CsvContent, *CsvPath))
    {
        UpdateStatus(FString::Printf(TEXT("Failed to read CSV: %s"), *CsvPath));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Import failed: read error"), false);
        return false;
    }

    TArray<FParsedCsvRow> Rows;
    FString ParseError;
    if (!ParseCsv(CsvContent, Rows, ParseError))
    {
        UpdateStatus(FString::Printf(TEXT("CSV parse failed: %s"), *ParseError));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Import failed: parse error"), false);
        return false;
    }

    const TSharedPtr<SGraphPanel> GraphPanel = FindActiveGraphPanel();
    if (!GraphPanel.IsValid())
    {
        UpdateStatus(TEXT("No active graph editor."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Import failed: no graph"), false);
        return false;
    }

    UEdGraph* Graph = GraphPanel->GetGraphObj();
    if (!Graph)
    {
        UpdateStatus(TEXT("No active graph."));
        BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Import failed: no graph"), false);
        return false;
    }

    // Map NodeGuid -> Node
    TMap<FGuid, UEdGraphNode*> GuidToNode;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeGuid.IsValid())
        {
            GuidToNode.Add(Node->NodeGuid, Node);
        }
    }

    if (GuidToNode.Num() == 0)
    {
        const FString Msg = TEXT("Graph has no nodes with valid GUIDs.");
        UpdateStatus(Msg);
        BEPNodeJsonUI::ShowNodeJsonToast(Msg, false);
        return false;
    }

    const int32 MaxToCreate = 500;
    int32 Created = 0;
    int32 Skipped = 0;
    FString SkipLog;

    const FScopedTransaction Transaction(NSLOCTEXT("BEP", "BEP_ImportComments", "BEP Import Comments"));
    Graph->Modify();

    TArray<UEdGraphNode_Comment*> CreatedComments;

    for (int32 RowIdx = 0; RowIdx < Rows.Num(); ++RowIdx)
    {
        if (Created >= MaxToCreate)
        {
            SkipLog += FString::Printf(TEXT("Row %d skipped (cap reached).\n"), RowIdx + 1);
            ++Skipped;
            continue;
        }

        FGuid Guid;
        if (!FGuid::Parse(Rows[RowIdx].GuidStr, Guid))
        {
            SkipLog += FString::Printf(TEXT("Row %d invalid guid: %s\n"), RowIdx + 1, *Rows[RowIdx].GuidStr);
            ++Skipped;
            continue;
        }

        if (Rows[RowIdx].Comment.IsEmpty())
        {
            SkipLog += FString::Printf(TEXT("Row %d empty comment\n"), RowIdx + 1);
            ++Skipped;
            continue;
        }

        UEdGraphNode** Found = GuidToNode.Find(Guid);
        if (!Found || !*Found)
        {
            SkipLog += FString::Printf(TEXT("Row %d guid not found in graph: %s\n"), RowIdx + 1, *Rows[RowIdx].GuidStr);
            ++Skipped;
            continue;
        }

        UEdGraphNode* TargetNode = *Found;
        TargetNode->Modify();

        UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph);
        CommentNode->CommentColor = FLinearColor::White;
        CommentNode->NodeComment = Rows[RowIdx].Comment;
        CommentNode->NodePosX = TargetNode->NodePosX - 40;
        CommentNode->NodePosY = TargetNode->NodePosY - 60;
        const int32 CommentLen = Rows[RowIdx].Comment.Len();
        const int32 DesiredWidth = FMath::Clamp(200 + CommentLen * 3, 300, 800);
        const int32 DesiredHeight = FMath::Clamp(100 + CommentLen / 2, 120, 400);
        CommentNode->NodeWidth = DesiredWidth;
        CommentNode->NodeHeight = DesiredHeight;
        Graph->AddNode(CommentNode, true, false);
        CommentNode->CreateNewGuid();

        CreatedComments.Add(CommentNode);
        ++Created;
    }

    if (GraphPanel.IsValid())
    {
        GraphPanel->SelectionManager.ClearSelectionSet();
        for (UEdGraphNode_Comment* CommentNode : CreatedComments)
        {
            GraphPanel->SelectionManager.SetNodeSelection(CommentNode, true);
        }
    }

    Graph->NotifyGraphChanged();

    FString Summary = FString::Printf(TEXT("Imported %d comments; skipped %d."), Created, Skipped);
    if (!SkipLog.IsEmpty())
    {
        Summary += TEXT("\n") + SkipLog;
    }

    UpdateStatus(Summary);
    BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Imported %d comments"), Created), true);
    LastCommentPath = CsvPath;
    return Created > 0;
}

void SBEP_NodeJsonPanel::UpdateStatus(const FString& InStatus)
{
    if (StatusTextBox.IsValid())
    {
        FString Enriched = InStatus;
        if (bHasLastStats)
        {
            Enriched += FString::Printf(TEXT("\nPreset: %s | Nodes: %d | Edges: %d (exec %d / data %d) | Caps Hit: N=%s E=%s"),
                *PresetToString(LastPreset),
                LastStats.FinalNodeCount,
                LastStats.ExecEdgeCount + LastStats.DataEdgeCount,
                LastStats.ExecEdgeCount,
                LastStats.DataEdgeCount,
                LastStats.bHitNodeCap ? TEXT("true") : TEXT("false"),
                LastStats.bHitEdgeCap ? TEXT("true") : TEXT("false"));
        }

        const FString AnyPath = !LastSavedPath.IsEmpty() ? LastSavedPath : (!LastCommentPath.IsEmpty() ? LastCommentPath : LastCommentTemplatePath);
        if (!AnyPath.IsEmpty())
        {
            Enriched += FString::Printf(TEXT("\nLast path: %s"), *AnyPath);
        }

        StatusTextBox->SetText(FText::FromString(Enriched));
    }
}

FString SBEP_NodeJsonPanel::BuildHeaderSubtitle() const
{
    const FString Root = GetDefaultBEPExportRoot();
    const FString FullPath = FPaths::ConvertRelativePathToFull(Root);
    return FString::Printf(TEXT("Output Root: %s"), *FullPath);
}

FString SBEP_NodeJsonPanel::GetOutputFolderForStatus() const
{
    const FString Root = GetDefaultBEPExportRoot();
    if (Root.IsEmpty())
    {
        return FString();
    }
    return FPaths::Combine(Root, TEXT("NodeJSON"));
}
