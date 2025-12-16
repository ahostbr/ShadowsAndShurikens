#pragma once

#include "Delegates/Delegate.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBEP, Log, All);

struct FAssetData;

class FBEPModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterContentBrowserHooks();
	void UnregisterContentBrowserHooks();
	TSharedRef<class FExtender> OnExtendContentBrowserPathMenu(const TArray<FString>& SelectedPaths);
	TSharedRef<class FExtender> OnExtendContentBrowserAssetMenu(const TArray<FAssetData>& SelectedAssets);
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
	FDelegateHandle ContentBrowserAssetExtenderHandle;
	FDelegateHandle MenuRegistrationHandle;

	TWeakPtr<class SBEPExportPanel> ExportPanelWeak;

	TSharedPtr<class FUICommandList> NodeJsonCommandList;
};

