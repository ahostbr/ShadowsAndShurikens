#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSOTS_BPGenBridgeServer;

class FSOTS_BPGen_BridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FSOTS_BPGenBridgeServer* GetServer();

private:
	TSharedPtr<FSOTS_BPGenBridgeServer> Server;
};
