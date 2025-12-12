#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_SandboxDummyTarget.generated.h"

UCLASS(Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API ASOTS_KEMSandboxDummyTarget : public ACharacter
{
    GENERATED_BODY()

public:
    ASOTS_KEMSandboxDummyTarget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox|Pose")
    bool bForceCrouch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox|Pose")
    bool bForceStand = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox|Pose")
    FRotator ForcedRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sandbox|Tags")
    FGameplayTagContainer AdditionalTargetTags;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

private:
    void ApplyPose();
    void RefreshTags();
};
