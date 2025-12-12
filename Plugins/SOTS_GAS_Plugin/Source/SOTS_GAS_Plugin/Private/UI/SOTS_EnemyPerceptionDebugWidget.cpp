#include "UI/SOTS_EnemyPerceptionDebugWidget.h"

#include "SOTS_AIPerceptionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

void USOTS_EnemyPerceptionDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode == ESOTSStealthDebugMode::Off)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (!ObservedActor)
    {
        return;
    }

    if (USOTS_AIPerceptionComponent* Perception =
            ObservedActor->FindComponentByClass<USOTS_AIPerceptionComponent>())
    {
        CachedPerceptionState = Perception->GetCurrentPerceptionState();
        CachedSuspicion01     = Perception->GetCurrentSuspicion01();

        bool bFound = false;
        CachedTargetState = Perception->GetTargetState(nullptr /* Player will be resolved in BP */, bFound);

        // Distance to player (if any).
        CachedDistanceToPlayer = 0.0f;
        bCachedHasLineOfSight  = CachedTargetState.SightScore > 0.0f;

        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (APawn* PlayerPawn = PC->GetPawn())
                {
                    const float Dist = FVector::Dist(
                        ObservedActor->GetActorLocation(),
                        PlayerPawn->GetActorLocation());

                    CachedDistanceToPlayer = Dist;
                }
            }
        }

        OnPerceptionDebugDataUpdated();
    }
}

