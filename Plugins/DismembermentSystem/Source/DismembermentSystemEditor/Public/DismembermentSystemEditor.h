#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/*
 *  Post Engine Init Module to ensure that the Dismemberment System is added to the cook list.
 */
class FDismembermentSystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    bool HasDirectoryBeenAdded(class UProjectPackagingSettings* PackagingSettings) const;
};