#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "SOTS_UIModalResultTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTS_UIModalResult : uint8
{
	Confirmed,
	Canceled
};

USTRUCT(BlueprintType)
struct F_SOTS_UIModalResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Modal")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Modal")
	ESOTS_UIModalResult Result = ESOTS_UIModalResult::Canceled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Modal")
	FInstancedStruct ResultData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Modal")
	FGameplayTag ActionTag;
};
