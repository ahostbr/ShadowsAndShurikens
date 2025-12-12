#pragma once

#include "CoreMinimal.h"
#include "SOTS_ExperimentalStateMachineTypes.generated.h"

class UAnimationAsset;
class UBlendProfile;

/**
 * Mirror of the experimental state machine blend stack inputs (Blueprint S_BlendStackInputs).
 */
USTRUCT(BlueprintType)
struct FS_BlendStackInputs
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    TObjectPtr<UAnimationAsset> Anim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    bool bLoop = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    float StartTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    float BlendTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    TObjectPtr<UBlendProfile> BlendProfile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendStack")
    TArray<FName> Tags;

    void Reset()
    {
        Anim = nullptr;
        bLoop = false;
        StartTime = 0.0f;
        BlendTime = 0.0f;
        BlendProfile = nullptr;
        Tags.Reset();
    }
};

/**
 * Mirror of the chooser output struct (Blueprint S_ChooserOutputs).
 */
USTRUCT(BlueprintType)
struct FS_ChooserOutputs
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    bool bUseMM = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    float MMCostLimit = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    float BlendTime = 0.0f;

    // Name of the blend profile to resolve at runtime.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    FName BlendProfileName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    TArray<FName> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
    float StartTime = 0.0f;

    void Reset()
    {
        bUseMM = false;
        MMCostLimit = 0.0f;
        BlendTime = 0.0f;
        BlendProfileName = NAME_None;
        Tags.Reset();
        StartTime = 0.0f;
    }
};
