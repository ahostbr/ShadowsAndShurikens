#include "SOTS_AIPerceptionLibrary.h"

#include "SOTS_AIPerceptionSubsystem.h"
#include "SOTS_AIPerceptionTypes.h"
#include "Engine/World.h"

bool USOTS_AIPerceptionLibrary::SOTS_TryReportNoise(
    UObject* WorldContextObject,
    AActor* Instigator,
    FVector Location,
    float Loudness,
    FGameplayTag NoiseTag,
    bool bLogIfFailed)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    if (USOTS_AIPerceptionSubsystem* Subsys = World->GetSubsystem<USOTS_AIPerceptionSubsystem>())
    {
        return Subsys->TryReportNoise(Instigator, Location, Loudness, NoiseTag);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogIfFailed)
    {
        UE_LOG(LogSOTS_AIPerception, Verbose, TEXT("[AIPerc/Noise] Failed to report noise (missing subsystem) Instigator=%s Loc=%s"),
            *GetNameSafe(Instigator), *Location.ToString());
    }
#endif

    return false;
}

void USOTS_AIPerceptionLibrary::SOTS_ReportNoise(
    UObject* WorldContextObject,
    AActor* Instigator,
    FVector Location,
    float Loudness,
    FGameplayTag NoiseTag)
{
    SOTS_TryReportNoise(WorldContextObject, Instigator, Location, Loudness, NoiseTag, /*bLogIfFailed=*/false);
}

