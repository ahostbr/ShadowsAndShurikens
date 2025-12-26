#pragma once

#include "CoreMinimal.h"
#include "SOTS_SaveTypes.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_SaveRequestContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    FString ProfileId;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    FString SlotId;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    bool bIsAutoSave = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    FString Reason;
};

USTRUCT(BlueprintType)
struct FSOTS_SaveParticipantStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    bool bCanSave = true;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    FString BlockReason;
};

USTRUCT(BlueprintType)
struct FSOTS_SavePayloadFragment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    FName FragmentId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Save")
    TArray<uint8> Data;
};
