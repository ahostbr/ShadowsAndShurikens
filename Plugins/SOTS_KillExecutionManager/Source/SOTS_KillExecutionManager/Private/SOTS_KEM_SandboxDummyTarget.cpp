#include "SOTS_KEM_SandboxDummyTarget.h"

ASOTS_KEMSandboxDummyTarget::ASOTS_KEMSandboxDummyTarget()
{
    AutoPossessAI = EAutoPossessAI::Disabled;
    bUseControllerRotationYaw = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
}

void ASOTS_KEMSandboxDummyTarget::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyPose();
    RefreshTags();
}

void ASOTS_KEMSandboxDummyTarget::BeginPlay()
{
    Super::BeginPlay();
    ApplyPose();
    RefreshTags();
}

void ASOTS_KEMSandboxDummyTarget::ApplyPose()
{
    if (bForceCrouch)
    {
        Crouch();
    }
    else if (bForceStand)
    {
        UnCrouch();
    }

    SetActorRotation(ForcedRotation);
}

void ASOTS_KEMSandboxDummyTarget::RefreshTags()
{
    for (const FGameplayTag& Tag : AdditionalTargetTags)
    {
        if (!Tag.IsValid())
        {
            continue;
        }

        const FName TagName = Tag.GetTagName();
        if (!Tags.Contains(TagName))
        {
            Tags.Add(TagName);
        }
    }
}
