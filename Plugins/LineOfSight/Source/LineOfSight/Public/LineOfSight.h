// Copyright (c) 2021 Evgeniy Oshmarin

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FLineOfSightModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
