#include "SOTS_AIPerceptionLibrary.h"

#include "SOTS_AIPerceptionSubsystem.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_UDSBridgeBlueprintLib.h"
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

bool USOTS_AIPerceptionLibrary::SOTS_TryReportDamageStimulus(
    UObject* WorldContextObject,
    AActor* VictimActor,
    AActor* InstigatorActor,
    float DamageAmount,
    FGameplayTag DamageTag,
    FVector Location,
    bool bHasLocation,
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
        return Subsys->TryReportDamageStimulus(VictimActor, InstigatorActor, DamageAmount, DamageTag, Location, bHasLocation);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogIfFailed)
    {
        UE_LOG(LogSOTS_AIPerception, Verbose, TEXT("[AIPerc/Damage] Failed to report damage stimulus (missing subsystem) Victim=%s Instigator=%s LocValid=%s"),
            *GetNameSafe(VictimActor),
            *GetNameSafe(InstigatorActor),
            bHasLocation ? TEXT("true") : TEXT("false"));
    }
#endif

    return false;
}

bool USOTS_AIPerceptionLibrary::SOTS_GetRecentUDSBreadcrumbs(
    UObject* WorldContextObject,
    int32 MaxCount,
    TArray<FSOTS_PerceivedBreadcrumb>& OutBreadcrumbs,
    bool bLogIfFailed)
{
    OutBreadcrumbs.Reset();

    TArray<FSOTS_UDSBreadcrumb> BridgeCrumbs;
    const bool bFound = USOTS_UDSBridgeBlueprintLib::GetRecentUDSBreadcrumbs(WorldContextObject, MaxCount, BridgeCrumbs);

    for (const FSOTS_UDSBreadcrumb& BridgeCrumb : BridgeCrumbs)
    {
        FSOTS_PerceivedBreadcrumb Perceived;
        Perceived.Location = BridgeCrumb.Location;
        Perceived.TimestampSeconds = BridgeCrumb.TimestampSeconds;
        Perceived.WorldName = BridgeCrumb.WorldName;
        OutBreadcrumbs.Add(Perceived);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bFound && bLogIfFailed)
    {
        UE_LOG(LogSOTS_AIPerception, Verbose, TEXT("[AIPerc/UDS] Breadcrumb pull failed (bridge missing or disabled)."));
    }
#endif

    return bFound && OutBreadcrumbs.Num() > 0;
}

