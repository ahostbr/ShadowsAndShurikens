#include "SOTS_GlobalStealthDebugLibrary.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_GlobalStealthTypes.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "SOTS_GlobalStealthManagerModule.h"

void USOTS_GlobalStealthDebugLibrary::RunSOTS_GSM_DebugSample(
    const UObject* WorldContextObject,
    AActor* PlayerActor,
    TSubclassOf<AActor> EnemyClass)
{
    if (!WorldContextObject)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning, TEXT("[GSM Debug] WorldContextObject is null."));
        return;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning, TEXT("[GSM Debug] No valid UWorld."));
        return;
    }

    USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject);
    if (!GSM)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning, TEXT("[GSM Debug] Global Stealth Manager subsystem not found."));
        return;
    }

    if (!PlayerActor)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning, TEXT("[GSM Debug] PlayerActor is null."));
        return;
    }

    // --- Derive noise from velocity ---
    const FVector Velocity = PlayerActor->GetVelocity();
    const float Speed = Velocity.Size();
    const float MaxSpeedForNoise = 600.0f; // basic walk/run assumption
    float NoiseLevel = FMath::Clamp(Speed / MaxSpeedForNoise, 0.0f, 1.0f);

    // --- Derive crouch/cover + light ---
    bool bInCover = false;
    float LightExposure = 0.6f;

    if (const ACharacter* Character = Cast<ACharacter>(PlayerActor))
    {
        bInCover = Character->bIsCrouched;
        LightExposure = bInCover ? 0.3f : 0.6f;
    }

    // --- Find nearest enemy of the given class ---
    float NearestDistance = 3000.0f;
    bool bHasEnemyLOS = false;

    if (*EnemyClass)
    {
        TArray<AActor*> FoundEnemies;
        UGameplayStatics::GetAllActorsOfClass(World, EnemyClass, FoundEnemies);

        float BestSqDist = TNumericLimits<float>::Max();
        AActor* BestEnemy = nullptr;

        const FVector PlayerLocation = PlayerActor->GetActorLocation();

        for (AActor* Enemy : FoundEnemies)
        {
            if (!Enemy)
            {
                continue;
            }

            const float SqDist = FVector::DistSquared(Enemy->GetActorLocation(), PlayerLocation);
            if (SqDist < BestSqDist)
            {
                BestSqDist = SqDist;
                BestEnemy = Enemy;
            }
        }

        if (BestEnemy)
        {
            NearestDistance = FMath::Sqrt(BestSqDist);

            // Simple LOS check from enemy to player.
            FHitResult Hit;
            const FVector Start = BestEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
            const FVector End = PlayerLocation + FVector(0.0f, 0.0f, 80.0f);

            FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_GSM_Debug_LOS), false);
            Params.AddIgnoredActor(PlayerActor);
            Params.AddIgnoredActor(BestEnemy);

            const bool bHit = World->LineTraceSingleByChannel(
                Hit,
                Start,
                End,
                ECC_Visibility,
                Params);

            bHasEnemyLOS = !bHit;

    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            // Visualize the LOS trace for debugging.
            const FColor DebugColor = bHasEnemyLOS ? FColor::Green : FColor::Red;
            DrawDebugLine(World, Start, End, DebugColor, false, 1.0f, 0, 1.5f);
    #endif
        }
    }

    // --- Build sample and send to GSM ---
    FSOTS_StealthSample Sample;
    Sample.TimeSeconds = World->GetTimeSeconds();
    Sample.LightExposure = LightExposure;
    Sample.DistanceToNearestEnemy = NearestDistance;
    Sample.bInEnemyLineOfSight = bHasEnemyLOS;
    Sample.NoiseLevel = NoiseLevel;
    Sample.bInCover = bInCover;

    GSM->ReportStealthSample(Sample);

    const float Score = GSM->GetCurrentStealthScore();
    const ESOTSStealthLevel Level = GSM->GetCurrentStealthLevel();

    UE_LOG(LogSOTSGlobalStealth, Log,
        TEXT("[GSM Debug] Sample sent | Speed=%.1f, Noise=%.2f, Dist=%.1f, LOS=%d, InCover=%d, Score=%.3f, Level=%d"),
        Speed,
        NoiseLevel,
        NearestDistance,
        bHasEnemyLOS ? 1 : 0,
        bInCover ? 1 : 0,
        Score,
        static_cast<int32>(Level));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Cyan,
            FString::Printf(TEXT("[GSM Debug] Score=%.3f Level=%d Dist=%.0f Speed=%.0f"),
                Score,
                static_cast<int32>(Level),
                NearestDistance,
                Speed));
    }
#endif
}