#include "SOTS_KEMExecutionAnchor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

ASOTS_KEMExecutionAnchor::ASOTS_KEMExecutionAnchor()
{
    PrimaryActorTick.bCanEverTick = false;

    AnchorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorRoot"));
    SetRootComponent(AnchorRoot);

    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    ArrowComponent->SetupAttachment(RootComponent);
    ArrowComponent->ArrowSize = 1.25f;
    ArrowComponent->ArrowColor = FColor(204, 51, 51);
#ifdef WITH_EDITOR
    ArrowComponent->SetUsingAbsoluteScale(true);
#endif
}
