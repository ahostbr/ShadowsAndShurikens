#include "SOTS_InteractionDriverComponent.h"
#include "SOTS_InteractionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

USOTS_InteractionDriverComponent::USOTS_InteractionDriverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    bAutoUpdate = true;
    AutoUpdateIntervalSeconds = 0.10f;
}

void USOTS_InteractionDriverComponent::BeginPlay()
{
    Super::BeginPlay();

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn)
    {
        CachedPC = Cast<APlayerController>(OwnerPawn->GetController());
    }
    else
    {
        CachedPC = Cast<APlayerController>(GetOwner());
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            Subsystem = GI->GetSubsystem<USOTS_InteractionSubsystem>();
        }
    }

    BindSubsystemEvents();

    if (bAutoUpdate && CachedPC.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AutoUpdateTimer,
            this,
            &USOTS_InteractionDriverComponent::AutoUpdateTick,
            AutoUpdateIntervalSeconds,
            true
        );
    }
}

void USOTS_InteractionDriverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoUpdateTimer);
    }

    UnbindSubsystemEvents();

    Super::EndPlay(EndPlayReason);
}

void USOTS_InteractionDriverComponent::BindSubsystemEvents()
{
    if (Subsystem.IsValid())
    {
        Subsystem->OnUIIntentPayload.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload);
    }
}

void USOTS_InteractionDriverComponent::UnbindSubsystemEvents()
{
    if (Subsystem.IsValid())
    {
        Subsystem->OnUIIntentPayload.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload);
    }
}

void USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload)
{
    if (!CachedPC.IsValid() || PlayerController != CachedPC.Get())
    {
        return;
    }

    OnUIIntentForwarded.Broadcast(Payload);
}

void USOTS_InteractionDriverComponent::AutoUpdateTick()
{
    if (!Subsystem.IsValid() || !CachedPC.IsValid())
    {
        return;
    }

    Subsystem->UpdateCandidateThrottled(CachedPC.Get());
}

void USOTS_InteractionDriverComponent::Driver_UpdateNow()
{
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        Subsystem->UpdateCandidateNow(CachedPC.Get());
    }
}

void USOTS_InteractionDriverComponent::Driver_RequestInteract()
{
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        Subsystem->RequestInteraction(CachedPC.Get());
    }
}

bool USOTS_InteractionDriverComponent::Driver_ExecuteOption(FGameplayTag OptionTag)
{
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        return Subsystem->ExecuteInteractionOption(CachedPC.Get(), OptionTag);
    }
    return false;
}
