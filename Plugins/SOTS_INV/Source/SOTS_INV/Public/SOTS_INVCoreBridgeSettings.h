#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_INVCoreBridgeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS INV Core Bridge"))
class SOTS_INV_API USOTS_INVCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_INVCoreBridgeSettings();
    virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreSaveParticipantBridge;

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs;
};
