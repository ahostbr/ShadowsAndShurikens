#include "Subsystems/SOTS_AbilityFXSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogTemp, Verbose, TEXT("[SOTS_AbilityFXSubsystem] Trigger FX %s for ability %s (source %s)"),
        *FXTag.ToString(), *AbilityTag.ToString(), *GetNameSafe(SourceActor));
#endif
    // TODO: hook into real FX manager once available.
}
