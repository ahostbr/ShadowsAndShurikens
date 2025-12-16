// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"

//
// ----------------------------------------------
// Log categories for Ninja Input.
//
// This component can output a lot of verbose/very verbose information and if you are interested in
// using these messages for debugging, you can enable these categories in your DefaultEngine.ini file:
//
// [Core.Log]
// LogNinjaInputManagerComponent=VeryVerbose
// ----------------------------------------------
//

DECLARE_LOG_CATEGORY_EXTERN(LogNinjaInputManagerComponent, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNinjaInputBufferComponent, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNinjaInputCombatComboHandler, Log, All);