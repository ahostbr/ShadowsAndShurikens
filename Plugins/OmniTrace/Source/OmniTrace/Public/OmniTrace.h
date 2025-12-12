#pragma once

#include "CoreMinimal.h"
#include "OmniTraceTypes.h"
#include "Modules/ModuleManager.h"


class FOmniTraceModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
