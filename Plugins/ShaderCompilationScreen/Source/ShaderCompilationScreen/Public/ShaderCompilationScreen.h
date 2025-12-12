// Copyright Yuzhda Bohdan (Bitkovin) 2023

#pragma once

#include "Modules/ModuleManager.h"

class FShaderCompilationScreenModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
