#pragma once

#include "Modules/ModuleInterface.h"

class FSOTS_UDSBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};
