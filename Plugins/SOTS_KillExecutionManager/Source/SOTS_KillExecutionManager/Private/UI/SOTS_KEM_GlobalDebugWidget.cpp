#include "UI/SOTS_KEM_GlobalDebugWidget.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_ManagerSubsystem.h"

void USOTS_KEM_GlobalDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode == ESOTSStealthDebugMode::Off)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
            {
                KEM->GetRecentKEMDebugRecords(CachedRecords);
            }
        }
    }

    OnKEMDebugRecordsUpdated();
}

