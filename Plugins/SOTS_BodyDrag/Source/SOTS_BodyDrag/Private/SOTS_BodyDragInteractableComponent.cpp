#include "SOTS_BodyDragInteractableComponent.h"
#include "SOTS_BodyDragTargetComponent.h"
#include "SOTS_BodyDragPlayerComponent.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"
#include "GameFramework/Actor.h"

USOTS_BodyDragInteractableComponent::USOTS_BodyDragInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    StartDragOptionTagName = TEXT("Interaction.Verb.DragStart");
    DropDragOptionTagName = TEXT("Interaction.Verb.DragStop");
    DragText = FText::FromString(TEXT("Drag Body"));
    DropText = FText::FromString(TEXT("Drop Body"));
}

USOTS_BodyDragTargetComponent* USOTS_BodyDragInteractableComponent::ResolveBodyTarget() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<USOTS_BodyDragTargetComponent>() : nullptr;
}

USOTS_BodyDragPlayerComponent* USOTS_BodyDragInteractableComponent::ResolvePlayerComponent(const FSOTS_InteractionContext& Context) const
{
    if (APawn* Pawn = Context.PlayerPawn.Get())
    {
        if (USOTS_BodyDragPlayerComponent* Comp = Pawn->FindComponentByClass<USOTS_BodyDragPlayerComponent>())
        {
            return Comp;
        }
    }

    if (APlayerController* PC = Context.PlayerController.Get())
    {
        return PC->FindComponentByClass<USOTS_BodyDragPlayerComponent>();
    }

    return nullptr;
}

bool USOTS_BodyDragInteractableComponent::CanInteract_Implementation(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReasonTag) const
{
    OutFailReasonTag = FGameplayTag();

    USOTS_BodyDragTargetComponent* BodyTarget = ResolveBodyTarget();
    if (!BodyTarget)
    {
        return false;
    }

    USOTS_BodyDragPlayerComponent* PlayerComp = ResolvePlayerComponent(Context);
    if (!PlayerComp)
    {
        return false;
    }

    if (PlayerComp->IsDragging() && PlayerComp->GetCurrentBody() != GetOwner())
    {
        return false;
    }

    if (!BodyTarget->CanBeDragged())
    {
        return false;
    }

    return true;
}

void USOTS_BodyDragInteractableComponent::GetInteractionOptions_Implementation(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const
{
    OutOptions.Reset();

    USOTS_BodyDragTargetComponent* BodyTarget = ResolveBodyTarget();
    if (!BodyTarget)
    {
        return;
    }

    USOTS_BodyDragPlayerComponent* PlayerComp = ResolvePlayerComponent(Context);
    if (!PlayerComp)
    {
        return;
    }

    USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(this);
    const FGameplayTag StartTag = Manager ? Manager->GetTagByName(StartDragOptionTagName) : FGameplayTag();
    const FGameplayTag DropTag = Manager ? Manager->GetTagByName(DropDragOptionTagName) : FGameplayTag();

    if (!PlayerComp->IsDragging())
    {
        if (StartTag.IsValid())
        {
            FSOTS_InteractionOption Opt;
            Opt.OptionTag = StartTag;
            Opt.DisplayText = DragText;
            OutOptions.Add(Opt);
        }
        return;
    }

    if (PlayerComp->GetCurrentBody() == GetOwner())
    {
        if (DropTag.IsValid())
        {
            FSOTS_InteractionOption Opt;
            Opt.OptionTag = DropTag;
            Opt.DisplayText = DropText;
            OutOptions.Add(Opt);
        }
    }
}

void USOTS_BodyDragInteractableComponent::ExecuteInteraction_Implementation(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag)
{
    USOTS_BodyDragPlayerComponent* PlayerComp = ResolvePlayerComponent(Context);
    if (!PlayerComp)
    {
        return;
    }

    USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(this);
    const FGameplayTag StartTag = Manager ? Manager->GetTagByName(StartDragOptionTagName) : FGameplayTag();
    const FGameplayTag DropTag = Manager ? Manager->GetTagByName(DropDragOptionTagName) : FGameplayTag();

    const FName OptionName = OptionTag.GetTagName();
    const bool bIsStart = (StartTag.IsValid() && OptionTag == StartTag) || OptionName == StartDragOptionTagName;
    const bool bIsDrop  = (DropTag.IsValid() && OptionTag == DropTag)   || OptionName == DropDragOptionTagName;

    if (bIsStart)
    {
        PlayerComp->TryBeginDrag(GetOwner());
    }
    else if (bIsDrop)
    {
        PlayerComp->TryDropBody();
    }
}
