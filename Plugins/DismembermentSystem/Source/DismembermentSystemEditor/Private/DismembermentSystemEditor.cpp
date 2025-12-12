#include "DismembermentSystemEditor.h"

#include "Settings/ProjectPackagingSettings.h"

#define LOCTEXT_NAMESPACE "FDismembermentSystemEditorModule"

void FDismembermentSystemEditorModule::StartupModule()
{
    UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
    if(!ensureMsgf(PackagingSettings, TEXT("Packaging Settings was not loaded before Dismemberment System. This will not cause any issues, but it is recommended that you add '/DismembermentSystem' without the quotes to your 'AdditionAssetDirectoriesToCook' in the project settings.")))
    {
        return;
    }
    
    if(!HasDirectoryBeenAdded(PackagingSettings))
    {
        PackagingSettings->Modify();
        PackagingSettings->DirectoriesToAlwaysCook.Add(FDirectoryPath("/DismembermentSystem"));
        PackagingSettings->SaveConfig();
    }
}

void FDismembermentSystemEditorModule::ShutdownModule()
{
    
}

bool FDismembermentSystemEditorModule::HasDirectoryBeenAdded(UProjectPackagingSettings* PackagingSettings) const
{
    for (FDirectoryPath& Path : PackagingSettings->DirectoriesToAlwaysCook)
    {
        if(Path.Path == "/DismembermentSystem")
        {
            return true;
        }
    }
    return false;
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDismembermentSystemEditorModule, DismembermentSystemEditor)