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

	void RegisterNodeJsonCommands();
	void UnregisterNodeJsonCommands();
	void BindNodeJsonCommands();

	void RegisterSettings();
	void UnregisterSettings();

	void ExecuteOpenNodeJsonPanel();
	void ExecuteExportNodeJson();
	void ExecuteCopyNodeJson();
	void ExecuteExportCommentJson();
	void ExecuteImportComments();
	void ExecuteWriteGoldenSamples();
	void ExecuteSelfCheck();

	bool CanRunNodeJsonSelectionAction() const;
	bool CanImportComments() const;
	bool CanRunSelfCheck() const;

private:
	FDelegateHandle ContentBrowserPathExtenderHandle;
	FDelegateHandle MenuRegistrationHandle;

	TSharedPtr<class FUICommandList> NodeJsonCommandList;
};

