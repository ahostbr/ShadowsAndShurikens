#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "SOTS_MMSS_Types.generated.h"

UENUM(BlueprintType)
enum class ESOTS_MusicDebugMode : uint8
{
    Off     UMETA(DisplayName = "Off"),
    Basic   UMETA(DisplayName = "Basic"),
    Verbose UMETA(DisplayName = "Verbose")
};

USTRUCT(BlueprintType)
struct SOTS_MMSS_API FSOTS_MusicTrackEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    TSoftObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    float FadeInTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    float FadeOutTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    bool bLooping = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    float VolumeMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct SOTS_MMSS_API FSOTS_MissionMusicSet
{
    GENERATED_BODY()

public:
    // TrackId -> Track data for this mission.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    TMap<FGameplayTag, FSOTS_MusicTrackEntry> Tracks;
};
