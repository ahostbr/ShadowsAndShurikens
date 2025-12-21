#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSOTS_BPGenBridgeServer;
class SDockTab;
class FSpawnTabArgs;

class FSOTS_BPGen_BridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FSOTS_BPGenBridgeServer* GetServer();
	static TSharedPtr<FSOTS_BPGenBridgeServer> GetServerShared();

private:
	TSharedRef<class SDockTab> SpawnBPGenControlCenterTab(const class FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();

	TSharedPtr<FSOTS_BPGenBridgeServer> Server;
	FDelegateHandle MenuRegistrationHandle;
};
