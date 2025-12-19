#include "SOTS_BPGen_BridgeModule.h"
#include "SOTS_BPGenBridgeServer.h"

#define LOCTEXT_NAMESPACE "FSOTS_BPGen_BridgeModule"

static TWeakPtr<FSOTS_BPGenBridgeServer> GSharedBridgeServer;

void FSOTS_BPGen_BridgeModule::StartupModule()
{
	Server = MakeShared<FSOTS_BPGenBridgeServer>();
	GSharedBridgeServer = Server;

	if (Server.IsValid())
	{
		Server->Start();
	}
}

void FSOTS_BPGen_BridgeModule::ShutdownModule()
{
	if (Server.IsValid())
	{
		Server->Stop();
		Server.Reset();
	}

	GSharedBridgeServer.Reset();
}

FSOTS_BPGenBridgeServer* FSOTS_BPGen_BridgeModule::GetServer()
{
	if (GSharedBridgeServer.IsValid())
	{
		return GSharedBridgeServer.Pin().Get();
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSOTS_BPGen_BridgeModule, SOTS_BPGen_Bridge)
