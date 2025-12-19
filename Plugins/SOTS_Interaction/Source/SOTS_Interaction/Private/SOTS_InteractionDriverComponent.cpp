#include "SOTS_InteractionDriverComponent.h"
#include "SOTS_InteractionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "SOTS_InputBlueprintLibrary.h"
#include "SOTS_InputRouterComponent.h"

namespace
{
    bool AreOptionsEquivalent(const FSOTS_InteractionOption& A, const FSOTS_InteractionOption& B)
    {
        return A.OptionTag == B.OptionTag
            && A.BlockedReasonTag == B.BlockedReasonTag
            && A.DisplayText.EqualTo(B.DisplayText)
            && A.RequiredPlayerTags == B.RequiredPlayerTags
            && A.BlockedPlayerTags == B.BlockedPlayerTags
            && A.RequiredTargetTags == B.RequiredTargetTags
            && A.BlockedTargetTags == B.BlockedTargetTags
            && A.bRequiresLineOfSight == B.bRequiresLineOfSight
            && A.bOverrideRequiresLineOfSight == B.bOverrideRequiresLineOfSight
            && FMath::IsNearlyEqual(A.MaxDistanceOverride, B.MaxDistanceOverride)
            && FMath::IsNearlyEqual(A.Priority, B.Priority)
            && A.MetaTags == B.MetaTags;
    }
}

USOTS_InteractionDriverComponent::USOTS_InteractionDriverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    bAutoUpdate = true;
    AutoUpdateIntervalSeconds = 0.10f;
    SelectedOptionIndex = INDEX_NONE;
    bInputRouterBound = false;
    InteractIntentTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Intent.Gameplay.Interact"), false);
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
    BindInputRouterIfAvailable();

    RefreshCachedData();
    RefreshPromptSpec();

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
    UnbindInputRouter();

    Super::EndPlay(EndPlayReason);
}

void USOTS_InteractionDriverComponent::BindSubsystemEvents()
{
    if (Subsystem.IsValid())
    {
        Subsystem->OnUIIntentPayload.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload);
        Subsystem->OnCandidateChanged.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemCandidateChanged);
        Subsystem->OnInteractionActionRequested.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemActionRequest);
    }
}

void USOTS_InteractionDriverComponent::UnbindSubsystemEvents()
{
    if (Subsystem.IsValid())
    {
        Subsystem->OnUIIntentPayload.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemUIIntentPayload);
        Subsystem->OnCandidateChanged.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemCandidateChanged);
        Subsystem->OnInteractionActionRequested.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleSubsystemActionRequest);
    }
}

void USOTS_InteractionDriverComponent::BindInputRouterIfAvailable()
{
    if (bInputRouterBound)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    USOTS_InputRouterComponent* Router = USOTS_InputBlueprintLibrary::GetRouterFromActor(OwnerActor);
    if (!Router)
    {
        Router = USOTS_InputBlueprintLibrary::EnsureRouterOnPlayerController(this, OwnerActor);
    }

    if (Router)
    {
        Router->OnInputIntent.AddDynamic(this, &USOTS_InteractionDriverComponent::HandleRouterIntentEvent);
        InputRouter = Router;
        bInputRouterBound = true;
    }
}

void USOTS_InteractionDriverComponent::UnbindInputRouter()
{
    if (!bInputRouterBound)
    {
        return;
    }

    if (InputRouter.IsValid())
    {
        InputRouter->OnInputIntent.RemoveDynamic(this, &USOTS_InteractionDriverComponent::HandleRouterIntentEvent);
    }

    InputRouter.Reset();
    bInputRouterBound = false;
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
        RefreshPromptSpec();
        return;
    }

    BroadcastOptionChangeIfNeeded(PreviousData.Options, CachedData.Options);
    RefreshPromptSpec();
}

void USOTS_InteractionDriverComponent::HandleSubsystemActionRequest(const FSOTS_InteractionActionRequest& Request)
{
    if (!CachedPC.IsValid())
    {
        return;
    }

    const AActor* CachedActor = CachedPC.IsValid() ? CachedPC.Get() : nullptr;
    const bool bMatchesController = CachedActor && Request.InstigatorActor.Get() == CachedActor;
    const bool bMatchesPawn = CachedPC.IsValid() && CachedPC->GetPawn() && Request.InstigatorActor.Get() == CachedPC->GetPawn();

    if (!bMatchesController && !bMatchesPawn)
    {
        return;
    }

    OnActionRequestForwarded.Broadcast(Request);
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
            if (!AreOptionsEquivalent(OldOptions[Index], NewOptions[Index]))
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
        RefreshCachedData();
        RefreshPromptSpec();
    }
    return Result;
}

FSOTS_InteractionResult USOTS_InteractionDriverComponent::Driver_ExecuteOption(FGameplayTag OptionTag)
{
    FSOTS_InteractionResult Result;
    if (Subsystem.IsValid() && CachedPC.IsValid())
    {
        Result = Subsystem->ExecuteInteractionOption(CachedPC.Get(), OptionTag);
        RefreshCachedData();
        RefreshPromptSpec();
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
    RefreshPromptSpec();
    return OutResult.Result == ESOTS_InteractionResultCode::Success;
}

bool USOTS_InteractionDriverComponent::HandleInputIntentTag(FGameplayTag IntentTag)
{
    if (!InteractIntentTag.IsValid() || !IntentTag.MatchesTagExact(InteractIntentTag))
    {
        return false;
    }

    FGameplayTag PreferredTag;
    if (CachedData.Options.IsValidIndex(SelectedOptionIndex))
    {
        PreferredTag = CachedData.Options[SelectedOptionIndex].OptionTag;
    }

    FSOTS_InteractionResult Result;
    FSOTS_InteractionOption ChosenOption;
    TryInteract(PreferredTag, Result, ChosenOption);
    RefreshPromptSpec();
    return true;
}

void USOTS_InteractionDriverComponent::BindToInputRouter()
{
    BindInputRouterIfAvailable();
}

bool USOTS_InteractionDriverComponent::GetCurrentPromptSpec(FSOTS_InteractionPromptSpec& OutSpec) const
{
    OutSpec = CachedPromptSpec;
    return CachedPromptSpec.bHasFocus;
}

void USOTS_InteractionDriverComponent::HandleRouterIntentEvent(const FSOTS_InputIntentEvent& Event)
{
    if (!InteractIntentTag.IsValid())
    {
        return;
    }

    if (!Event.IntentTag.MatchesTagExact(InteractIntentTag))
    {
        return;
    }

    if (Event.TriggerEvent == ETriggerEvent::Triggered || Event.TriggerEvent == ETriggerEvent::Started)
    {
        HandleInputIntentTag(Event.IntentTag);
    }
}

void USOTS_InteractionDriverComponent::RefreshPromptSpec()
{
    ApplySelectionForCurrentOptions();

    FSOTS_InteractionPromptSpec NewSpec;
    NewSpec.FocusedActor = CachedData.Context.TargetActor;
    NewSpec.bHasFocus = CachedData.Context.TargetActor.IsValid();
    NewSpec.Options = CachedData.Options;
    NewSpec.SelectedOptionIndex = SelectedOptionIndex;
    NewSpec.SuggestedVerbTag = CachedData.Context.InteractionTypeTag;

    if (NewSpec.bHasFocus && CachedData.Options.Num() > 0)
    {
        NewSpec.bShowPrompt = true;

        const FSOTS_InteractionOption* SelectedOption = CachedData.Options.IsValidIndex(SelectedOptionIndex)
            ? &CachedData.Options[SelectedOptionIndex]
            : nullptr;

        const FSOTS_InteractionOption& OptionForPrompt = SelectedOption ? *SelectedOption : CachedData.Options[0];
        NewSpec.PromptText = OptionForPrompt.DisplayText;
        NewSpec.SuggestedVerbTag = OptionForPrompt.OptionTag;
    }
    else
    {
        NewSpec.bShowPrompt = false;
        NewSpec.PromptText = FText::GetEmpty();
    }

    BroadcastPromptChanged(NewSpec);
}

void USOTS_InteractionDriverComponent::BroadcastPromptChanged(const FSOTS_InteractionPromptSpec& PromptSpec)
{
    const FSOTS_InteractionPromptSpec Previous = CachedPromptSpec;

    bool bChanged = Previous.bHasFocus != PromptSpec.bHasFocus
        || Previous.bShowPrompt != PromptSpec.bShowPrompt
        || Previous.SelectedOptionIndex != PromptSpec.SelectedOptionIndex
        || Previous.FocusedActor.Get() != PromptSpec.FocusedActor.Get()
        || !Previous.PromptText.EqualTo(PromptSpec.PromptText)
        || Previous.SuggestedVerbTag != PromptSpec.SuggestedVerbTag
        || Previous.Options.Num() != PromptSpec.Options.Num();

    if (!bChanged)
    {
        for (int32 Index = 0; Index < Previous.Options.Num(); ++Index)
        {
            if (!AreOptionsEquivalent(Previous.Options[Index], PromptSpec.Options[Index]))
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        CachedPromptSpec = PromptSpec;
        OnInteractionPromptChanged.Broadcast(CachedPromptSpec);
    }
}

int32 USOTS_InteractionDriverComponent::ChoosePreferredOptionIndex(const TArray<FSOTS_InteractionOption>& Options) const
{
    if (Options.Num() == 0)
    {
        return INDEX_NONE;
    }

    int32 BestAvailableIndex = INDEX_NONE;
    float BestAvailablePriority = 0.f;
    bool bHasAvailable = false;

    int32 BestAnyIndex = 0;
    float BestAnyPriority = Options[0].Priority;

    for (int32 Index = 0; Index < Options.Num(); ++Index)
    {
        const float Priority = Options[Index].Priority;

        if (!bHasAvailable || Priority > BestAvailablePriority)
        {
            if (!Options[Index].BlockedReasonTag.IsValid())
            {
                BestAvailableIndex = Index;
                BestAvailablePriority = Priority;
                bHasAvailable = true;
            }
        }

        if (Priority > BestAnyPriority)
        {
            BestAnyPriority = Priority;
            BestAnyIndex = Index;
        }
    }

    if (bHasAvailable)
    {
        return BestAvailableIndex;
    }

    return BestAnyIndex;
}

void USOTS_InteractionDriverComponent::ApplySelectionForCurrentOptions()
{
    const TArray<FSOTS_InteractionOption>& Options = CachedData.Options;
    if (Options.Num() == 0)
    {
        SelectedOptionIndex = INDEX_NONE;
        return;
    }

    if (Options.IsValidIndex(SelectedOptionIndex))
    {
        return;
    }

    SelectedOptionIndex = ChoosePreferredOptionIndex(Options);
}
