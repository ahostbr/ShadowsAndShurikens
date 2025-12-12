#pragma once

#include "Modules/ModuleManager.h"
#include "Delegates/Delegate.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBEP, Log, All);

class FBEPModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterContentBrowserHooks();
	void UnregisterContentBrowserHooks();
	TSharedRef<class FExtender> OnExtendContentBrowserPathMenu(const TArray<FString>& SelectedPaths);
	void ExecuteExportFolderWithBEP(const TArray<FString>& SelectedPaths);

	void RegisterTabSpawner();
	void UnregisterTabSpawner();
	TSharedRef<class SDockTab> SpawnBEPExportTab(const class FSpawnTabArgs& Args);

	void RegisterMenus();
	void RegisterMenus_Impl();

private:
	FDelegateHandle ContentBrowserPathExtenderHandle;
	FDelegateHandle MenuRegistrationHandle;
};

