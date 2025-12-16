#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "BEP_NodeJsonExport.h"

struct FBEP_NodeJsonExportOptions;
class UBEP_NodeJsonExportSettings;
class IDetailsView;
class SMultiLineEditableTextBox;

class SBEP_NodeJsonPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBEP_NodeJsonPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnApplyPresetClicked();
    FReply OnQuickPresetClicked(EBEP_NodeJsonPreset Preset);
    FReply OnExportClicked();
    FReply OnCopyClicked();
    FReply OnExportCommentsClicked();
    FReply OnImportCommentsClicked();
    FReply OnPreviewClicked();
    FReply OnExportCopyPathClicked();
    FReply OnExportCommentsTemplateClicked();
    FReply OnGoldenSamplesClicked();
    FReply OnOpenLastFileClicked();
    FReply OnCopySummaryClicked();
    FReply OnOpenFolderClicked();
    FReply OnCopyLastPathClicked();

    bool RunExport(bool bCopyOnly);
    bool RunExportWithPathCopy();
    bool RunPreview();
    bool RunCommentExport();
    bool RunCommentExportWithTemplate();
    bool RunCommentImport();
    bool RunGoldenSamples();
    void UpdateStatus(const FString& InStatus);
    FString BuildHeaderSubtitle() const;
    FString GetOutputFolderForStatus() const;

private:
    UBEP_NodeJsonExportSettings* Settings = nullptr;
    TSharedPtr<IDetailsView> SettingsView;
    TSharedPtr<SMultiLineEditableTextBox> StatusTextBox;

    bool bHasLastStats = false;
    FBEP_NodeJsonExportStats LastStats;
    EBEP_NodeJsonPreset LastPreset = EBEP_NodeJsonPreset::SuperCompact;
    FString LastSavedPath;
    FString LastJsonPreview;
    FString LastCommentPath;
    FString LastSummaryPath;
    FString LastCommentTemplatePath;
};
