#include "UI/SOTS_StealthSummaryHUDWidget.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_AIPerceptionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void USOTS_StealthSummaryHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode == ESOTSStealthDebugMode::Off)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        CachedBreakdown = GSM->GetCurrentStealthBreakdown();
    }

    NumEnemiesUnaware    = 0;
    NumEnemiesSuspicious = 0;
    NumEnemiesAlerted    = 0;

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (!Actor)
            {
                continue;
            }

            if (USOTS_AIPerceptionComponent* Perception =
                    Actor->FindComponentByClass<USOTS_AIPerceptionComponent>())
            {
                const ESOTS_PerceptionState State = Perception->GetCurrentPerceptionState();

                switch (State)
                {
                case ESOTS_PerceptionState::Unaware:
                    ++NumEnemiesUnaware;
                    break;

                case ESOTS_PerceptionState::SoftSuspicious:
                case ESOTS_PerceptionState::HardSuspicious:
                    ++NumEnemiesSuspicious;
                    break;

                case ESOTS_PerceptionState::Alerted:
                    ++NumEnemiesAlerted;
                    break;

                default:
                    break;
                }
            }
        }
    }

    OnStealthSummaryUpdated();
}

