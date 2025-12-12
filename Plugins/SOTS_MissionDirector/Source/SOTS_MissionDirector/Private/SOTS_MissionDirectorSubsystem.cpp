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
#include "SOTS_TagAccessHelpers.h"

USOTS_MissionDirectorSubsystem::USOTS_MissionDirectorSubsystem()
    : bMissionActive(false)
    , MissionStartTimeSeconds(0.0f)
    , CurrentScore(0.0f)
    , PrimaryObjectivesCompleted(0)
    , OptionalObjectivesCompleted(0)
    , ActiveMission(nullptr)
    , MissionState(ESOTS_MissionState::Inactive)
    , CachedStealthSubsystem(nullptr)
{
}

void USOTS_MissionDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    MissionState = ESOTS_MissionState::Inactive;
    ActiveMission = nullptr;
    ObjectiveCompletion.Reset();
    ObjectiveFailed.Reset();
    CurrentOutcomeTag = FGameplayTag();
    StealthScore = 0;
    AlertsTriggered = 0;
    ActiveMissionIdForProfile = NAME_None;
    CompletedMissionIds.Reset();
    FailedMissionIds.Reset();
    LastMissionIdForProfile = NAME_None;
    LastFinalScoreForProfile = 0.0f;
    LastDurationSecondsForProfile = 0.0f;
    bLastMissionCompletedForProfile = false;
    bLastMissionFailedForProfile = false;

    CachedStealthSubsystem = USOTS_GlobalStealthManagerSubsystem::Get(this);
    if (CachedStealthSubsystem)
    {
        CachedStealthSubsystem->OnStealthLevelChanged.AddDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleStealthLevelChanged);
    }

    // Subscribe to KillExecutionManager execution events for KEM-driven objectives.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
        {
            KEM->OnExecutionEvent.AddDynamic(this, &USOTS_MissionDirectorSubsystem::HandleExecutionEvent);
        }
    }
}

void USOTS_MissionDirectorSubsystem::Deinitialize()
{
    if (CachedStealthSubsystem)
    {
        CachedStealthSubsystem->OnStealthLevelChanged.RemoveDynamic(
            this,
            &USOTS_MissionDirectorSubsystem::HandleStealthLevelChanged);
        CachedStealthSubsystem = nullptr;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_KEMManagerSubsystem* KEM = GI->GetSubsystem<USOTS_KEMManagerSubsystem>())
        {
            KEM->OnExecutionEvent.RemoveDynamic(this, &USOTS_MissionDirectorSubsystem::HandleExecutionEvent);
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

void USOTS_MissionDirectorSubsystem::StartMission(USOTS_MissionDefinition* MissionDef)
{
    if (!MissionDef)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("StartMission called with null MissionDef."));
        return;
    }

    ActiveMission = MissionDef;
    MissionState = ESOTS_MissionState::InProgress;

    ObjectiveCompletion.Reset();
    ObjectiveFailed.Reset();
    CurrentOutcomeTag = FGameplayTag();
    StealthScore = 0;
    AlertsTriggered = 0;

    for (const FSOTS_MissionObjective& Obj : MissionDef->Objectives)
    {
        if (!Obj.ObjectiveId.IsNone())
        {
            ObjectiveCompletion.FindOrAdd(Obj.ObjectiveId) = false;
        }
    }

    // Also start a scoring run using this mission id; difficulty is left unset here
    // so other systems can decide how to tag it.
    StartMissionRun(MissionDef->MissionId, FGameplayTag());

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
            GSM->PushStealthConfig(ActiveMission->OverrideStealthConfig);
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

    // Pop any mission-specific stealth config override.
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        if (ActiveMission && ActiveMission->OverrideStealthConfig)
        {
            GSM->PopStealthConfig();
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

void USOTS_MissionDirectorSubsystem::EvaluateMissionCompletion()
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress)
    {
        return;
    }

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

    bool bShouldComplete = false;

    if (!bHasMandatory)
    {
        // No mandatory objectives: treat mission as complete once any objective has been recorded.
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

        // Pop any mission-specific stealth config override.
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
        {
            if (ActiveMission && ActiveMission->OverrideStealthConfig)
            {
                GSM->PopStealthConfig();
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

void USOTS_MissionDirectorSubsystem::ForceFailObjective(FName ObjectiveId, const FString& Reason)
{
    if (!ActiveMission || MissionState != ESOTS_MissionState::InProgress || ObjectiveId.IsNone())
    {
        return;
    }

    ObjectiveFailed.FindOrAdd(ObjectiveId) = true;

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

    State.MissionId = ActiveMission ? ActiveMission->MissionId : CurrentMissionId;
    State.bMissionCompleted = (MissionState == ESOTS_MissionState::Completed);
    State.bMissionFailed    = (MissionState == ESOTS_MissionState::Failed);
    State.OutcomeTag        = CurrentOutcomeTag;
    State.StealthScore      = StealthScore;
    State.OptionalObjectivesCompleted = OptionalObjectivesCompleted;
    State.AlertsTriggered   = AlertsTriggered;

    State.Objectives = GetCurrentMissionObjectives();
    return State;
}

void USOTS_MissionDirectorSubsystem::RestoreMissionFromSave(const FSOTS_MissionRuntimeState& SavedState)
{
    CurrentMissionId   = SavedState.MissionId;
    CurrentOutcomeTag  = SavedState.OutcomeTag;
    StealthScore       = SavedState.StealthScore;
    AlertsTriggered    = SavedState.AlertsTriggered;

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
