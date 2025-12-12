#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetData.h"
#include "DesktopPlatformModule.h"
#include "Engine/DataAsset.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"
#include "SOTS_GenericJsonImporter.h"
#include "Styling/CoreStyle.h"
#include "ToolMenus.h"
#include "Types/SlateEnums.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SSOTSJsonImportWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSOTSJsonImportWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ChildSlot
        [
            SNew(SBorder)
            .Padding(8.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 8.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("SOTS JSON Importer")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Target Data Asset")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SNew(SObjectPropertyEntryBox)
                    .AllowedClass(UDataAsset::StaticClass())
                    .ObjectPath(this, &SSOTSJsonImportWidget::GetAssetPath)
                    .OnObjectChanged(this, &SSOTSJsonImportWidget::OnAssetPicked)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("JSON File Path")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SNew(SUniformGridPanel)
                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SAssignNew(JsonPathTextBox, SEditableTextBox)
                        .MinDesiredWidth(300.f)
                    ]
                    + SUniformGridPanel::Slot(1, 0)
                    .HAlign(HAlign_Left)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Browse")))
                        .OnClicked(this, &SSOTSJsonImportWidget::OnBrowseJson)
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Array Property Name (optional)")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SAssignNew(ArrayPropTextBox, SEditableTextBox)
                    .MinDesiredWidth(300.f)
                    .HintText(FText::FromString(TEXT("Defaults to 'Actions' or the only array")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SAssignNew(MarkDirtyCheckBox, SCheckBox)
                    .IsChecked(ECheckBoxState::Checked)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Mark asset dirty after import")))
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Import JSON")))
                    .OnClicked(this, &SSOTSJsonImportWidget::OnImportClicked)
                ]
            ]
        ];
    }

private:
    FReply OnBrowseJson()
    {
        IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
        if (!DesktopPlatform)
        {
            return FReply::Handled();
        }

        void* ParentWindowHandle = nullptr;
        TArray<FString> OutFiles;
        const bool bOpened = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("Choose JSON file"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("JSON Files (*.json)|*.json|All Files (*.*)|*.*"),
            EFileDialogFlags::None,
            OutFiles);

        if (bOpened && OutFiles.Num() > 0 && JsonPathTextBox.IsValid())
        {
            JsonPathTextBox->SetText(FText::FromString(OutFiles[0]));
        }

        return FReply::Handled();
    }

    FReply OnImportClicked()
    {
        if (!TargetAsset.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("SOTS_EdUtil: Select a DataAsset before importing."));
            return FReply::Handled();
        }

        const FString JsonPath = JsonPathTextBox.IsValid() ? JsonPathTextBox->GetText().ToString() : FString();
        const FString ArrayPropNameString = ArrayPropTextBox.IsValid() ? ArrayPropTextBox->GetText().ToString() : FString();
        const FName ArrayPropName = ArrayPropNameString.IsEmpty() ? NAME_None : FName(*ArrayPropNameString);
        const ECheckBoxState CheckedState = MarkDirtyCheckBox.IsValid() ? MarkDirtyCheckBox->GetCheckedState() : ECheckBoxState::Checked;
        const bool bMarkDirty = (CheckedState == ECheckBoxState::Checked);

        UDataAsset* Asset = Cast<UDataAsset>(TargetAsset.GetAsset());
        const bool bResult = USOTS_GenericJsonImporter::ImportJson(Asset, JsonPath, ArrayPropName, bMarkDirty);

        if (!bResult)
        {
            UE_LOG(LogTemp, Warning, TEXT("SOTS_EdUtil: Import failed (see previous log for details)."));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("SOTS_EdUtil: Import succeeded."));
        }

        return FReply::Handled();
    }

    void OnAssetPicked(const FAssetData& InAssetData)
    {
        TargetAsset = InAssetData;
    }

    FString GetAssetPath() const
    {
        return TargetAsset.IsValid() ? TargetAsset.ToSoftObjectPath().ToString() : FString();
    }

private:
    TSharedPtr<SEditableTextBox> JsonPathTextBox;
    TSharedPtr<SEditableTextBox> ArrayPropTextBox;
    TSharedPtr<SCheckBox> MarkDirtyCheckBox;
    FAssetData TargetAsset;
};

class FSOTS_EdUtilModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateRaw(this, &FSOTS_EdUtilModule::SpawnImporterTab))
            .SetDisplayName(FText::FromString(TEXT("SOTS JSON Importer")))
            .SetTooltipText(FText::FromString(TEXT("Import JSON into DataAssets using SOTS_EdUtil")))
            .SetMenuType(ETabSpawnerMenuType::Hidden);

        RegisterMenus();
    }

    virtual void ShutdownModule() override
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);

        if (UToolMenus* ToolMenus = UToolMenus::TryGet())
        {
            ToolMenus->UnregisterOwner(this);
        }
    }

private:
    TSharedRef<SDockTab> SpawnImporterTab(const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SSOTSJsonImportWidget)
            ];
    }

    void RegisterMenus()
    {
        MenuRegistrationHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSOTS_EdUtilModule::RegisterMenus_Impl));
    }

    void RegisterMenus_Impl()
    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        if (!Menu)
        {
            return;
        }

        static const FName SectionName("SOTS_Tools");

        FToolMenuSection* Section = Menu->FindSection(SectionName);
        if (!Section)
        {
            Section = &Menu->AddSection(SectionName, FText::FromString(TEXT("SOTS Tools")));
        }

        Section->AddMenuEntry(
            "OpenSOTSJsonImporter",
            FText::FromString(TEXT("SOTS JSON Importer")),
            FText::FromString(TEXT("Open the SOTS JSON Importer tab.")),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(TabName);
            })));
    }

private:
    static const FName TabName;
    FDelegateHandle MenuRegistrationHandle;
};

const FName FSOTS_EdUtilModule::TabName(TEXT("SOTSJsonImporter"));

IMPLEMENT_MODULE(FSOTS_EdUtilModule, SOTS_EdUtil)
