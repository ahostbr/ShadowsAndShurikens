#include "SOTS_KillExecutionManagerKEMAnchorDebugWidget.h"

#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

USOTS_KillExecutionManagerKEMAnchorDebugWidget::USOTS_KillExecutionManagerKEMAnchorDebugWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USOTS_KillExecutionManagerKEMAnchorDebugWidget::SetCenterActor(AActor* Actor)
{
    CenterActor = Actor;
}

void USOTS_KillExecutionManagerKEMAnchorDebugWidget::SetSearchRadius(float Radius)
{
    SearchRadius = FMath::Max(0.f, Radius);
}

void USOTS_KillExecutionManagerKEMAnchorDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    AActor* TargetActor = CenterActor.Get();
    if (!TargetActor)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                TargetActor = Pawn;
            }
        }
    }

    if (!TargetActor)
    {
        return;
    }

    UWorld* World = TargetActor->GetWorld();
    if (!World)
    {
        return;
    }

    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (USOTS_KEMManagerSubsystem* Manager = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
        {
            Manager->GetNearbyAnchorsForDebug(TargetActor, SearchRadius, AnchorInfos);
            Manager->DrawAnchorDebugVisualization(TargetActor, SearchRadius);
        }
    }
}
