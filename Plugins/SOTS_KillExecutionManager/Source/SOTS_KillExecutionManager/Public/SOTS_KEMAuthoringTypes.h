#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEMAuthoringTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTS_KEMTargetKind : uint8
{
    Generic     UMETA(DisplayName="Generic"),
    CommonGuard UMETA(DisplayName="Common Guard"),
    EliteGuard  UMETA(DisplayName="Elite Guard"),
    Boss        UMETA(DisplayName="Boss"),
    Civilian    UMETA(DisplayName="Civilian")
};

USTRUCT(BlueprintType)
struct SOTS_KILLEXECUTIONMANAGER_API FSOTS_KEMAuthoringMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring", meta=(MultiLine="true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    ESOTS_KEMTargetKind TargetKind = ESOTS_KEMTargetKind::Generic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    bool bIsHeroExecution = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    bool bIsBossOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    bool bIsDragonOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    bool bExposedForSandbox = true;
};

USTRUCT(BlueprintType)
struct SOTS_KILLEXECUTIONMANAGER_API FSOTS_KEMCoverageCell
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    FGameplayTag ExecutionFamilyTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    FGameplayTag PositionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    ESOTS_KEMTargetKind TargetKind = ESOTS_KEMTargetKind::Generic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    int32 DefinitionCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    bool bHasAnyBossOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Coverage")
    bool bHasAnyDragonOnly = false;
};
