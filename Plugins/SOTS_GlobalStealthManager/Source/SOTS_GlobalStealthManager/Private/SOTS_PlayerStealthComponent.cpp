#include "SOTS_PlayerStealthComponent.h"

#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"

namespace
{
    static constexpr int32 TierTagCount = 4;
    static const FName TierTagNames[TierTagCount] =
    {
        TEXT("SAS.Stealth.Tier.Hidden"),
        TEXT("SAS.Stealth.Tier.Cautious"),
        TEXT("SAS.Stealth.Tier.Danger"),
        TEXT("SAS.Stealth.Tier.Compromised")
    };

    static const FName Tag_InCover(TEXT("SAS.Stealth.InCover"));
    static const FName Tag_InShadow(TEXT("SAS.Stealth.InShadow"));
    static const FName Tag_Detected(TEXT("SAS.Stealth.Detected"));
    static constexpr float DetectionThreshold = 0.8f;
}

USOTS_PlayerStealthComponent::USOTS_PlayerStealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    Weight_Light = 0.5f;
    Weight_Cover = 0.3f;
    Weight_Motion = 0.2f;
}

void USOTS_PlayerStealthComponent::SetLightLevel(float In01)
{
    State.LightLevel01 = FMath::Clamp(In01, 0.0f, 1.0f);
    State.ShadowLevel01 = 1.0f - State.LightLevel01;

    RecomputeLocalVisibility();
}

void USOTS_PlayerStealthComponent::SetCoverExposure(float In01)
{
    State.CoverExposure01 = FMath::Clamp(In01, 0.0f, 1.0f);
    RecomputeLocalVisibility();
}

void USOTS_PlayerStealthComponent::SetMovementNoise(float In01)
{
    State.MovementNoise01 = FMath::Clamp(In01, 0.0f, 1.0f);
    RecomputeLocalVisibility();
}

void USOTS_PlayerStealthComponent::SetAISuspicion(float In01)
{
    State.AISuspicion01 = FMath::Clamp(In01, 0.0f, 1.0f);

    // Forward AI suspicion directly to the global manager as well.
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        GSM->SetAISuspicion(State.AISuspicion01);
    }

    // Local visibility does not directly depend on AISuspicion, so we only
    // push the updated state to the global manager.
    PushStateToGlobalManager();
}

void USOTS_PlayerStealthComponent::RecomputeLocalVisibility()
{
    const float SumWeights = Weight_Light + Weight_Cover + Weight_Motion;
    float Local = 0.0f;

    if (SumWeights > KINDA_SMALL_NUMBER)
    {
        Local =
            (State.LightLevel01    * Weight_Light +
             State.CoverExposure01 * Weight_Cover +
             State.MovementNoise01 * Weight_Motion) / SumWeights;
    }

    State.LocalVisibility01 = FMath::Clamp(Local, 0.0f, 1.0f);

    OnLocalVisibilityUpdated.Broadcast(State.LocalVisibility01);

    PushStateToGlobalManager();
}

void USOTS_PlayerStealthComponent::PushStateToGlobalManager()
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        GSM->UpdateFromPlayer(State);
    }
}

void USOTS_PlayerStealthComponent::BeginPlay()
{
    Super::BeginPlay();

    State.StealthTier = CurrentTier;
    SetStealthTier(CurrentTier);
}

void USOTS_PlayerStealthComponent::SetStealthTier(ESOTS_StealthTier NewTier)
{
    if (CurrentTier == NewTier && CurrentTierTag.IsValid())
    {
        return;
    }

    const FGameplayTag OldTag = CurrentTierTag;
    const FGameplayTag NewTag = MakeTierTag(NewTier);

    CurrentTier = NewTier;
    State.StealthTier = NewTier;
    CurrentTierTag = NewTag;

    if (OldTag.IsValid())
    {
        UpdateTagPresence(OldTag, false);
    }

    UpdateTagPresence(NewTag, true);
    OnStealthTierChanged.Broadcast(NewTier);
}

void USOTS_PlayerStealthComponent::SetIsInCover(bool bNewInCover)
{
    if (bIsInCover == bNewInCover)
    {
        return;
    }

    bIsInCover = bNewInCover;
    UpdateTagPresence(FGameplayTag::RequestGameplayTag(Tag_InCover), bIsInCover);
}

void USOTS_PlayerStealthComponent::SetIsInShadow(bool bNewInShadow)
{
    if (bIsInShadow == bNewInShadow)
    {
        return;
    }

    bIsInShadow = bNewInShadow;
    UpdateTagPresence(FGameplayTag::RequestGameplayTag(Tag_InShadow), bIsInShadow);
}

void USOTS_PlayerStealthComponent::SetDetected(bool bNewDetected)
{
    if (bIsDetected == bNewDetected)
    {
        return;
    }

    bIsDetected = bNewDetected;
    UpdateTagPresence(FGameplayTag::RequestGameplayTag(Tag_Detected), bIsDetected);
}

void USOTS_PlayerStealthComponent::OnDetectionScoreUpdated(float NewScore)
{
    LastDetectionScore = FMath::Clamp(NewScore, 0.0f, 1.0f);
    SetDetected(LastDetectionScore >= DetectionThreshold);
}

USOTS_GameplayTagManagerSubsystem* USOTS_PlayerStealthComponent::GetTagManager() const
{
    return SOTS_GetTagSubsystem(this);
}

void USOTS_PlayerStealthComponent::UpdateTagPresence(const FGameplayTag& Tag, bool bShouldHave)
{
    if (!Tag.IsValid())
    {
        return;
    }

    if (AActor* Owner = GetOwner())
    {
        if (USOTS_GameplayTagManagerSubsystem* TagManager = GetTagManager())
        {
            if (bShouldHave)
            {
                TagManager->AddTagToActor(Owner, Tag);
            }
            else
            {
                TagManager->RemoveTagFromActor(Owner, Tag);
            }
        }
    }
}

FGameplayTag USOTS_PlayerStealthComponent::MakeTierTag(ESOTS_StealthTier Tier)
{
    const int32 Index = static_cast<int32>(Tier);
    if (Index >= 0 && Index < TierTagCount)
    {
        return FGameplayTag::RequestGameplayTag(TierTagNames[Index]);
    }

    return FGameplayTag();
}

