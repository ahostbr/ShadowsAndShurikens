#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_SkillTreeCoreBridgeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS SkillTree Core Bridge"))
class SOTS_SKILLTREE_API USOTS_SkillTreeCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_SkillTreeCoreBridgeSettings();

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreSaveParticipantBridge;

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs;
};
