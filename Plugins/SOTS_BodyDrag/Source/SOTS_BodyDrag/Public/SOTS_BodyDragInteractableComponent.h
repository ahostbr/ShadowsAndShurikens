#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_InteractableInterface.h"
#include "SOTS_BodyDragInteractableComponent.generated.h"

class USOTS_BodyDragTargetComponent;
class USOTS_BodyDragPlayerComponent;

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class USOTS_BodyDragInteractableComponent : public UActorComponent, public ISOTS_InteractableInterface
{
    GENERATED_BODY()

public:
    USOTS_BodyDragInteractableComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag|Interaction")
    FName StartDragOptionTagName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag|Interaction")
    FName DropDragOptionTagName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag|Interaction")
    FText DragText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag|Interaction")
    FText DropText;

    // ISOTS_InteractableInterface
    virtual bool CanInteract_Implementation(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReasonTag) const override;
    virtual void GetInteractionOptions_Implementation(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const override;
    virtual void ExecuteInteraction_Implementation(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag) override;

private:
    USOTS_BodyDragTargetComponent* ResolveBodyTarget() const;
    USOTS_BodyDragPlayerComponent* ResolvePlayerComponent(const FSOTS_InteractionContext& Context) const;
};
