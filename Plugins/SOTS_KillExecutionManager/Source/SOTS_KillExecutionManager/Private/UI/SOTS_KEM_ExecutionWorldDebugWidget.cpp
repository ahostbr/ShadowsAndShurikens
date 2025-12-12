#include "UI/SOTS_KEM_ExecutionWorldDebugWidget.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_ManagerSubsystem.h"

void USOTS_KEM_ExecutionWorldDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode == ESOTSStealthDebugMode::Off || !ObservedActor)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        bHasValidRecord = false;
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    bHasValidRecord = false;

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
            {
                TArray<FSOTS_KEMDebugRecord> Records;
                KEM->GetRecentKEMDebugRecords(Records);

                for (const FSOTS_KEMDebugRecord& Record : Records)
                {
                    if (Record.Instigator == ObservedActor || Record.Target == ObservedActor)
                    {
                        CachedRecord   = Record;
                        bHasValidRecord = true;
                        break;
                    }
                }
            }
        }
    }

    OnExecutionWorldDebugUpdated();
}

