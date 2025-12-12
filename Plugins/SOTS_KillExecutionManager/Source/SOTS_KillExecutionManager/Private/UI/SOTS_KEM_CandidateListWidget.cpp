#include "UI/SOTS_KEM_CandidateListWidget.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_ManagerSubsystem.h"

void USOTS_KEM_CandidateListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode != ESOTSStealthDebugMode::Advanced)
    {
        // Only show the candidate list in Advanced mode.
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
                KEM->GetLastKEMCandidateDebug(CachedCandidates);
            }
        }
    }

    OnCandidatesUpdated();
}

