#include "UI/SOTS_DragonStealthDebugWidget.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"

void USOTS_DragonStealthDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
        CachedStealthState = GSM->GetStealthState();
        CachedBreakdown    = GSM->GetCurrentStealthBreakdown();
    }

    OnStealthDebugDataUpdated();
}

