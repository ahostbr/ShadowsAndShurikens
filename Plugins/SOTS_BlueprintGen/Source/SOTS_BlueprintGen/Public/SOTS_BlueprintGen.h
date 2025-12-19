#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSOTS_BlueprintGen, Log, All);

class SDockTab;
class FSpawnTabArgs;

class FSOTS_BlueprintGenModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnBPGenRunnerTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnBPGenControlCenterTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();

	FDelegateHandle MenuRegistrationHandle;
};
