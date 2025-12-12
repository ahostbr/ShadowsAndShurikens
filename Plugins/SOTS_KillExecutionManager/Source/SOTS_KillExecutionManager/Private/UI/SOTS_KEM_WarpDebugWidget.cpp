#include "UI/SOTS_KEM_WarpDebugWidget.h"

#include "SOTS_ExecutionHelperActor.h"
#include "SOTS_GAS_DebugLibrary.h"

void USOTS_KEM_WarpDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    CurrentDebugMode = USOTS_GAS_DebugLibrary::GetStealthDebugMode();

    if (CurrentDebugMode != ESOTSStealthDebugMode::Advanced || !ObservedHelper)
    {
        // Warp debug is only meaningful in Advanced mode and when a helper exists.
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    CachedWarpResult     = ObservedHelper->GetWarpResult();
    CachedSpawnTransform = ObservedHelper->GetSpawnTransform();

    OnWarpDebugUpdated();
}
