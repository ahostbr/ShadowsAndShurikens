#include "SOTS_InteractableComponent.h"
#include "SOTS_InteractableInterface.h"

USOTS_InteractableComponent::USOTS_InteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    MaxDistance = 250.f;
    bRequiresLineOfSight = true;
    DisplayName = FText::GetEmpty();
}

void USOTS_InteractableComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool USOTS_InteractableComponent::OwnerImplementsInteractableInterface() const
{
    const AActor* Owner = GetOwner();
    return Owner && Owner->GetClass()->ImplementsInterface(USOTS_InteractableInterface::StaticClass());
}