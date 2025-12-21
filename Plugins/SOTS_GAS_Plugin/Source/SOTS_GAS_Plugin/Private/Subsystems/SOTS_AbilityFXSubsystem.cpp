#include "Subsystems/SOTS_AbilityFXSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_GAS_Plugin.h"

USOTS_AbilityFXSubsystem* USOTS_AbilityFXSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (UGameInstance* GI = World->GetGameInstance())
    {
        return GI->GetSubsystem<USOTS_AbilityFXSubsystem>();
    }

    return nullptr;
}

void USOTS_AbilityFXSubsystem::TriggerAbilityFX(FGameplayTag FXTag, FGameplayTag AbilityTag, AActor* SourceActor)
{
    const FString FXTagString = FXTag.ToString();
    const FString AbilityTagString = AbilityTag.ToString();

    if (!FXTag.IsValid())
    {
        UE_LOG(LogSOTSGAS, Warning, TEXT("[SOTS_AbilityFXSubsystem] Invalid FX tag for ability %s; skipping."), *AbilityTagString);
        return;
    }

    UWorld* World = SourceActor ? SourceActor->GetWorld() : GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTSGAS, Warning, TEXT("[SOTS_AbilityFXSubsystem] World context missing for %s (ability=%s); cannot trigger FX."), *FXTagString, *AbilityTagString);
        return;
    }

    if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
    {
        const FVector Location = SourceActor ? SourceActor->GetActorLocation() : FVector::ZeroVector;
        const FRotator Rotation = SourceActor ? SourceActor->GetActorRotation() : FRotator::ZeroRotator;
        FX->TriggerFXByTag(World, FXTag, SourceActor, nullptr, Location, Rotation);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogSOTSGAS, Verbose, TEXT("[SOTS_AbilityFXSubsystem] Trigger FX %s for ability %s (source %s)"),
            *FXTagString, *AbilityTagString, *GetNameSafe(SourceActor));
#endif
    }
    else
    {
        UE_LOG(LogSOTSGAS, Warning, TEXT("[SOTS_AbilityFXSubsystem] FX manager unavailable; skipping %s for ability %s."), *FXTagString, *AbilityTagString);
    }
}
