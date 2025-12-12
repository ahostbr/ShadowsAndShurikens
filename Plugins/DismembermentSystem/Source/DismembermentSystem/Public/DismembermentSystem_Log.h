// © 2021, Brock Marsh. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

DISMEMBERMENTSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogDismembermentSystem, Log, All);

#define DIS_LOG(Text, ...) UE_LOG(LogDismembermentSystem, Log, TEXT(Text), ##__VA_ARGS__);
#define DIS_WARN(Text, ...) UE_LOG(LogDismembermentSystem, Warning, TEXT(Text), ##__VA_ARGS__);
#define DIS_ERROR(Text, ...) UE_LOG(LogDismembermentSystem, Error, TEXT(Text), ##__VA_ARGS__);

#define DIS_WARN_RETURN(ReturnValue, Text, ...) { UE_LOG(LogDismembermentSystem, Warning, TEXT(Text), ##__VA_ARGS__); return ReturnValue; }
#define DIS_ERROR_RETURN(ReturnValue, Text, ...) { UE_LOG(LogDismembermentSystem, Error, TEXT(Text), ##__VA_ARGS__); return ReturnValue; }