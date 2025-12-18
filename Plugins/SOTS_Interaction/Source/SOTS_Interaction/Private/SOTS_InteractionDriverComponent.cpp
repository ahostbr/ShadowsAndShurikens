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

    RefreshCachedData();

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
        Subsystem->OnCandidateChanged.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemCandidateChanged);
    }
}

void USOTS_InteractionDriverComponent::UnbindSubsystemEvents()
{
    if (Subsystem.IsValid())
    {
        Subsystem->OnUIIntentPayload.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload);
        Subsystem->OnCandidateChanged.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemCandidateChanged);
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

void USOTS_InteractionDriverComponent::HandleSubsystemCandidateChanged(APlayerController* PlayerController, const FSOTS_InteractionContext& NewCandidate)
{
    if (!CachedPC.IsValid() || PlayerController != CachedPC.Get())
    {
        return;
    }

    const FSOTS_InteractionData PreviousData = CachedData;
    AActor* OldActor = PreviousData.Context.TargetActor.Get();

    RefreshCachedData();

    AActor* NewActor = CachedData.Context.TargetActor.Get();

    if (OldActor != NewActor)
    {
        OnFocusChanged.Broadcast(OldActor, NewActor);
        OnOptionsChanged.Broadcast(NewActor, CachedData.Options);
        return;
    }

    BroadcastOptionChangeIfNeeded(PreviousData.Options, CachedData.Options);
}

void USOTS_InteractionDriverComponent::RefreshCachedData()
{
    if (!Subsystem.IsValid() || !CachedPC.IsValid())
    {
        CachedData = FSOTS_InteractionData();
        return;
    }

    FSOTS_InteractionData NewData;
    if (Subsystem->GetCurrentInteractionData(CachedPC.Get(), NewData))
    {
        CachedData = NewData;
    }
    else
    {
        CachedData = FSOTS_InteractionData();
    }
}

void USOTS_InteractionDriverComponent::BroadcastOptionChangeIfNeeded(const TArray<FSOTS_InteractionOption>& OldOptions, const TArray<FSOTS_InteractionOption>& NewOptions)
{
    bool bChanged = OldOptions.Num() != NewOptions.Num();

    if (!bChanged)
    {
        for (int32 Index = 0; Index < OldOptions.Num(); ++Index)
        {
            if (OldOptions[Index].OptionTag != NewOptions[Index].OptionTag || OldOptions[Index].BlockedReasonTag != NewOptions[Index].BlockedReasonTag)
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        OnOptionsChanged.Broadcast(CachedData.Context.TargetActor.Get(), NewOptions);
    }
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

void USOTS_InteractionDriverComponent::ForceRefreshFocus()
{
    Driver_UpdateNow();
}

FSOTS_InteractionResult USOTS_InteractionDriverComponent::Driver_RequestInteract()
{
    FSOTS_InteractionResult Result;
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        Result = Subsystem->RequestInteraction(CachedPC.Get());
    }
    return Result;
}

FSOTS_InteractionResult USOTS_InteractionDriverComponent::Driver_ExecuteOption(FGameplayTag OptionTag)
{
    FSOTS_InteractionResult Result;
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        Result = Subsystem->ExecuteInteractionOption(CachedPC.Get(), OptionTag);
    }
    return Result;
}

AActor* USOTS_InteractionDriverComponent::GetFocusedActor() const
{
    return CachedData.Context.TargetActor.Get();
}

float USOTS_InteractionDriverComponent::GetFocusedScore() const
{
    return CachedData.Context.Score;
}

bool USOTS_InteractionDriverComponent::GetFocusedOptions(TArray<FSOTS_InteractionOption>& OutOptions) const
{
    OutOptions = CachedData.Options;
    return CachedData.Context.TargetActor.IsValid();
}

bool USOTS_InteractionDriverComponent::TryInteract(FGameplayTag OptionTag, FSOTS_InteractionResult& OutResult, FSOTS_InteractionOption& OutChosenOption)
{
    OutResult = FSOTS_InteractionResult();
    OutChosenOption = FSOTS_InteractionOption();

    if (!Subsystem.IsValid() || !CachedPC.IsValid())
    {
        return false;
    }

    RefreshCachedData();

    if (!CachedData.Context.TargetActor.IsValid())
    {
        OutResult = Subsystem->RequestInteraction(CachedPC.Get());
        return false;
    }

    const FSOTS_InteractionOption* FoundOpt = nullptr;
    FGameplayTag UseTag = OptionTag;

    for (const FSOTS_InteractionOption& Opt : CachedData.Options)
    {
        const bool bTagMatches = OptionTag.IsValid() ? (Opt.OptionTag == OptionTag) : !Opt.BlockedReasonTag.IsValid();
        if (bTagMatches)
        {
            FoundOpt = &Opt;
            UseTag = Opt.OptionTag;
            break;
        }
    }

    if (!FoundOpt && CachedData.Options.Num() > 0)
    {
        FoundOpt = &CachedData.Options[0];
        UseTag = FoundOpt->OptionTag;
    }

    if (FoundOpt)
    {
        OutChosenOption = *FoundOpt;
    }

    OnInteractRequested.Broadcast(CachedData.Context.TargetActor.Get(), UseTag);

    if (UseTag.IsValid())
    {
        OutResult = Subsystem->ExecuteInteractionOption(CachedPC.Get(), UseTag);
    }
    else
    {
        OutResult = Subsystem->RequestInteraction(CachedPC.Get());
    }

    RefreshCachedData();
    return OutResult.Result == ESOTS_InteractionResultCode::Success;
}
