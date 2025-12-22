#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileTypes.generated.h"

inline constexpr int32 SOTS_PROFILE_SHARED_CURRENT_SNAPSHOT_VERSION = 1;

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_ProfileId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FName ProfileName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    int32 SlotIndex = 0;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_ProfileMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_ProfileId Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FDateTime LastPlayedUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    int32 TotalPlaySeconds = 0;
};

// FSOTS_CharacterStateData stores per-character runtime state that needs
// to be persisted, including StatValues which mirror USOTS_StatsComponent.
USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_CharacterStateData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TMap<FGameplayTag, float> StatValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FGameplayTagContainer MovementStateTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FGameplayTag> EquippedAbilityTags;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_SerializedItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Inventory")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Inventory")
    int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_ItemSlotBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Inventory")
    int32 SlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Inventory")
    FName ItemId = NAME_None;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_GSMProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    float GlobalAlertLevel = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FGameplayTag CurrentAlertTier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FGameplayTag> PersistentStealthFlags;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_AbilityProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FGameplayTag> GrantedAbilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TMap<FGameplayTag, int32> AbilityRanks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TMap<FGameplayTag, float> CooldownsRemaining;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_SkillTreeProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile|SkillTree")
    TArray<FGameplayTag> UnlockedSkillNodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile|SkillTree")
    int32 UnspentSkillPoints = 0;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_InventoryProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FSOTS_SerializedItem> CarriedItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FSOTS_SerializedItem> StashItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    TArray<FSOTS_ItemSlotBinding> QuickSlots;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_MissionProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    FName ActiveMissionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    TArray<FName> CompletedMissionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    TArray<FName> FailedMissionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    FName LastMissionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    float LastFinalScore = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    float LastDurationSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    bool bLastMissionCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Mission")
    bool bLastMissionFailed = false;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_MMSSProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FGameplayTag CurrentMusicRoleTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FName CurrentTrackId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    float PlaybackPositionSeconds = 0.f;
};

USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_FXProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile|FX")
    bool bBloodEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile|FX")
    bool bHighIntensityFX = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile|FX")
    bool bCameraMotionFXEnabled = true;
};

// FSOTS_ProfileSnapshot is the single source of truth for persistent SOTS
// player profile data. It aggregates slices from multiple SOTS plugins
// (stats, inventory, missions, music, FX, abilities, etc.).
// DevTools: Profile validators and debug dumps will treat this as the canonical layout.
USTRUCT(BlueprintType)
struct SOTS_PROFILESHARED_API FSOTS_ProfileSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    int32 SnapshotVersion = SOTS_PROFILE_SHARED_CURRENT_SNAPSHOT_VERSION;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_ProfileMetadata Meta;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_CharacterStateData PlayerCharacter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_GSMProfileData GSM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_AbilityProfileData Ability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_SkillTreeProfileData SkillTree;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_InventoryProfileData Inventory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_MissionProfileData Missions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_MMSSProfileData Music;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Profile")
    FSOTS_FXProfileData FX;
};
