#include "SOTS_MissionDirectorSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_MissionDirectorModule.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_ProfileSubsystem.h"
#include "SOTS_TagAccessHelpers.h"
#include "SOTS_UIRouterSubsystem.h"
#include "SOTS_ShaderWarmupSubsystem.h"
#include "SOTS_UIPayloadTypes.h"
#include "Misc/PackageName.h"

USOTS_MissionDirectorSubsystem::USOTS_MissionDirectorSubsystem()
    : bMissionActive(false)
    , MissionStartTimeSeconds(0.0f)
    , CurrentScore(0.0f)
    , PrimaryObjectivesCompleted(0)
    , OptionalObjectivesCompleted(0)
    , ActiveMission(nullptr)
    , MissionState(ESOTS_MissionState::NotStarted)
    , CachedStealthSubsystem(nullptr)
{
}

void USOTS_MissionDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    MissionState = ESOTS_MissionState::NotStarted;
    ActiveMission = nullptr;
    ObjectiveCompletion.Reset();
    ObjectiveFailed.Reset();
    CurrentOutcomeTag = FGameplayTag();
    StealthScore = 0;
    AlertsTriggered = 0;
    ResetConditionTracking();
    ActiveMissionIdForProfile = NAME_None;
    CompletedMissionIds.Reset();
    FailedMissionIds.Reset();
    LastMissionIdForProfile = NAME_None;
    LastFinalScoreForProfile = 0.0f;
    LastDurationSecondsForProfile = 0.0f;
    bLastMissionCompletedForProfile = false;
    bLastMissionFailedForProfile = false;
    MilestoneHistory.Reset();
    bLoggedMissingStatsOnce = false;
    bLoggedMissingUIRouterOnce = false;

    ResetConditionTracking();

    CachedStealthSubsystem = USOTS_GlobalStealthManagerSubsystem::Get(this);
    if (CachedStealthSubsystem)
    {
        CachedStealthSubsystem->OnStealthLevelChanged.AddDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleStealthLevelChanged);

        CachedStealthSubsystem->OnAIAwarenessStateChanged.AddDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleAIAwarenessStateChanged);

        CachedStealthSubsystem->OnGlobalAlertnessChanged.AddDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleGlobalAlertnessChanged);
    }

    // Subscribe to KillExecutionManager execution events for KEM-driven objectives.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
        {
            KEM->OnExecutionEvent.AddDynamic(this, &USOTS_MissionDirectorSubsystem::HandleExecutionEvent);
        }

        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->RegisterProvider(this, 0);
        }
    }
}

void USOTS_MissionDirectorSubsystem::Deinitialize()
{
    CancelWarmupTravel();

    if (CachedStealthSubsystem)
    {
        CachedStealthSubsystem->OnStealthLevelChanged.RemoveDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleStealthLevelChanged);

        CachedStealthSubsystem->OnAIAwarenessStateChanged.RemoveDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleAIAwarenessStateChanged);

        CachedStealthSubsystem->OnGlobalAlertnessChanged.RemoveDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleGlobalAlertnessChanged);
        CachedStealthSubsystem = nullptr;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
        {
            KEM->OnExecutionEvent.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleExecutionEvent);
        }

        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->UnregisterProvider(this);
        }
    }

    Super::Deinitialize();
}

USOTS_MissionDirectorSubsystem* USOTS_MissionDirectorSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<USOTS_MissionDirectorSubsystem>();
}

void USOTS_MissionDirectorSubsystem::StartMissionRun(FName MissionId, FGameplayTag InDifficultyTag)
{
    UWorld* World = nullptr;
    if (UGameInstance* GI = GetGameInstance())
    {
        World = GI->GetWorld();
    }

    float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

    bMissionActive = true;
    CurrentMissionId = MissionId;
    ActiveMissionIdForProfile = MissionId;
    CurrentDifficultyTag = InDifficultyTag;
    MissionStartTimeSeconds = CurrentTime;
    CurrentScore = 0.0f;
    PrimaryObjectivesCompleted = 0;
    OptionalObjectivesCompleted = 0;
    EventLog.Reset();

    UE_LOG(LogSOTSMissionDirector, Log, TEXT("Mission run started: %s"), *CurrentMissionId.ToString());
}

void USOTS_MissionDirectorSubsystem::RequestMissionTravelWithWarmup(FName TargetLevelPackageName)
{
    if (bWarmupTravelPending)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("Mission travel warmup already pending; ignoring new request."));
        return;
    }

    if (TargetLevelPackageName.IsNone())
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("Mission travel warmup requested with empty target level."));
        return;
    }

    USOTS_ShaderWarmupSubsystem* WarmupSubsystem = nullptr;
    if (UGameInstance* GI = GetGameInstance())
    {
        WarmupSubsystem = GI->GetSubsystem<USOTS_ShaderWarmupSubsystem>();
    }

    if (!WarmupSubsystem)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("Warmup subsystem missing; traveling immediately."));
        const FString MapName = FPackageName::GetShortName(TargetLevelPackageName.ToString());
        UGameplayStatics::OpenLevel(this, FName(*MapName));
        return;
    }

    F_SOTS_ShaderWarmupRequest Request;
    Request.TargetLevelPackageName = TargetLevelPackageName;
    Request.ScreenWidgetId = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Screen.Loading.ShaderWarmup")), false);
    Request.bUseMoviePlayerDuringMapLoad = true;
    Request.bFreezeWithGlobalTimeDilation = true;
    Request.FrozenTimeDilation = 5.0f;
    Request.SourceMode = ESOTS_ShaderWarmupSourceMode::UseLoadListDA;

    WarmupSubsystem->OnReadyToTravel.AddDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel);
    WarmupSubsystem->OnCancelled.AddDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupCancelled);
    bWarmupTravelPending = true;
    PendingWarmupTargetLevelPackage = TargetLevelPackageName;
    ActiveWarmupSubsystem = WarmupSubsystem;

    if (!WarmupSubsystem->BeginWarmup(Request))
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("Warmup BeginWarmup failed; traveling immediately."));
        WarmupSubsystem->OnReadyToTravel.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel);
        WarmupSubsystem->OnCancelled.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupCancelled);
        bWarmupTravelPending = false;
        PendingWarmupTargetLevelPackage = NAME_None;
        ActiveWarmupSubsystem = nullptr;

        const FString MapName = FPackageName::GetShortName(TargetLevelPackageName.ToString());
        UGameplayStatics::OpenLevel(this, FName(*MapName));
        return;
    }
}

void USOTS_MissionDirectorSubsystem::CancelWarmupTravel()
{
    if (!bWarmupTravelPending)
    {
        return;
    }

    if (USOTS_ShaderWarmupSubsystem* WarmupSubsystem = ActiveWarmupSubsystem.Get())
    {
        WarmupSubsystem->OnReadyToTravel.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel);
        WarmupSubsystem->OnCancelled.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupCancelled);
        WarmupSubsystem->CancelWarmup(FText::FromString(TEXT("Travel cancelled")));
    }

    bWarmupTravelPending = false;
    PendingWarmupTargetLevelPackage = NAME_None;
    ActiveWarmupSubsystem = nullptr;
}

void USOTS_MissionDirectorSubsystem::StartMission(USOTS_MissionDefinition* MissionDef)
{
    if (MissionState == ESOTS_MissionState::InProgress)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("StartMission called while a mission is already in progress (CurrentMissionId=%s)."), *CurrentMissionId.ToString());
        return;
    }

    if (!MissionDef)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("StartMission called with null MissionDef."));
        return;
    }

    FString ValidationError;
    if (!ValidateMissionDefinition(MissionDef, ValidationError))
    {
        UE_LOG(LogSOTSMissionDirector, Error, TEXT("StartMission validation failed: %s"), *ValidationError);
        return;
    }

    ResetConditionTracking();

    ActiveMission = MissionDef;
    MissionState = ESOTS_MissionState::InProgress;
    MissionStealthConfigHandle = FSOTS_GSM_Handle();

    ObjectiveCompletion.Reset();
    ObjectiveFailed.Reset();
    CurrentOutcomeTag = FGameplayTag();
    StealthScore = 0;
    AlertsTriggered = 0;
    MilestoneHistory.Reset();
    bLoggedMissingStatsOnce = false;
    bLoggedMissingUIRouterOnce = false;

    for (const FSOTS_MissionObjective& Obj : MissionDef->Objectives)
    {
        if (!Obj.ObjectiveId.IsNone())
        {
            ObjectiveCompletion.FindOrAdd(Obj.ObjectiveId) = false;
        }
    }

    const FName ResolvedMissionId = !MissionDef->MissionIdentifier.Id.IsNone()
        ? MissionDef->MissionIdentifier.Id
        : MissionDef->MissionId;

    StartMissionRun(ResolvedMissionId, FGameplayTag());

    InitializeMissionRuntimeState(MissionDef);

    const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
    const FSOTS_MissionMilestoneSnapshot Snapshot = BuildMilestoneSnapshot(NowSeconds);
    TryWriteMilestoneToStatsAndProfile(Snapshot);

    // UI intent: mission started.
    const FText MissionTitle = ActiveMission ? ActiveMission->MissionName : FText::GetEmpty();
    const FGameplayTag UiCategory = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.MissionStarted"), false);
    EmitMissionUIIntent(!MissionTitle.IsEmpty() ? MissionTitle : FText::FromString(TEXT("Mission Started")), UiCategory);

    OnMissionStarted.Broadcast();

    // Optional mission-start FX.
    if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
    {
        if (ActiveMission && ActiveMission->FXTag_OnMissionStart.IsValid())
        {
            FX->TriggerFXByTag(
                this,
                ActiveMission->FXTag_OnMissionStart,
                nullptr,
                nullptr,
                FVector::ZeroVector,
                FRotator::ZeroRotator);
        }
    }

    // Optional mission-start tag for profile / analytics systems.
    if (ActiveMission && ActiveMission->MissionId != NAME_None)
    {
        if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
        {
            const FGameplayTag MissionStartedTag =
                TagSubsystem->GetTagByName(TEXT("Mission.State.Started"));

            if (MissionStartedTag.IsValid())
            {
                if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
                {
                    TagSubsystem->AddTagToActor(PlayerPawn, MissionStartedTag);
                }
            }
        }
    }
}

void USOTS_MissionDirectorSubsystem::RegisterObjectiveCompleted(bool bIsPrimaryObjective, float ScoreDelta, FText Title, FText Description, const FGameplayTagContainer& ContextTags)
{
    if (!bMissionActive)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("RegisterObjectiveCompleted called while no mission is active."));
        return;
    }

    if (bIsPrimaryObjective)
    {
        ++PrimaryObjectivesCompleted;
    }
    else
    {
        ++OptionalObjectivesCompleted;
    }
    FSOTS_MissionEventLogEntry Entry;
    Entry.TimeSinceStart = GetTimeSinceStart(this);
    Entry.Category = ESOTSMissionEventCategory::Objective;
    Entry.Title = Title;
    Entry.Description = Description;
    Entry.ScoreDelta = ScoreDelta;
    Entry.bIsPenalty = (ScoreDelta < 0.0f);
    Entry.ContextTags = ContextTags;

    CurrentScore += ScoreDelta;
    Entry.CumulativeScore = CurrentScore;

    AppendEventInternal(Entry);
    OnScoreChanged.Broadcast(CurrentScore, ScoreDelta, ContextTags);
}

void USOTS_MissionDirectorSubsystem::ClearMissionStealthConfigOverride()
{
    if (MissionStealthConfigHandle.IsValid())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
        {
            GSM->PopStealthConfig(MissionStealthConfigHandle, FGameplayTag());
        }
        MissionStealthConfigHandle = FSOTS_GSM_Handle();
    }
}

void USOTS_MissionDirectorSubsystem::CompleteObjectiveByTag(const FGameplayTag& CompletedTag)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    if (!CompletedTag.IsValid())
    {
        return;
    }

    bool bAnyUpdated = false;

    for (const FSOTS_MissionObjective& Obj : ActiveMission->Objectives)
    {
        // Only compare objectives that are keyed by a completion tag and have an id.
        if (Obj.ObjectiveId.IsNone())
        {
            continue;
        }

        bool bMatches = false;
        if (Obj.CompletionTag.IsValid() && Obj.CompletionTag == CompletedTag)
        {
            bMatches = true;
        }
        else if (Obj.CompletionTags.HasTag(CompletedTag))
        {
            bMatches = true;
        }

        if (bMatches)
        {
            bool& bCompleted = ObjectiveCompletion.FindOrAdd(Obj.ObjectiveId);
            if (!bCompleted)
            {
                bCompleted = true;
                bAnyUpdated = true;
                OnObjectiveUpdated.Broadcast(Obj.ObjectiveId);
            }
        }
    }

    // Optional per-mission stealth config override.
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        if (ActiveMission && ActiveMission->OverrideStealthConfig)
        {
            MissionStealthConfigHandle = GSM->PushStealthConfig(ActiveMission->OverrideStealthConfig);
        }
    }

    if (bAnyUpdated)
    {
        EvaluateMissionCompletion();
    }
}

void USOTS_MissionDirectorSubsystem::RegisterScoreEvent(ESOTSMissionEventCategory Category, float ScoreDelta, FText Title, FText Description, const FGameplayTagContainer& ContextTags)
{
    if (!bMissionActive)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("RegisterScoreEvent called while no mission is active."));
        return;
    }

    FSOTS_MissionEventLogEntry Entry;
    Entry.TimeSinceStart = GetTimeSinceStart(this);
    Entry.Category = Category;
    Entry.Title = Title;
    Entry.Description = Description;
    Entry.ScoreDelta = ScoreDelta;
    Entry.bIsPenalty = (ScoreDelta < 0.0f);
    Entry.ContextTags = ContextTags;

    CurrentScore += ScoreDelta;
    Entry.CumulativeScore = CurrentScore;

    AppendEventInternal(Entry);
    OnScoreChanged.Broadcast(CurrentScore, ScoreDelta, ContextTags);

    // Data-driven failure conditions based on mission events.
    if (ActiveMission && MissionState == ESOTS_MissionState::InProgress)
    {
        const FSOTS_MissionFailureConditions& Fail = ActiveMission->FailureConditions;
        if (!Fail.FailOnEventTags.IsEmpty() &&
            Fail.FailOnEventTags.HasAny(ContextTags))
        {
            UE_LOG(LogSOTSMissionDirector, Log,
                   TEXT("Mission '%s' failing due to event tags."),
                   *CurrentMissionId.ToString());
            FailMission(FGameplayTag());
        }
    }
}

void USOTS_MissionDirectorSubsystem::FailMission(const FGameplayTag& FailReasonTag)
{
    if (MissionState == ESOTS_MissionState::Completed ||
        MissionState == ESOTS_MissionState::Failed)
    {
        return;
    }

    MissionState = ESOTS_MissionState::Failed;

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        CurrentOutcomeTag = TagSubsystem->GetTagByName(TEXT("MissionOutcome.Failed"));
    }
    else
    {
        CurrentOutcomeTag = FGameplayTag();
    }

    UE_LOG(LogSOTSMissionDirector, Log,
        TEXT("Mission '%s' failed. ReasonTag=%s"),
        *CurrentMissionId.ToString(),
        FailReasonTag.IsValid() ? *FailReasonTag.ToString() : TEXT("None"));

    // Persistence + UI intents for mission failed.
    const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
    TryWriteMilestoneToStatsAndProfile(BuildMilestoneSnapshot(NowSeconds));
    EmitMissionTerminalUIIntent(MissionState);

    OnMissionFailed.Broadcast();

    // Optional mission-failed FX.
    if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
    {
        if (ActiveMission && ActiveMission->FXTag_OnMissionFailed.IsValid())
        {
            FX->TriggerFXByTag(
                this,
                ActiveMission->FXTag_OnMissionFailed,
                nullptr,
                nullptr,
                FVector::ZeroVector,
                FRotator::ZeroRotator);
        }
    }

    // Optional mission-failed tag for profile / analytics systems.
    if (ActiveMission && ActiveMission->MissionId != NAME_None)
    {
        if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
        {
            const FGameplayTag MissionFailedTag =
                TagSubsystem->GetTagByName(TEXT("Mission.State.Failed"));

            if (MissionFailedTag.IsValid())
            {
                if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
                {
                    TagSubsystem->AddTagToActor(PlayerPawn, MissionFailedTag);
                }
            }
        }
    }

    HandleMissionTerminalState();
}

void USOTS_MissionDirectorSubsystem::EndMissionRun(bool bSuccess, FSOTS_MissionRunSummary& OutSummary)
{
    if (!bMissionActive)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("EndMissionRun called while no mission is active."));
    }

    UWorld* World = nullptr;
    if (UGameInstance* GI = GetGameInstance())
    {
        World = GI->GetWorld();
    }

    const float CurrentTime = World ? World->GetTimeSeconds() : MissionStartTimeSeconds;
    const float Duration = FMath::Max(0.0f, CurrentTime - MissionStartTimeSeconds);

    OutSummary.MissionId = CurrentMissionId;
    OutSummary.DifficultyTag = CurrentDifficultyTag;
    OutSummary.DurationSeconds = Duration;
    OutSummary.FinalScore = CurrentScore;
    OutSummary.FinalRank = EvaluateRankFromScore(CurrentScore);
    OutSummary.bSuccess = bSuccess;
    OutSummary.PrimaryObjectivesCompleted = PrimaryObjectivesCompleted;
    OutSummary.OptionalObjectivesCompleted = OptionalObjectivesCompleted;
    OutSummary.EventLog = EventLog;

    LastMissionIdForProfile = OutSummary.MissionId;
    LastFinalScoreForProfile = OutSummary.FinalScore;
    LastDurationSecondsForProfile = OutSummary.DurationSeconds;
    bLastMissionCompletedForProfile = bSuccess;
    bLastMissionFailedForProfile = !bSuccess;

    if (OutSummary.MissionId != NAME_None)
    {
        if (bSuccess)
        {
            CompletedMissionIds.AddUnique(OutSummary.MissionId);
            FailedMissionIds.Remove(OutSummary.MissionId);
        }
        else
        {
            FailedMissionIds.AddUnique(OutSummary.MissionId);
        }
    }

    bMissionActive = false;
    ActiveMissionIdForProfile = NAME_None;

    UE_LOG(LogSOTSMissionDirector, Log, TEXT("Mission run ended: %s | Success=%d | Score=%.2f | Rank=%s"),
        *CurrentMissionId.ToString(),
        bSuccess ? 1 : 0,
        CurrentScore,
        *OutSummary.FinalRank.ToString());

    OnMissionEnded.Broadcast(OutSummary);
}

float USOTS_MissionDirectorSubsystem::GetTimeSinceStart(const UObject* WorldContextObject) const
{
    if (!bMissionActive)
    {
        return 0.0f;
    }

    const UWorld* World = nullptr;
    if (const UObject* Obj = WorldContextObject)
    {
        World = Obj->GetWorld();
    }

    const float CurrentTime = World ? World->GetTimeSeconds() : MissionStartTimeSeconds;
    return FMath::Max(0.0f, CurrentTime - MissionStartTimeSeconds);
}

void USOTS_MissionDirectorSubsystem::AppendEventInternal(const FSOTS_MissionEventLogEntry& Entry)
{
    EventLog.Add(Entry);
    OnEventLogged.Broadcast(Entry);
}

FName USOTS_MissionDirectorSubsystem::EvaluateRankFromScore(float FinalScore) const
{
    // Simple placeholder curve. You can replace this with data-driven tuning later.
    if (FinalScore >= 1000.0f)
    {
        return FName(TEXT("S"));
    }
    if (FinalScore >= 750.0f)
    {
        return FName(TEXT("A"));
    }
    if (FinalScore >= 500.0f)
    {
        return FName(TEXT("B"));
    }
    if (FinalScore >= 250.0f)
    {
        return FName(TEXT("C"));
    }
    return FName(TEXT("D"));
}

bool USOTS_MissionDirectorSubsystem::ValidateMissionDefinition(const USOTS_MissionDefinition* MissionDef, FString& OutError) const
{
    OutError.Reset();

    if (!MissionDef)
    {
        OutError = TEXT("MissionDef is null");
        return false;
    }

    const FName ResolvedMissionId = !MissionDef->MissionIdentifier.Id.IsNone()
        ? MissionDef->MissionIdentifier.Id
        : MissionDef->MissionId;

    if (ResolvedMissionId.IsNone())
    {
        OutError = TEXT("Mission has no MissionIdentifier or MissionId");
        return false;
    }

    TSet<FName> RouteIds;
    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            OutError = TEXT("Mission contains null RouteDefinition");
            return false;
        }

        if (Route->RouteId.Id.IsNone())
        {
            OutError = FString::Printf(TEXT("RouteDefinition missing RouteId (Mission=%s)"), *ResolvedMissionId.ToString());
            return false;
        }

        if (RouteIds.Contains(Route->RouteId.Id))
        {
            OutError = FString::Printf(TEXT("Duplicate RouteId '%s'"), *Route->RouteId.Id.ToString());
            return false;
        }

        RouteIds.Add(Route->RouteId.Id);
    }

    TSet<FName> ObjectiveIds;

    auto AccumulateObjective = [&](const USOTS_ObjectiveDefinition* Obj, const FString& Context) -> bool
    {
        if (!Obj)
        {
            OutError = FString::Printf(TEXT("Null ObjectiveDefinition in %s"), *Context);
            return false;
        }

        if (Obj->ObjectiveId.Id.IsNone())
        {
            OutError = FString::Printf(TEXT("ObjectiveDefinition missing ObjectiveId in %s"), *Context);
            return false;
        }

        if (ObjectiveIds.Contains(Obj->ObjectiveId.Id))
        {
            OutError = FString::Printf(TEXT("Duplicate ObjectiveId '%s'"), *Obj->ObjectiveId.Id.ToString());
            return false;
        }

        ObjectiveIds.Add(Obj->ObjectiveId.Id);

        if (Obj->AllowedRoutes.Num() > 0)
        {
            for (const FSOTS_RouteId& AllowedRouteId : Obj->AllowedRoutes)
            {
                if (!RouteIds.Contains(AllowedRouteId.Id))
                {
                    OutError = FString::Printf(TEXT("Objective '%s' references unknown AllowedRoute '%s'"), *Obj->ObjectiveId.Id.ToString(), *AllowedRouteId.Id.ToString());
                    return false;
                }
            }
        }

        return true;
    };

    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : MissionDef->GlobalObjectives)
    {
        if (!AccumulateObjective(Obj.Get(), TEXT("GlobalObjectives")))
        {
            return false;
        }
    }

    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            continue;
        }

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
        {
            if (!AccumulateObjective(Obj.Get(), FString::Printf(TEXT("Route '%s'"), *Route->RouteId.Id.ToString())))
            {
                return false;
            }
        }
    }

    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : MissionDef->GlobalObjectives)
    {
        if (!Obj)
        {
            continue;
        }

        for (const FSOTS_ObjectiveId& Requirement : Obj->RequiresCompleted)
        {
            if (!ObjectiveIds.Contains(Requirement.Id))
            {
                OutError = FString::Printf(TEXT("Objective '%s' requires unknown ObjectiveId '%s'"), *Obj->ObjectiveId.Id.ToString(), *Requirement.Id.ToString());
                return false;
            }
        }
    }

    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            continue;
        }

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
        {
            if (!Obj)
            {
                continue;
            }

            for (const FSOTS_ObjectiveId& Requirement : Obj->RequiresCompleted)
            {
                if (!ObjectiveIds.Contains(Requirement.Id))
                {
                    OutError = FString::Printf(TEXT("Objective '%s' requires unknown ObjectiveId '%s'"), *Obj->ObjectiveId.Id.ToString(), *Requirement.Id.ToString());
                    return false;
                }
            }
        }
    }

    return true;
}

void USOTS_MissionDirectorSubsystem::ResetConditionTracking()
{
    ConditionCountsByKey.Reset();
    ConditionStartTimeByKey.Reset();

    if (UWorld* World = GetWorld())
    {
        for (TPair<FName, FTimerHandle>& Pair : ConditionTimerHandles)
        {
            World->GetTimerManager().ClearTimer(Pair.Value);
        }
    }

    ConditionTimerHandles.Reset();
}

bool USOTS_MissionDirectorSubsystem::IsObjectiveIdCompleted(const FSOTS_ObjectiveId& ObjectiveId) const
{
    if (ObjectiveId.Id.IsNone())
    {
        return false;
    }

    if (const FSOTS_ObjectiveRuntimeState* State = GlobalObjectiveStatesById.Find(ObjectiveId.Id))
    {
        return State->State == ESOTS_ObjectiveState::Completed;
    }

    for (const TPair<FName, FSOTS_RouteRuntimeState>& Pair : RouteStatesById)
    {
        if (const FSOTS_ObjectiveRuntimeState* State = Pair.Value.ObjectiveStatesById.Find(ObjectiveId.Id))
        {
            if (State->State == ESOTS_ObjectiveState::Completed)
            {
                return true;
            }
        }
    }

    if (const bool* bLegacyCompleted = ObjectiveCompletion.Find(ObjectiveId.Id))
    {
        return *bLegacyCompleted;
    }

    return false;
}

bool USOTS_MissionDirectorSubsystem::AreObjectiveRequirementsSatisfied(const TArray<FSOTS_ObjectiveId>& RequiresCompleted) const
{
    for (const FSOTS_ObjectiveId& Requirement : RequiresCompleted)
    {
        if (!IsObjectiveIdCompleted(Requirement))
        {
            return false;
        }
    }

    return true;
}

void USOTS_MissionDirectorSubsystem::ClearConditionTrackingForObjective(const FSOTS_ObjectiveId& ObjectiveId)
{
    if (ObjectiveId.Id.IsNone())
    {
        return;
    }

    const FString Prefix = FString::Printf(TEXT("Obj:%s|"), *ObjectiveId.Id.ToString());

    auto ShouldRemove = [&](const FName& Key)
    {
        return Key.ToString().StartsWith(Prefix);
    };

    for (auto It = ConditionCountsByKey.CreateIterator(); It; ++It)
    {
        if (ShouldRemove(It.Key()))
        {
            It.RemoveCurrent();
        }
    }

    for (auto It = ConditionStartTimeByKey.CreateIterator(); It; ++It)
    {
        if (ShouldRemove(It.Key()))
        {
            It.RemoveCurrent();
        }
    }

    if (UWorld* World = GetWorld())
    {
        for (auto It = ConditionTimerHandles.CreateIterator(); It; ++It)
        {
            if (ShouldRemove(It.Key()))
            {
                World->GetTimerManager().ClearTimer(It.Value());
                It.RemoveCurrent();
            }
        }
    }
}

void USOTS_MissionDirectorSubsystem::HandleMissionTerminalState()
{
    ResetConditionTracking();

    if (ActiveMission && ActiveMission->OverrideStealthConfig)
    {
        ClearMissionStealthConfigOverride();
    }
}

FName USOTS_MissionDirectorSubsystem::BuildConditionKey(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_ObjectiveCondition& Condition) const
{
    const FString KeyString = FString::Printf(TEXT("Obj:%s|Tag:%s|Name:%s"),
        *ObjectiveId.Id.ToString(),
        *Condition.RequiredEventTag.ToString(),
        *Condition.RequiredNameId.ToString());
    return FName(*KeyString);
}

bool USOTS_MissionDirectorSubsystem::IsConditionSatisfied(const FSOTS_ObjectiveCondition& Condition, const FName& ConditionKey, double NowSeconds) const
{
    const int32* CountPtr = ConditionCountsByKey.Find(ConditionKey);
    const int32 CurrentCount = CountPtr ? *CountPtr : 0;
    if (CurrentCount < Condition.RequiredCount)
    {
        return false;
    }

    if (Condition.DurationSeconds > 0.0f)
    {
        if (const double* StartPtr = ConditionStartTimeByKey.Find(ConditionKey))
        {
            const double Elapsed = NowSeconds - *StartPtr;
            return Elapsed >= static_cast<double>(Condition.DurationSeconds);
        }
        return false;
    }

    return true;
}

void USOTS_MissionDirectorSubsystem::EvaluateMissionCompletion()
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    bool bShouldComplete = false;

    if (ActiveMission->Objectives.Num() > 0)
    {
        bool bHasMandatory = false;
        bool bAnyMandatoryCompleted = false;
        bool bAllMandatoryCompleted = true;

        for (const FSOTS_MissionObjective& Obj : ActiveMission->Objectives)
        {
            if (Obj.Type != ESOTS_ObjectiveType::Mandatory || Obj.ObjectiveId.IsNone())
            {
                continue;
            }

            bHasMandatory = true;

            const bool* bCompletedPtr = ObjectiveCompletion.Find(Obj.ObjectiveId);
            const bool bCompleted = (bCompletedPtr && *bCompletedPtr);

            if (bCompleted)
            {
                bAnyMandatoryCompleted = true;
            }
            else
            {
                bAllMandatoryCompleted = false;
            }
        }

        if (!bHasMandatory)
        {
            bShouldComplete = ObjectiveCompletion.Num() > 0;
        }
        else if (ActiveMission->bRequireAllMandatoryObjectives)
        {
            bShouldComplete = bAllMandatoryCompleted;
        }
        else
        {
            bShouldComplete = bAnyMandatoryCompleted;
        }
    }
    else if (ActiveMission->GlobalObjectives.Num() > 0 || ActiveMission->Routes.Num() > 0)
    {
        bool bHasMandatory = false;
        bool bAnyMandatoryCompleted = false;
        bool bAllMandatoryCompleted = true;
        bool bAnyCompleted = false;

        auto AccumulateState = [&](const USOTS_ObjectiveDefinition* ObjDef, const FSOTS_ObjectiveRuntimeState* RuntimeState)
        {
            if (!ObjDef || !RuntimeState)
            {
                return;
            }

            const bool bCompleted = RuntimeState->State == ESOTS_ObjectiveState::Completed;
            bAnyCompleted |= bCompleted;

            if (!ObjDef->bIsOptional)
            {
                bHasMandatory = true;
                bAnyMandatoryCompleted |= bCompleted;
                if (!bCompleted)
                {
                    bAllMandatoryCompleted = false;
                }
            }
        };

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : ActiveMission->GlobalObjectives)
        {
            const FSOTS_ObjectiveRuntimeState* RuntimeState = GlobalObjectiveStatesById.Find(Obj ? Obj->ObjectiveId.Id : NAME_None);
            AccumulateState(Obj.Get(), RuntimeState);
        }

        if (ActiveMission->Routes.Num() > 0)
        {
            if (ActiveRouteId.Id.IsNone())
            {
                ActivateDefaultRoute();
            }

            const FSOTS_RouteRuntimeState* RouteState = RouteStatesById.Find(ActiveRouteId.Id);
            if (RouteState)
            {
                if (USOTS_RouteDefinition* RouteDef = GetActiveRouteDefinition())
                {
                    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : RouteDef->Objectives)
                    {
                        const FSOTS_ObjectiveRuntimeState* RuntimeState = RouteState->ObjectiveStatesById.Find(Obj ? Obj->ObjectiveId.Id : NAME_None);
                        AccumulateState(Obj.Get(), RuntimeState);
                    }
                }
            }
        }

        if (!bHasMandatory)
        {
            bShouldComplete = bAnyCompleted;
        }
        else if (ActiveMission->bRequireAllMandatoryObjectives)
        {
            bShouldComplete = bAllMandatoryCompleted;
        }
        else
        {
            bShouldComplete = bAnyMandatoryCompleted;
        }
    }

    if (bShouldComplete)
    {
        MissionState = ESOTS_MissionState::Completed;
        if (!CurrentOutcomeTag.IsValid())
        {
            if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
            {
                CurrentOutcomeTag = TagSubsystem->GetTagByName(TEXT("MissionOutcome.Completed"));
            }
            else
            {
                CurrentOutcomeTag = FGameplayTag();
            }
        }
        OnMissionCompleted.Broadcast();

        // Persistence + UI intents for mission completed.
        const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
        TryWriteMilestoneToStatsAndProfile(BuildMilestoneSnapshot(NowSeconds));
        EmitMissionTerminalUIIntent(MissionState);

        // Optional mission-completed FX.
        if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
        {
            if (ActiveMission && ActiveMission->FXTag_OnMissionCompleted.IsValid())
            {
                FX->TriggerFXByTag(
                    this,
                    ActiveMission->FXTag_OnMissionCompleted,
                    nullptr,
                    nullptr,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator);
            }
        }

        if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
        {
            if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
            {
                // Optional mission-completed tag for profile / analytics systems.
                if (ActiveMission && ActiveMission->MissionId != NAME_None)
                {
                    const FGameplayTag MissionCompletedTag =
                        TagSubsystem->GetTagByName(TEXT("Mission.State.Completed"));

                    if (MissionCompletedTag.IsValid())
                    {
                        TagSubsystem->AddTagToActor(PlayerPawn, MissionCompletedTag);
                    }
                }

                // Apply mission rewards in a tag-driven manner.
                if (ActiveMission)
                {
                    const FSOTS_MissionRewards& Rewards = ActiveMission->Rewards;

                    auto AddRewardTags = [&](const FGameplayTagContainer& Tags)
                    {
                        for (const FGameplayTag& Tag : Tags)
                        {
                            if (Tag.IsValid())
                            {
                                TagSubsystem->AddTagToActor(PlayerPawn, Tag);
                            }
                        }
                    };

                    AddRewardTags(Rewards.GrantedTags);
                    AddRewardTags(Rewards.GrantedSkillTags);
                    AddRewardTags(Rewards.GrantedAbilityTags);

                    if (Rewards.FXTag_OnRewardsGranted.IsValid())
                    {
                        if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
                        {
                            FX->TriggerFXByTag(
                                this,
                                Rewards.FXTag_OnRewardsGranted,
                                nullptr,
                                nullptr,
                                FVector::ZeroVector,
                                FRotator::ZeroRotator);
                        }
                    }
                }
            }
        }

        HandleMissionTerminalState();
    }
}

void USOTS_MissionDirectorSubsystem::InitializeMissionRuntimeState(const USOTS_MissionDefinition* MissionDef)
{
    GlobalObjectiveStatesById.Reset();
    RouteStatesById.Reset();
    ActiveRouteId = FSOTS_RouteId();

    if (!MissionDef)
    {
        return;
    }

    const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;

    // Global objectives are always considered active.
    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : MissionDef->GlobalObjectives)
    {
        if (!Obj)
        {
            continue;
        }

        FSOTS_ObjectiveRuntimeState State;
        State.ObjectiveId = Obj->ObjectiveId;
        State.State = ESOTS_ObjectiveState::Active;
        State.ActivatedTimeSeconds = NowSeconds;
        GlobalObjectiveStatesById.Add(Obj->ObjectiveId.Id, State);
    }

    // Initialize route states and pick a default active route (first in the list).
    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            continue;
        }

        if (ActiveRouteId.Id.IsNone())
        {
            ActiveRouteId = Route->RouteId;
        }

        FSOTS_RouteRuntimeState RouteState;
        RouteState.RouteId = Route->RouteId;
        RouteState.bIsActive = (Route->RouteId.Id == ActiveRouteId.Id);

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
        {
            if (!Obj)
            {
                continue;
            }

            FSOTS_ObjectiveRuntimeState State;
            State.ObjectiveId = Obj->ObjectiveId;
            State.State = RouteState.bIsActive ? ESOTS_ObjectiveState::Active : ESOTS_ObjectiveState::Inactive;
            State.ActivatedTimeSeconds = RouteState.bIsActive ? NowSeconds : 0.0;

            RouteState.ObjectiveStatesById.Add(Obj->ObjectiveId.Id, State);
        }

        RouteStatesById.Add(Route->RouteId.Id, RouteState);
    }
}

void USOTS_MissionDirectorSubsystem::ActivateDefaultRoute()
{
    if (!ActiveMission || ActiveMission->Routes.Num() == 0)
    {
        return;
    }

    if (!ActiveRouteId.Id.IsNone())
    {
        return;
    }

    if (const TObjectPtr<USOTS_RouteDefinition>& FirstRoute = ActiveMission->Routes[0])
    {
        ActiveRouteId = FirstRoute->RouteId;

        for (TPair<FName, FSOTS_RouteRuntimeState>& Pair : RouteStatesById)
        {
            Pair.Value.bIsActive = (Pair.Key == ActiveRouteId.Id);
        }

            const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
            TryWriteMilestoneToStatsAndProfile(BuildMilestoneSnapshot(NowSeconds));

            const FGameplayTag UiCategory = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.RouteActivated"), false);
            EmitMissionUIIntent(FText::FromString(FString::Printf(TEXT("Route Activated: %s"), *ActiveRouteId.Id.ToString())), UiCategory);
    }
}

USOTS_RouteDefinition* USOTS_MissionDirectorSubsystem::GetActiveRouteDefinition() const
{
    if (!ActiveMission)
    {
        return nullptr;
    }

    for (const TObjectPtr<USOTS_RouteDefinition>& Route : ActiveMission->Routes)
    {
        if (Route && Route->RouteId.Id == ActiveRouteId.Id)
        {
            return Route.Get();
        }
    }

    return ActiveMission->Routes.Num() > 0 ? ActiveMission->Routes[0].Get() : nullptr;
}

bool USOTS_MissionDirectorSubsystem::EvaluateObjectiveConditions(const USOTS_ObjectiveDefinition* ObjectiveDef, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective, double NowSeconds, bool& bOutFailed, bool& bOutCompleted) const
{
    bOutFailed = false;
    bOutCompleted = false;

    if (!ObjectiveDef || ObjectiveDef->Conditions.Num() == 0)
    {
        return false;
    }

    int32 NonFailureConditionCount = 0;
    int32 NonFailureConditionsSatisfied = 0;

    for (const FSOTS_ObjectiveCondition& Condition : ObjectiveDef->Conditions)
    {
        if (!Condition.RequiredEventTag.IsValid())
        {
            continue;
        }

        const FName ConditionKey = BuildConditionKey(ObjectiveDef->ObjectiveId, Condition);
        const bool bSatisfied = IsConditionSatisfied(Condition, ConditionKey, NowSeconds);

        if (Condition.bIsFailureCondition)
        {
            if (bSatisfied)
            {
                bOutFailed = true;
                return true;
            }
            continue;
        }

        ++NonFailureConditionCount;
        if (bSatisfied)
        {
            ++NonFailureConditionsSatisfied;
        }
    }

    if (NonFailureConditionCount == 0)
    {
        return bOutFailed || bOutCompleted;
    }

    if (ObjectiveDef->bAllConditionsRequired)
    {
        bOutCompleted = (NonFailureConditionsSatisfied == NonFailureConditionCount);
    }
    else
    {
        bOutCompleted = (NonFailureConditionsSatisfied > 0);
    }

    return bOutFailed || bOutCompleted;
}

bool USOTS_MissionDirectorSubsystem::SetObjectiveState(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective, ESOTS_ObjectiveState NewState, double TimestampSeconds)
{
    FSOTS_ObjectiveRuntimeState* RuntimeState = nullptr;

    if (bIsGlobalObjective)
    {
        RuntimeState = GlobalObjectiveStatesById.Find(ObjectiveId.Id);
    }
    else if (!RouteId.Id.IsNone())
    {
        if (FSOTS_RouteRuntimeState* RouteState = RouteStatesById.Find(RouteId.Id))
        {
            RuntimeState = RouteState->ObjectiveStatesById.Find(ObjectiveId.Id);
        }
    }

    if (!RuntimeState)
    {
        return false;
    }

    if (RuntimeState->State == NewState)
    {
        return false;
    }

    RuntimeState->State = NewState;

    switch (NewState)
    {
    case ESOTS_ObjectiveState::Active:
        if (RuntimeState->ActivatedTimeSeconds <= 0.0)
        {
            RuntimeState->ActivatedTimeSeconds = TimestampSeconds;
        }
        break;
    case ESOTS_ObjectiveState::Completed:
        RuntimeState->CompletedTimeSeconds = TimestampSeconds;
        ObjectiveCompletion.FindOrAdd(ObjectiveId.Id) = true;
        ObjectiveFailed.Remove(ObjectiveId.Id);
        ClearConditionTrackingForObjective(ObjectiveId);
        break;
    case ESOTS_ObjectiveState::Failed:
        RuntimeState->FailedTimeSeconds = TimestampSeconds;
        ObjectiveFailed.FindOrAdd(ObjectiveId.Id) = true;
        ObjectiveCompletion.FindOrAdd(ObjectiveId.Id) = false;
        ClearConditionTrackingForObjective(ObjectiveId);
        break;
    default:
        break;
    }

    OnObjectiveUpdated.Broadcast(ObjectiveId.Id);

    if (NewState == ESOTS_ObjectiveState::Completed)
    {
        EvaluateMissionCompletion();
    }

    if (NewState == ESOTS_ObjectiveState::Completed || NewState == ESOTS_ObjectiveState::Failed)
    {
        const double NowSeconds = TimestampSeconds;
        TryWriteMilestoneToStatsAndProfile(BuildMilestoneSnapshot(NowSeconds));

        const USOTS_ObjectiveDefinition* ObjDef = FindObjectiveDefinition(ObjectiveId, RouteId, bIsGlobalObjective);
        EmitObjectiveUIIntent(ObjDef, bIsGlobalObjective, RouteId, NewState);
    }

    return true;
}

const USOTS_ObjectiveDefinition* USOTS_MissionDirectorSubsystem::FindObjectiveDefinition(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective) const
{
    if (!ActiveMission)
    {
        return nullptr;
    }

    if (bIsGlobalObjective)
    {
        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : ActiveMission->GlobalObjectives)
        {
            if (Obj && Obj->ObjectiveId.Id == ObjectiveId.Id)
            {
                return Obj.Get();
            }
        }
    }
    else
    {
        USOTS_RouteDefinition* RouteDef = nullptr;

        if (!RouteId.Id.IsNone())
        {
            for (const TObjectPtr<USOTS_RouteDefinition>& Route : ActiveMission->Routes)
            {
                if (Route && Route->RouteId.Id == RouteId.Id)
                {
                    RouteDef = Route.Get();
                    break;
                }
            }
        }

        if (!RouteDef)
        {
            RouteDef = GetActiveRouteDefinition();
        }

        if (RouteDef)
        {
            for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : RouteDef->Objectives)
            {
                if (Obj && Obj->ObjectiveId.Id == ObjectiveId.Id)
                {
                    return Obj.Get();
                }
            }
        }
    }

    return nullptr;
}

FSOTS_MissionMilestoneSnapshot USOTS_MissionDirectorSubsystem::BuildMilestoneSnapshot(double NowSeconds) const
{
    FSOTS_MissionMilestoneSnapshot Snapshot;

    if (ActiveMission)
    {
        Snapshot.MissionId = ActiveMission->MissionIdentifier.Id.IsNone()
            ? FSOTS_MissionId{ ActiveMission->MissionId }
            : ActiveMission->MissionIdentifier;
    }
    else
    {
        Snapshot.MissionId.Id = CurrentMissionId;
    }

    Snapshot.MissionState = MissionState;
    Snapshot.ActiveRouteId = ActiveRouteId;
    Snapshot.TimestampSeconds = NowSeconds;

    auto Collect = [&](const TMap<FName, FSOTS_ObjectiveRuntimeState>& States)
    {
        for (const TPair<FName, FSOTS_ObjectiveRuntimeState>& Pair : States)
        {
            if (Pair.Value.State == ESOTS_ObjectiveState::Completed)
            {
                Snapshot.CompletedObjectives.Add(Pair.Value.ObjectiveId);
            }
            else if (Pair.Value.State == ESOTS_ObjectiveState::Failed)
            {
                Snapshot.FailedObjectives.Add(Pair.Value.ObjectiveId);
            }
        }
    };

    Collect(GlobalObjectiveStatesById);

    for (const TPair<FName, FSOTS_RouteRuntimeState>& RoutePair : RouteStatesById)
    {
        Collect(RoutePair.Value.ObjectiveStatesById);
    }

    return Snapshot;
}

void USOTS_MissionDirectorSubsystem::TryWriteMilestoneToStatsAndProfile(const FSOTS_MissionMilestoneSnapshot& Snapshot)
{
    MilestoneHistory.Add(Snapshot);

    // Placeholder: integrate with Stats/ProfileShared when persistence sink is available.
    if (!bLoggedMissingStatsOnce)
    {
        UE_LOG(LogSOTSMissionDirector, Verbose, TEXT("[MissionDirector] Stats/Profile persistence sink not found; caching milestone only."));
        bLoggedMissingStatsOnce = true;
    }
}

void USOTS_MissionDirectorSubsystem::EmitMissionUIIntent(const FText& Message, FGameplayTag CategoryTag) const
{
    if (USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(this))
    {
        F_SOTS_UINotificationPayload Payload;
        Payload.Message = Message;
        Payload.DurationSeconds = 2.5f;
        Payload.CategoryTag = CategoryTag;
        Router->PushNotification_SOTS(Payload);
    }
    else if (!bLoggedMissingUIRouterOnce)
    {
        UE_LOG(LogSOTSMissionDirector, Verbose, TEXT("[MissionDirector] UI router missing; skipping mission UI intent."));
        const_cast<USOTS_MissionDirectorSubsystem*>(this)->bLoggedMissingUIRouterOnce = true;
    }
}

void USOTS_MissionDirectorSubsystem::EmitObjectiveUIIntent(const USOTS_ObjectiveDefinition* ObjectiveDef, bool bIsGlobalObjective, const FSOTS_RouteId& RouteId, ESOTS_ObjectiveState NewState) const
{
    if (!ObjectiveDef)
    {
        return;
    }

    FGameplayTag CategoryTag;
    if (NewState == ESOTS_ObjectiveState::Completed)
    {
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.ObjectiveCompleted"), false);
    }
    else if (NewState == ESOTS_ObjectiveState::Failed)
    {
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.ObjectiveFailed"), false);
    }

    FString RouteText;
    if (!bIsGlobalObjective && !RouteId.Id.IsNone())
    {
        RouteText = FString::Printf(TEXT(" [Route: %s]"), *RouteId.Id.ToString());
    }

    const FText Message = !ObjectiveDef->Title.IsEmpty()
        ? ObjectiveDef->Title
        : FText::FromString(FString::Printf(TEXT("Objective %s %s%s"), *ObjectiveDef->ObjectiveId.Id.ToString(), *UEnum::GetValueAsString(NewState), *RouteText));

    EmitMissionUIIntent(Message, CategoryTag);
}

void USOTS_MissionDirectorSubsystem::EmitMissionTerminalUIIntent(ESOTS_MissionState NewState) const
{
    FGameplayTag CategoryTag;
    switch (NewState)
    {
    case ESOTS_MissionState::Completed:
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.MissionCompleted"), false);
        break;
    case ESOTS_MissionState::Failed:
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.MissionFailed"), false);
        break;
    case ESOTS_MissionState::Aborted:
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.MissionAborted"), false);
        break;
    default:
        CategoryTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.UI.Mission.MissionUpdated"), false);
        break;
    }

    const FText Title = (ActiveMission && !ActiveMission->MissionName.IsEmpty()) ? ActiveMission->MissionName : FText::FromString(TEXT("Mission"));
    EmitMissionUIIntent(Title, CategoryTag);
}

void USOTS_MissionDirectorSubsystem::OnConditionDurationElapsed(FName ConditionKey, FSOTS_ObjectiveId ObjectiveId, bool bIsGlobalObjective, FSOTS_RouteId RouteId)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;

    const USOTS_ObjectiveDefinition* ObjectiveDef = nullptr;

    if (bIsGlobalObjective)
    {
        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : ActiveMission->GlobalObjectives)
        {
            if (Obj && Obj->ObjectiveId.Id == ObjectiveId.Id)
            {
                ObjectiveDef = Obj.Get();
                break;
            }
        }
    }
    else
    {
        if (USOTS_RouteDefinition* Route = GetActiveRouteDefinition())
        {
            for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
            {
                if (Obj && Obj->ObjectiveId.Id == ObjectiveId.Id)
                {
                    ObjectiveDef = Obj.Get();
                    break;
                }
            }
        }
    }

    if (!ObjectiveDef)
    {
        return;
    }

    bool bFailed = false;
    bool bCompleted = false;
    EvaluateObjectiveConditions(ObjectiveDef, RouteId, bIsGlobalObjective, NowSeconds, bFailed, bCompleted);

    if (bFailed)
    {
        SetObjectiveState(ObjectiveId, RouteId, bIsGlobalObjective, ESOTS_ObjectiveState::Failed, NowSeconds);
    }
    else if (bCompleted)
    {
        SetObjectiveState(ObjectiveId, RouteId, bIsGlobalObjective, ESOTS_ObjectiveState::Completed, NowSeconds);
    }
}

void USOTS_MissionDirectorSubsystem::HandleProgressEvent(const FSOTS_MissionProgressEvent& Event)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    if (!Event.EventTag.IsValid())
    {
        return;
    }

    const double NowSeconds = (Event.TimestampSeconds > 0.0)
        ? Event.TimestampSeconds
        : (GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0);

    // Ensure we have an active route selection if any routes exist.
    if (ActiveRouteId.Id.IsNone())
    {
        ActivateDefaultRoute();
    }

    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : ActiveMission->GlobalObjectives)
    {
        ApplyProgressToObjective(Obj.Get(), true, FSOTS_RouteId(), Event, NowSeconds);
    }

    if (USOTS_RouteDefinition* ActiveRoute = GetActiveRouteDefinition())
    {
        const FSOTS_RouteId RouteId = ActiveRoute->RouteId;
        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : ActiveRoute->Objectives)
        {
            ApplyProgressToObjective(Obj.Get(), false, RouteId, Event, NowSeconds);
        }
    }
}

void USOTS_MissionDirectorSubsystem::ApplyProgressToObjective(const USOTS_ObjectiveDefinition* ObjectiveDef, bool bIsGlobalObjective, const FSOTS_RouteId& RouteId, const FSOTS_MissionProgressEvent& Event, double NowSeconds)
{
    if (!ObjectiveDef || ObjectiveDef->Conditions.Num() == 0)
    {
        return;
    }

    const FName RouteContextId = !RouteId.Id.IsNone() ? RouteId.Id : ActiveRouteId.Id;
    if (ObjectiveDef->AllowedRoutes.Num() > 0)
    {
        bool bRouteAllowed = false;
        for (const FSOTS_RouteId& AllowedRouteId : ObjectiveDef->AllowedRoutes)
        {
            if (AllowedRouteId.Id == RouteContextId)
            {
                bRouteAllowed = true;
                break;
            }
        }

        if (!bRouteAllowed)
        {
            return;
        }
    }

    if (ObjectiveDef->RequiresCompleted.Num() > 0 && !AreObjectiveRequirementsSatisfied(ObjectiveDef->RequiresCompleted))
    {
        return;
    }

    FSOTS_ObjectiveRuntimeState* RuntimeState = nullptr;

    if (bIsGlobalObjective)
    {
        RuntimeState = GlobalObjectiveStatesById.Find(ObjectiveDef->ObjectiveId.Id);
        if (!RuntimeState)
        {
            FSOTS_ObjectiveRuntimeState NewState;
            NewState.ObjectiveId = ObjectiveDef->ObjectiveId;
            NewState.State = ESOTS_ObjectiveState::Active;
            NewState.ActivatedTimeSeconds = NowSeconds;
            RuntimeState = &GlobalObjectiveStatesById.Add(ObjectiveDef->ObjectiveId.Id, NewState);
        }
    }
    else if (!RouteId.Id.IsNone())
    {
        if (FSOTS_RouteRuntimeState* RouteState = RouteStatesById.Find(RouteId.Id))
        {
            RuntimeState = RouteState->ObjectiveStatesById.Find(ObjectiveDef->ObjectiveId.Id);
            if (!RuntimeState)
            {
                FSOTS_ObjectiveRuntimeState NewState;
                NewState.ObjectiveId = ObjectiveDef->ObjectiveId;
                NewState.State = RouteState->bIsActive ? ESOTS_ObjectiveState::Active : ESOTS_ObjectiveState::Inactive;
                NewState.ActivatedTimeSeconds = RouteState->bIsActive ? NowSeconds : 0.0;
                RuntimeState = &RouteState->ObjectiveStatesById.Add(ObjectiveDef->ObjectiveId.Id, NewState);
            }
        }
    }

    if (!RuntimeState || RuntimeState->State == ESOTS_ObjectiveState::Completed || RuntimeState->State == ESOTS_ObjectiveState::Failed)
    {
        return;
    }

    bool bMatchedCondition = false;

    for (const FSOTS_ObjectiveCondition& Condition : ObjectiveDef->Conditions)
    {
        if (!Condition.RequiredEventTag.IsValid() || Condition.RequiredEventTag != Event.EventTag)
        {
            continue;
        }

        if (!Condition.RequiredNameId.IsNone() && Condition.RequiredNameId != Event.NameId)
        {
            continue;
        }

        const FName ConditionKey = BuildConditionKey(ObjectiveDef->ObjectiveId, Condition);
        int32& Count = ConditionCountsByKey.FindOrAdd(ConditionKey);
        ++Count;

        if (Condition.DurationSeconds > 0.0f && !ConditionStartTimeByKey.Contains(ConditionKey))
        {
            ConditionStartTimeByKey.Add(ConditionKey, NowSeconds);

            if (UWorld* World = GetWorld())
            {
                FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
                    this,
                    &USOTS_MissionDirectorSubsystem::OnConditionDurationElapsed,
                    ConditionKey,
                    ObjectiveDef->ObjectiveId,
                    bIsGlobalObjective,
                    RouteId);

                FTimerHandle& Handle = ConditionTimerHandles.FindOrAdd(ConditionKey);
                World->GetTimerManager().SetTimer(Handle, Delegate, Condition.DurationSeconds, false);
            }
        }

        bMatchedCondition = true;
    }

    if (!bMatchedCondition)
    {
        return;
    }

    bool bFailed = false;
    bool bCompleted = false;
    if (EvaluateObjectiveConditions(ObjectiveDef, RouteId, bIsGlobalObjective, NowSeconds, bFailed, bCompleted))
    {
        if (bFailed)
        {
            SetObjectiveState(ObjectiveDef->ObjectiveId, RouteId, bIsGlobalObjective, ESOTS_ObjectiveState::Failed, NowSeconds);
        }
        else if (bCompleted)
        {
            SetObjectiveState(ObjectiveDef->ObjectiveId, RouteId, bIsGlobalObjective, ESOTS_ObjectiveState::Completed, NowSeconds);
        }
    }
}

void USOTS_MissionDirectorSubsystem::HandleStealthLevelChanged(
    ESOTSStealthLevel /*OldLevel*/,
    ESOTSStealthLevel NewLevel,
    float /*NewScore*/)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        const FGameplayTag ProgressTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.MissionEvent.GSM.StealthLevelChanged"), false);
        if (ProgressTag.IsValid())
        {
            FSOTS_MissionProgressEvent Progress;
            Progress.EventTag = ProgressTag;
            Progress.TimestampSeconds = World->GetTimeSeconds();
            Progress.Value01 = static_cast<float>(NewLevel);
            HandleProgressEvent(Progress);
        }
    }

    const int32 LevelAsInt = static_cast<int32>(NewLevel);
    StealthScore = FMath::Max(StealthScore, LevelAsInt);

    // Immediate failure on highest risk level if configured.
    if (ActiveMission->bFailOnMaxTier &&
        NewLevel == ESOTSStealthLevel::FullyDetected)
    {
        FailMission(FGameplayTag());
        return;
    }

    if (ActiveMission->MaxAllowedStealthTier >= 0 &&
        LevelAsInt > ActiveMission->MaxAllowedStealthTier)
    {
        FailMission(FGameplayTag());
    }
}

void USOTS_MissionDirectorSubsystem::HandleAIAwarenessStateChanged(
    AActor* SubjectAI,
    ESOTS_AIAwarenessState /*OldState*/,
    ESOTS_AIAwarenessState NewState,
    const FSOTS_GSM_AIRecord& /*Record*/)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    const FGameplayTag ProgressTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.MissionEvent.GSM.AwarenessChanged"), false);
    if (!ProgressTag.IsValid())
    {
        return;
    }

    FSOTS_MissionProgressEvent Event;
    Event.EventTag = ProgressTag;
    Event.InstigatorActor = SubjectAI;
    Event.Value01 = static_cast<float>(NewState);
    if (UWorld* World = GetWorld())
    {
        Event.TimestampSeconds = World->GetTimeSeconds();
    }

    HandleProgressEvent(Event);
}

void USOTS_MissionDirectorSubsystem::HandleGlobalAlertnessChanged(float NewValue, float /*OldValue*/)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    const FGameplayTag ProgressTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.MissionEvent.GSM.GlobalAlertnessChanged"), false);
    if (!ProgressTag.IsValid())
    {
        return;
    }

    FSOTS_MissionProgressEvent Event;
    Event.EventTag = ProgressTag;
    Event.Value01 = NewValue;
    if (UWorld* World = GetWorld())
    {
        Event.TimestampSeconds = World->GetTimeSeconds();
    }

    HandleProgressEvent(Event);
}

void USOTS_MissionDirectorSubsystem::HandleExecutionEvent(const FSOTS_KEM_ExecutionEvent& Event)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

    if (Event.Result != ESOTS_KEM_ExecutionResult::Succeeded)
    {
        return;
    }

    const FGameplayTag ExecutionTag = Event.ExecutionTag;
    AActor* TargetActor = Event.Target.Get();

    // Normalize into progress pipeline.
    {
        const FGameplayTag ProgressTag = FGameplayTag::RequestGameplayTag(TEXT("SAS.MissionEvent.KEM.Execution"), false);
        if (ProgressTag.IsValid())
        {
            FSOTS_MissionProgressEvent Progress;
            Progress.EventTag = ProgressTag;
            Progress.InstigatorActor = Event.Instigator.Get();
            Progress.NameId = ExecutionTag.IsValid() ? ExecutionTag.GetTagName() : NAME_None;
            if (UWorld* World = GetWorld())
            {
                Progress.TimestampSeconds = World->GetTimeSeconds();
            }
            HandleProgressEvent(Progress);
        }
    }

    bool bAnyUpdated = false;

    for (const FSOTS_MissionObjective& Obj : ActiveMission->Objectives)
    {
        if (Obj.ObjectiveId.IsNone())
        {
            continue;
        }

        // Skip objectives that are already completed or failed.
        if (const bool* bCompletedPtr = ObjectiveCompletion.Find(Obj.ObjectiveId))
        {
            if (*bCompletedPtr)
            {
                continue;
            }
        }
        if (const bool* bFailedPtr = ObjectiveFailed.Find(Obj.ObjectiveId))
        {
            if (*bFailedPtr)
            {
                continue;
            }
        }

        // Check KEM execution tag requirement.
        if (Obj.RequiredExecutionTag.IsValid())
        {
            if (!ExecutionTag.IsValid() || ExecutionTag != Obj.RequiredExecutionTag)
            {
                continue;
            }
        }

        // Check target tag requirement via the central tag manager.
        if (Obj.RequiredTargetTag.IsValid())
        {
            if (!TargetActor)
            {
                continue;
            }

            if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
            {
                if (!TagSubsystem->ActorHasTag(TargetActor, Obj.RequiredTargetTag))
                {
                    continue;
                }
            }
        }

        // All KEM requirements met for this objective; mark it completed.
        bool& bCompleted = ObjectiveCompletion.FindOrAdd(Obj.ObjectiveId);
        if (!bCompleted)
        {
            bCompleted = true;
            bAnyUpdated = true;

            UE_LOG(LogSOTSMissionDirector, Log,
                   TEXT("Mission '%s': Objective '%s' completed via KEM execution (ExecutionTag=%s)."),
                   *CurrentMissionId.ToString(),
                   *Obj.ObjectiveId.ToString(),
                   ExecutionTag.IsValid() ? *ExecutionTag.ToString() : TEXT("None"));

            OnObjectiveUpdated.Broadcast(Obj.ObjectiveId);
        }
    }

    if (bAnyUpdated)
    {
        EvaluateMissionCompletion();
    }
}

void USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel()
{
    if (USOTS_ShaderWarmupSubsystem* WarmupSubsystem = ActiveWarmupSubsystem.Get())
    {
        WarmupSubsystem->OnReadyToTravel.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel);
        WarmupSubsystem->OnCancelled.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupCancelled);
    }

    const FName TargetLevel = PendingWarmupTargetLevelPackage;
    bWarmupTravelPending = false;
    PendingWarmupTargetLevelPackage = NAME_None;
    ActiveWarmupSubsystem = nullptr;

    if (TargetLevel.IsNone())
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("Warmup ready, but no pending target level was set."));
        return;
    }

    const FString MapName = FPackageName::GetShortName(TargetLevel.ToString());
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSMissionDirector, Log, TEXT("MD: ShaderWarmup -> OpenLevel(%s)"), *MapName);
#endif
    UGameplayStatics::OpenLevel(this, FName(*MapName));
}

void USOTS_MissionDirectorSubsystem::HandleWarmupCancelled(FText Reason)
{
    if (USOTS_ShaderWarmupSubsystem* WarmupSubsystem = ActiveWarmupSubsystem.Get())
    {
        WarmupSubsystem->OnFinished.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupReadyToTravel);
        WarmupSubsystem->OnCancelled.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleWarmupCancelled);
    }

    UE_LOG(LogSOTSMissionDirector, Log, TEXT("Warmup cancelled; travel aborted. Reason=%s"), *Reason.ToString());

    bWarmupTravelPending = false;
    PendingWarmupTargetLevelPackage = NAME_None;
    ActiveWarmupSubsystem = nullptr;
}

TArray<FSOTS_MissionObjectiveRuntimeState> USOTS_MissionDirectorSubsystem::GetCurrentMissionObjectives() const
{
    TArray<FSOTS_MissionObjectiveRuntimeState> Result;

    if (!ActiveMission)
    {
        return Result;
    }

    for (const FSOTS_MissionObjective& Obj : ActiveMission->Objectives)
    {
        FSOTS_MissionObjectiveRuntimeState Runtime;
        Runtime.ObjectiveId = Obj.ObjectiveId;
        Runtime.Description = Obj.Description;
        Runtime.Type        = Obj.Type;
        Runtime.bOptional   = (Obj.Type == ESOTS_ObjectiveType::Optional) || Obj.bOptional;

        if (Obj.ObjectiveId != NAME_None)
        {
            if (const bool* bCompletedPtr = ObjectiveCompletion.Find(Obj.ObjectiveId))
            {
                Runtime.bCompleted = *bCompletedPtr;
            }
            if (const bool* bFailedPtr = ObjectiveFailed.Find(Obj.ObjectiveId))
            {
                Runtime.bFailed = *bFailedPtr;
            }
        }

        // Active if not completed/failed and all prerequisites (if any) are completed.
        bool bPrereqsMet = true;
        if (Obj.PrerequisiteObjectiveIds.Num() > 0)
        {
            for (const FName& PrereqId : Obj.PrerequisiteObjectiveIds)
            {
                const bool* bPrereqCompleted = ObjectiveCompletion.Find(PrereqId);
                if (!(bPrereqCompleted && *bPrereqCompleted))
                {
                    bPrereqsMet = false;
                    break;
                }
            }
        }

        Runtime.bActive = !Runtime.bCompleted && !Runtime.bFailed && bPrereqsMet;

        Result.Add(Runtime);
    }

    return Result;
}

bool USOTS_MissionDirectorSubsystem::IsObjectiveCompleted(FName ObjectiveId) const
{
    if (ObjectiveId.IsNone())
    {
        return false;
    }

    if (const bool* bCompletedPtr = ObjectiveCompletion.Find(ObjectiveId))
    {
        return *bCompletedPtr;
    }

    return false;
}

void USOTS_MissionDirectorSubsystem::NotifyMissionEvent(const FGameplayTag& EventTag)
{
    CompleteObjectiveByTag(EventTag);
}

void USOTS_MissionDirectorSubsystem::PushMissionProgressEvent(const FSOTS_MissionProgressEvent& Event)
{
    FSOTS_MissionProgressEvent NormalizedEvent = Event;

    if (NormalizedEvent.TimestampSeconds <= 0.0)
    {
        if (UWorld* World = GetWorld())
        {
            NormalizedEvent.TimestampSeconds = static_cast<double>(World->GetTimeSeconds());
        }
    }

    HandleProgressEvent(NormalizedEvent);
}

void USOTS_MissionDirectorSubsystem::ForceFailObjective(FName ObjectiveId, const FString& Reason)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress || ObjectiveId.IsNone())
    {
        return;
    }

    ObjectiveFailed.FindOrAdd(ObjectiveId) = true;

    const double NowSeconds = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;

    FSOTS_ObjectiveId WrappedId;
    WrappedId.Id = ObjectiveId;

    bool bRuntimeStateUpdated = false;

    if (GlobalObjectiveStatesById.Contains(ObjectiveId))
    {
        bRuntimeStateUpdated = SetObjectiveState(WrappedId, FSOTS_RouteId(), true, ESOTS_ObjectiveState::Failed, NowSeconds);
    }
    else
    {
        for (const TPair<FName, FSOTS_RouteRuntimeState>& Pair : RouteStatesById)
        {
            if (Pair.Value.ObjectiveStatesById.Contains(ObjectiveId))
            {
                FSOTS_RouteId RouteId;
                RouteId.Id = Pair.Key;
                bRuntimeStateUpdated = SetObjectiveState(WrappedId, RouteId, false, ESOTS_ObjectiveState::Failed, NowSeconds);
                break;
            }
        }
    }

    // If this objective is marked mission-critical, failing it fails the mission.
    for (const FSOTS_MissionObjective& Obj : ActiveMission->Objectives)
    {
        if (Obj.ObjectiveId == ObjectiveId && Obj.bMissionCritical)
        {
            UE_LOG(LogSOTSMissionDirector, Log,
                TEXT("Objective '%s' failed with reason '%s' and is mission-critical. Failing mission."),
                *ObjectiveId.ToString(),
                *Reason);
            FailMission(FGameplayTag());
            break;
        }
    }
}

void USOTS_MissionDirectorSubsystem::NotifyAlertTriggered()
{
    ++AlertsTriggered;

    if (ActiveMission && MissionState == ESOTS_MissionState::InProgress)
    {
        const FSOTS_MissionFailureConditions& Fail = ActiveMission->FailureConditions;
        if (Fail.MaxAllowedAlerts > 0 &&
            AlertsTriggered > Fail.MaxAllowedAlerts)
        {
            UE_LOG(LogSOTSMissionDirector, Log,
                   TEXT("Mission '%s' failing due to alerts (Alerts=%d > MaxAllowed=%d)."),
                   *CurrentMissionId.ToString(),
                   AlertsTriggered,
                   Fail.MaxAllowedAlerts);
            FailMission(FGameplayTag());
        }
    }
}

FSOTS_MissionRuntimeState USOTS_MissionDirectorSubsystem::GetCurrentMissionState() const
{
    FSOTS_MissionRuntimeState State;

    if (ActiveMission)
    {
        State.MissionId = ActiveMission->MissionIdentifier.Id.IsNone() ? FSOTS_MissionId{ ActiveMission->MissionId } : ActiveMission->MissionIdentifier;
    }
    else
    {
        State.MissionId.Id = CurrentMissionId;
    }
    State.bMissionCompleted = (MissionState == ESOTS_MissionState::Completed);
    State.bMissionFailed    = (MissionState == ESOTS_MissionState::Failed);
    State.OutcomeTag        = CurrentOutcomeTag;
    State.StealthScore      = StealthScore;
    State.OptionalObjectivesCompleted = OptionalObjectivesCompleted;
    State.AlertsTriggered   = AlertsTriggered;

    State.State = MissionState;
    State.ActiveRouteId = ActiveRouteId;
    State.GlobalObjectiveStatesById = GlobalObjectiveStatesById;
    State.RouteStatesById = RouteStatesById;
    State.StartTimeSeconds = MissionStartTimeSeconds;

    if (MissionState == ESOTS_MissionState::Completed || MissionState == ESOTS_MissionState::Failed)
    {
        State.EndTimeSeconds = MissionStartTimeSeconds + GetTimeSinceStart(this);
    }

    State.Objectives = GetCurrentMissionObjectives();
    return State;
}

void USOTS_MissionDirectorSubsystem::RestoreMissionFromSave(const FSOTS_MissionRuntimeState& SavedState)
{
    CurrentMissionId   = SavedState.MissionId.Id;
    CurrentOutcomeTag  = SavedState.OutcomeTag;
    StealthScore       = SavedState.StealthScore;
    AlertsTriggered    = SavedState.AlertsTriggered;
    MissionState       = SavedState.State;
    ActiveRouteId      = SavedState.ActiveRouteId;
    GlobalObjectiveStatesById = SavedState.GlobalObjectiveStatesById;
    RouteStatesById = SavedState.RouteStatesById;
    MissionStartTimeSeconds = SavedState.StartTimeSeconds;

    ObjectiveCompletion.Reset();
    ObjectiveFailed.Reset();

    for (const FSOTS_MissionObjectiveRuntimeState& ObjState : SavedState.Objectives)
    {
        if (!ObjState.ObjectiveId.IsNone())
        {
            ObjectiveCompletion.FindOrAdd(ObjState.ObjectiveId) = ObjState.bCompleted;
            ObjectiveFailed.FindOrAdd(ObjState.ObjectiveId)     = ObjState.bFailed;
        }
    }
}

bool USOTS_MissionDirectorSubsystem::GetNextMissionIdByOutcome(FName& OutMissionId) const
{
    OutMissionId = NAME_None;

    if (!ActiveMission || !CurrentOutcomeTag.IsValid())
    {
        return false;
    }

    if (const FName* Found = ActiveMission->NextMissionByOutcome.Find(CurrentOutcomeTag))
    {
        OutMissionId = *Found;
        return !OutMissionId.IsNone();
    }

    return false;
}

FSOTS_MissionRunSummary USOTS_MissionDirectorSubsystem::GetCurrentMissionResult() const
{
    FSOTS_MissionRunSummary Summary;

    Summary.MissionId                  = CurrentMissionId;
    Summary.DifficultyTag              = CurrentDifficultyTag;
    Summary.FinalScore                 = CurrentScore;
    Summary.FinalRank                  = EvaluateRankFromScore(CurrentScore);
    Summary.bSuccess                   = (MissionState == ESOTS_MissionState::Completed);
    Summary.PrimaryObjectivesCompleted = PrimaryObjectivesCompleted;
    Summary.OptionalObjectivesCompleted = OptionalObjectivesCompleted;
    Summary.EventLog                   = EventLog;

    Summary.DurationSeconds = GetTimeSinceStart(this);

    return Summary;
}

void USOTS_MissionDirectorSubsystem::ExportMissionState(FName /*MissionId*/, FSOTS_MissionRuntimeState& OutState) const
{
    OutState = GetCurrentMissionState();
}

void USOTS_MissionDirectorSubsystem::ImportMissionState(const FSOTS_MissionRuntimeState& InState)
{
    RestoreMissionFromSave(InState);
}

void USOTS_MissionDirectorSubsystem::BuildProfileData(FSOTS_MissionProfileData& OutData) const
{
    OutData.ActiveMissionId = ActiveMissionIdForProfile;
    OutData.CompletedMissionIds = CompletedMissionIds;
    OutData.FailedMissionIds = FailedMissionIds;
    OutData.LastMissionId = LastMissionIdForProfile;
    OutData.LastFinalScore = LastFinalScoreForProfile;
    OutData.LastDurationSeconds = LastDurationSecondsForProfile;
    OutData.bLastMissionCompleted = bLastMissionCompletedForProfile;
    OutData.bLastMissionFailed = bLastMissionFailedForProfile;
}

void USOTS_MissionDirectorSubsystem::ApplyProfileData(const FSOTS_MissionProfileData& InData)
{
    ActiveMissionIdForProfile = InData.ActiveMissionId;
    CompletedMissionIds = InData.CompletedMissionIds;
    FailedMissionIds = InData.FailedMissionIds;
    LastMissionIdForProfile = InData.LastMissionId;
    LastFinalScoreForProfile = InData.LastFinalScore;
    LastDurationSecondsForProfile = InData.LastDurationSeconds;
    bLastMissionCompletedForProfile = InData.bLastMissionCompleted;
    bLastMissionFailedForProfile = InData.bLastMissionFailed;
}

void USOTS_MissionDirectorSubsystem::BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot)
{
    BuildProfileData(InOutSnapshot.Missions);
}

void USOTS_MissionDirectorSubsystem::ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot)
{
    ApplyProfileData(Snapshot.Missions);
}
