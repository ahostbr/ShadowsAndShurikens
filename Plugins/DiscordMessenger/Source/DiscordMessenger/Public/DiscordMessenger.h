// Copyright © 2023 David ten Kate - All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FDiscordMessengerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
