#include "SOTS_StealthFXController.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "Engine/World.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"

void USOTS_StealthFXController::Initialize(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
    {
        GSM->OnStealthStateChanged.AddUObject(this, &USOTS_StealthFXController::HandleStealthStateChanged);
    }
}

void USOTS_StealthFXController::HandleStealthStateChanged(const FSOTS_PlayerStealthState& NewState)
{
    // Evaluate curves (if any) and translate into FX parameters.
    const float ShadowT = NewState.ShadowLevel01;
    const float DangerT = NewState.GlobalStealthScore01;

    float AmbientFX = 0.0f;
    float DangerFX  = 0.0f;

    if (Curve_LightToAmbientFX)
    {
        AmbientFX = Curve_LightToAmbientFX->GetFloatValue(ShadowT);
    }

    if (Curve_StealthToDangerFX)
    {
        DangerFX = Curve_StealthToDangerFX->GetFloatValue(DangerT);
    }

    // Use existing FX manager subsystem to drive cues / parameters.
    if (USOTS_FXManagerSubsystem* FXMgr = USOTS_FXManagerSubsystem::Get())
    {
        // This is intentionally kept lightweight: we evaluate curves for
        // designers to optionally use, and fire a tier-based FX tag when the
        // stealth tier changes. The actual Niagara/SFX spawning is handled
        // by game-side Blueprint listeners bound to OnFXTriggered.
        UE_LOG(LogTemp, VeryVerbose,
            TEXT("[StealthFX] AmbientFX=%.2f DangerFX=%.2f (Shadow=%.2f Stealth=%.2f)"),
            AmbientFX,
            DangerFX,
            ShadowT,
            DangerT);

        if (LastStealthTier != NewState.StealthTier)
        {
            LastStealthTier = NewState.StealthTier;

            const int32 TierIndex = static_cast<int32>(LastStealthTier);

            if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
            {
                const FString TagString = FString::Printf(TEXT("SOTS.Stealth.FX.Tier.%d"), TierIndex);
                const FGameplayTag FXTag = TagSubsystem->GetTagByName(FName(*TagString));

                if (FXTag.IsValid())
                {
                    FXMgr->TriggerFXByTag(
                        this,
                        FXTag,
                        nullptr,
                        nullptr,
                        FVector::ZeroVector,
                        FRotator::ZeroRotator);
                }
            }
        }
    }
}
