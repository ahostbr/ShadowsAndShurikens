#include "SOTS_BodyDragPlayerComponent.h"
#include "SOTS_BodyDragTargetComponent.h"
#include "SOTS_BodyDragConfigDA.h"
#include "SOTS_BodyDragTypes.h"
#include "SOTS_TagAccessHelpers.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

USOTS_BodyDragPlayerComponent::USOTS_BodyDragPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    DragState = ESOTS_BodyDragState::None;
    CurrentBodyActor = nullptr;
    CurrentBodyTargetComp = nullptr;
    ConfigDA = nullptr;
    CachedOriginalMaxWalkSpeed = 0.f;
    CurrentBodyType = ESOTS_BodyDragBodyType::KnockedOut;
}

bool USOTS_BodyDragPlayerComponent::IsDragging() const
{
    return DragState == ESOTS_BodyDragState::Dragging;
}

ESOTS_BodyDragState USOTS_BodyDragPlayerComponent::GetDragState() const
{
    return DragState;
}

AActor* USOTS_BodyDragPlayerComponent::GetCurrentBody() const
{
    return CurrentBodyActor;
}

const FSOTS_BodyDragConfig* USOTS_BodyDragPlayerComponent::GetConfig() const
{
    return ConfigDA ? &ConfigDA->Config : nullptr;
}

UCharacterMovementComponent* USOTS_BodyDragPlayerComponent::ResolveMovementComponent() const
{
    if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
    {
        return CharacterOwner->GetCharacterMovement();
    }
    return nullptr;
}

bool USOTS_BodyDragPlayerComponent::TryBeginDrag(AActor* BodyActor)
{
    if (!BodyActor || DragState != ESOTS_BodyDragState::None)
    {
        return false;
    }

    const FSOTS_BodyDragConfig* Config = GetConfig();
    if (!Config)
    {
        return false;
    }

    ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    if (!CharacterOwner || !CharacterOwner->GetMesh())
    {
        return false;
    }

    USOTS_BodyDragTargetComponent* TargetComp = BodyActor->FindComponentByClass<USOTS_BodyDragTargetComponent>();
    if (!TargetComp || !TargetComp->CanBeDragged())
    {
        return false;
    }

    CurrentBodyActor = BodyActor;
    CurrentBodyTargetComp = TargetComp;
    CurrentBodyType = TargetComp->BodyType;
    DragState = ESOTS_BodyDragState::Entering;

    // Walk speed override
    if (UCharacterMovementComponent* MoveComp = ResolveMovementComponent())
    {
        CachedOriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
        MoveComp->MaxWalkSpeed = Config->DragWalkSpeed;
    }

    TargetComp->BeginDragged();

    if (Config->bAttachOnStart && CharacterOwner->GetMesh())
    {
        BodyActor->AttachToComponent(
            CharacterOwner->GetMesh(),
            FAttachmentTransformRules::SnapToTargetIncludingScale,
            Config->AttachSocketName
        );
    }

    const FSOTS_BodyDragMontageSet& MontageSet = (CurrentBodyType == ESOTS_BodyDragBodyType::Dead) ? Config->Dead : Config->KO;
    UAnimMontage* StartMontage = MontageSet.StartDrag;

    if (UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance())
    {
        if (StartMontage)
        {
            float PlayResult = AnimInstance->Montage_Play(StartMontage);
            if (PlayResult > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &USOTS_BodyDragPlayerComponent::OnStartMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, StartMontage);
                return true;
            }
        }
    }

    // If no montage or failed to play, advance immediately.
    OnStartMontageEnded(StartMontage, true);
    return true;
}

bool USOTS_BodyDragPlayerComponent::TryDropBody()
{
    if (DragState != ESOTS_BodyDragState::Dragging)
    {
        return false;
    }

    const FSOTS_BodyDragConfig* Config = GetConfig();
    ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

    DragState = ESOTS_BodyDragState::Exiting;

    // If body/target destroyed or missing state, finalize immediately.
    if (!Config || !CharacterOwner || !CharacterOwner->GetMesh() || !CurrentBodyActor || !CurrentBodyTargetComp)
    {
        FinalizeDrop();
        return true;
    }

    const FSOTS_BodyDragMontageSet& MontageSet = (CurrentBodyType == ESOTS_BodyDragBodyType::Dead) ? Config->Dead : Config->KO;
    UAnimMontage* StopMontage = MontageSet.StopDrag;

    if (UAnimInstance* AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance())
    {
        if (StopMontage)
        {
            float PlayResult = AnimInstance->Montage_Play(StopMontage);
            if (PlayResult > 0.f)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &USOTS_BodyDragPlayerComponent::OnStopMontageEnded);
                AnimInstance->Montage_SetEndDelegate(EndDelegate, StopMontage);
                return true;
            }
        }
    }

    // If no montage or failed to play, finalize immediately.
    OnStopMontageEnded(StopMontage, true);
    return true;
}

void USOTS_BodyDragPlayerComponent::OnStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    (void)Montage;
    (void)bInterrupted;

    DragState = ESOTS_BodyDragState::Dragging;
    ApplyDraggingTags();

    DebugDrawDragging();
}

void USOTS_BodyDragPlayerComponent::OnStopMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    (void)Montage;
    (void)bInterrupted;
    FinalizeDrop();
}

void USOTS_BodyDragPlayerComponent::ApplyDraggingTags()
{
    const FSOTS_BodyDragConfig* Config = GetConfig();
    if (!Config || !CurrentBodyActor)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(this))
    {
        if (Config->DraggingStateTagName != NAME_None)
        {
            Manager->AddTagToActorByName(GetOwner(), Config->DraggingStateTagName);
        }

        const FName BodyTag = (CurrentBodyType == ESOTS_BodyDragBodyType::Dead) ? Config->DraggingDeadTagName : Config->DraggingKOTagName;
        if (BodyTag != NAME_None)
        {
            Manager->AddTagToActorByName(GetOwner(), BodyTag);
        }
    }
}

void USOTS_BodyDragPlayerComponent::RemoveDraggingTags()
{
    const FSOTS_BodyDragConfig* Config = GetConfig();
    if (!Config)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(this))
    {
        if (Config->DraggingStateTagName != NAME_None)
        {
            Manager->RemoveTagFromActorByName(GetOwner(), Config->DraggingStateTagName);
        }

        const FName BodyTag = (CurrentBodyType == ESOTS_BodyDragBodyType::Dead) ? Config->DraggingDeadTagName : Config->DraggingKOTagName;
        if (BodyTag != NAME_None)
        {
            Manager->RemoveTagFromActorByName(GetOwner(), BodyTag);
        }
    }
}

void USOTS_BodyDragPlayerComponent::FinalizeDrop()
{
    const FSOTS_BodyDragConfig* Config = GetConfig();
    if (CurrentBodyActor)
    {
        CurrentBodyActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }

    if (CurrentBodyTargetComp)
    {
        bool bDropToPhysics = false;
        if (Config)
        {
            bDropToPhysics = (CurrentBodyType == ESOTS_BodyDragBodyType::Dead)
                ? Config->bReenablePhysicsOnDrop_Dead
                : Config->bReenablePhysicsOnDrop_KO;
        }
        CurrentBodyTargetComp->EndDragged(bDropToPhysics);
    }

    if (UCharacterMovementComponent* MoveComp = ResolveMovementComponent())
    {
        MoveComp->MaxWalkSpeed = CachedOriginalMaxWalkSpeed;
    }

    if (Config)
    {
        RemoveDraggingTags();
    }

    DebugDrawDragging();

    CurrentBodyActor = nullptr;
    CurrentBodyTargetComp = nullptr;
    DragState = ESOTS_BodyDragState::None;
}

void USOTS_BodyDragPlayerComponent::DebugDrawDragging() const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    static const auto CVarDebugBodyDrag = IConsoleManager::Get().FindConsoleVariable(TEXT("SOTS.BodyDrag.Debug"));
    const bool bDebugEnabled = CVarDebugBodyDrag && CVarDebugBodyDrag->GetInt() != 0;
    if (!bDebugEnabled || !IsDragging() || !CurrentBodyActor)
    {
        return;
    }

    if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
    {
        const FVector Start = CharacterOwner->GetActorLocation();
        const FVector End = CurrentBodyActor->GetActorLocation();
        DrawDebugLine(CharacterOwner->GetWorld(), Start, End, FColor::Cyan, false, 2.0f, 0, 2.0f);

        const FString DebugText = FString::Printf(
            TEXT("BodyDrag [%s] -> %s"),
            *UEnum::GetValueAsString(DragState),
            *UEnum::GetValueAsString(CurrentBodyType)
        );
        DrawDebugString(CharacterOwner->GetWorld(), End + FVector(0, 0, 50.f), DebugText, nullptr, FColor::Cyan, 2.0f, false);
    }
#endif
}
