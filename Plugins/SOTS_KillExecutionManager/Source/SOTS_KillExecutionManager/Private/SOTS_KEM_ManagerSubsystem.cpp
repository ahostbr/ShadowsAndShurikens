#include "GameplayTagContainer.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "SOTS_KEMCatalogLibrary.h"
#include "SOTS_KEM_ExecutionCatalog.h"
#include "SOTS_KillExecutionManagerModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_ExecutionHelperActor.h"
#include "SOTS_KEM_OmniTraceBridge.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "SOTS_GAS_AbilityRequirementLibraryAsset.h"
#include "ContextualAnimSceneAsset.h"
#include "MotionWarpingComponent.h"
#include "EngineUtils.h"
#include "SOTS_KEMExecutionAnchor.h"
#include "DrawDebugHelpers.h"
#include "Containers/Set.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"
#include "SOTS_KEM_OmniTraceTuning.h"

DEFINE_LOG_CATEGORY(LogSOTS_KEM);

namespace
{
    // Maximum number of debug records to keep in memory for UI.
    constexpr int32 KEM_MaxDebugRecords = 10;
    constexpr float KEM_DefaultAnchorSearchRadius = 400.f;

    static TAutoConsoleVariable<int32> CVarSOTSKEMTelemetryLogging(
        TEXT("sots.kem.telemetryLogging"),
        0,
        TEXT("Enable KEM execution telemetry logging (1=on)."),
        ECVF_Default);

    constexpr TCHAR KEMTelemetryLogPrefix[] = TEXT("[KEM Telemetry]");

    static const FName KEMTag_GroundRear(TEXT("SOTS.KEM.Position.Ground.Rear"));
    static const FName KEMTag_GroundFront(TEXT("SOTS.KEM.Position.Ground.Front"));
    static const FName KEMTag_GroundLeft(TEXT("SOTS.KEM.Position.Ground.Left"));
    static const FName KEMTag_GroundRight(TEXT("SOTS.KEM.Position.Ground.Right"));
    static const FName KEMTag_LegacyRear(TEXT("SOTS.KEM.Position.Rear"));
    static const FName KEMTag_LegacyFront(TEXT("SOTS.KEM.Position.Front"));
    static const FName KEMTag_LegacyLeft(TEXT("SOTS.KEM.Position.Left"));
    static const FName KEMTag_LegacyRight(TEXT("SOTS.KEM.Position.Right"));
    static const FName KEMTag_CornerLeft(TEXT("SOTS.KEM.Position.Corner.Left"));
    static const FName KEMTag_CornerRight(TEXT("SOTS.KEM.Position.Corner.Right"));
    static const FName KEMTag_Corner(TEXT("SOTS.KEM.Position.Corner"));
    static const FName KEMTag_Vertical(TEXT("SOTS.KEM.Position.Vertical"));
    static const FName KEMTag_VerticalAbove(TEXT("SOTS.KEM.Position.Vertical.Above"));
    static const FName KEMTag_VerticalBelow(TEXT("SOTS.KEM.Position.Vertical.Below"));

    bool TagNameEquals(const FGameplayTag& Tag, FName TagName)
    {
        return Tag.IsValid() && Tag.GetTagName() == TagName;
    }

    bool ContainsTagName(const TArray<FGameplayTag>& Tags, FName TagName)
    {
        for (const FGameplayTag& Tag : Tags)
        {
            if (TagNameEquals(Tag, TagName))
            {
                return true;
            }
        }
        return false;
    }

    FGameplayTag RequestCanonicalTag(FName TagName)
    {
        return FGameplayTag::RequestGameplayTag(TagName, false);
    }

    FLinearColor GetColorForFamily(ESOTS_KEM_ExecutionFamily Family)
    {
        switch (Family)
        {
        case ESOTS_KEM_ExecutionFamily::GroundRear:
            return FLinearColor::Red;
        case ESOTS_KEM_ExecutionFamily::GroundFront:
            return FLinearColor::Green;
        case ESOTS_KEM_ExecutionFamily::GroundLeft:
            return FLinearColor(1.0f, 0.7f, 0.0f);
        case ESOTS_KEM_ExecutionFamily::GroundRight:
            return FLinearColor(0.0f, 0.75f, 0.75f);
        case ESOTS_KEM_ExecutionFamily::VerticalAbove:
            return FLinearColor::Blue;
        case ESOTS_KEM_ExecutionFamily::VerticalBelow:
            return FLinearColor(0.6f, 0.2f, 0.7f);
        case ESOTS_KEM_ExecutionFamily::CornerLeft:
            return FLinearColor(1.0f, 0.2f, 0.65f);
        case ESOTS_KEM_ExecutionFamily::CornerRight:
            return FLinearColor(0.8f, 0.4f, 0.2f);
        case ESOTS_KEM_ExecutionFamily::Special:
            return FLinearColor(1.0f, 0.55f, 0.0f);
        default:
            return FLinearColor::White;
        }
    }

    ESOTS_OmniTraceKEMPreset DetermineKEMPresetForTag(const FGameplayTag& PositionTag)
    {
        if (!PositionTag.IsValid())
        {
            return ESOTS_OmniTraceKEMPreset::Unknown;
        }

        const FName TagName = PositionTag.GetTagName();
        if (TagName == KEMTag_GroundRear || TagName == KEMTag_LegacyRear)
        {
            return ESOTS_OmniTraceKEMPreset::GroundRear;
        }
        if (TagName == KEMTag_GroundFront || TagName == KEMTag_LegacyFront)
        {
            return ESOTS_OmniTraceKEMPreset::GroundFront;
        }
        if (TagName == KEMTag_GroundLeft || TagName == KEMTag_LegacyLeft)
        {
            return ESOTS_OmniTraceKEMPreset::GroundLeft;
        }
        if (TagName == KEMTag_GroundRight || TagName == KEMTag_LegacyRight)
        {
            return ESOTS_OmniTraceKEMPreset::GroundRight;
        }
        if (TagName == KEMTag_CornerLeft)
        {
            return ESOTS_OmniTraceKEMPreset::CornerLeft;
        }
        if (TagName == KEMTag_CornerRight)
        {
            return ESOTS_OmniTraceKEMPreset::CornerRight;
        }
        if (TagName == KEMTag_Corner)
        {
            return ESOTS_OmniTraceKEMPreset::CornerLeft;
        }
        if (TagName == KEMTag_Vertical || TagName == KEMTag_VerticalAbove)
        {
            return ESOTS_OmniTraceKEMPreset::VerticalAbove;
        }
        if (TagName == KEMTag_VerticalBelow)
        {
            return ESOTS_OmniTraceKEMPreset::VerticalBelow;
        }
        return ESOTS_OmniTraceKEMPreset::Unknown;
    }

    FGameplayTag DetermineEffectivePositionTag(const USOTS_KEM_ExecutionDefinition* Def)
    {
        if (!Def)
        {
            return FGameplayTag();
        }

        auto MaybeCornerFromTag = [&](const FGameplayTag& PrimaryTag) -> FGameplayTag
        {
            if (!PrimaryTag.IsValid())
            {
                return FGameplayTag();
            }

            const FName PrimaryName = PrimaryTag.GetTagName();
            if (PrimaryName == KEMTag_GroundRear || PrimaryName == KEMTag_LegacyRear ||
                PrimaryName == KEMTag_GroundFront || PrimaryName == KEMTag_LegacyFront)
            {
                if (ContainsTagName(Def->AdditionalPositionTags, KEMTag_GroundLeft) ||
                    ContainsTagName(Def->AdditionalPositionTags, KEMTag_LegacyLeft))
                {
                    return RequestCanonicalTag(KEMTag_CornerLeft);
                }
                if (ContainsTagName(Def->AdditionalPositionTags, KEMTag_GroundRight) ||
                    ContainsTagName(Def->AdditionalPositionTags, KEMTag_LegacyRight))
                {
                    return RequestCanonicalTag(KEMTag_CornerRight);
                }
            }
            return FGameplayTag();
        };

        if (Def->PositionTag.IsValid())
        {
            if (FGameplayTag CornerTag = MaybeCornerFromTag(Def->PositionTag); CornerTag.IsValid())
            {
                return CornerTag;
            }
            return Def->PositionTag;
        }

        for (const FGameplayTag& Additional : Def->AdditionalPositionTags)
        {
            if (!Additional.IsValid())
            {
                continue;
            }
            if (FGameplayTag CornerTag = MaybeCornerFromTag(Additional); CornerTag.IsValid())
            {
                return CornerTag;
            }
            return Additional;
        }

        return FGameplayTag();
    }

    void GatherExecutionDefinitions(const USOTS_KEMManagerSubsystem* Manager, TArray<USOTS_KEM_ExecutionDefinition*>& OutDefinitions)
    {
        if (!Manager)
        {
            return;
        }

        TSet<USOTS_KEM_ExecutionDefinition*> Seen;
        OutDefinitions.Reset();

        auto AddSoftDefinition = [&](const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef)
        {
            if (!SoftDef.IsValid() && !SoftDef.ToSoftObjectPath().IsValid())
            {
                return;
            }

            USOTS_KEM_ExecutionDefinition* Def = SoftDef.Get();
            if (!Def)
            {
                Def = SoftDef.LoadSynchronous();
            }

            if (Def && !Seen.Contains(Def))
            {
                Seen.Add(Def);
                OutDefinitions.Add(Def);
            }
        };

        for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Manager->ExecutionDefinitions)
        {
            AddSoftDefinition(SoftDef);
        }

        if (USOTS_KEM_ExecutionCatalog* Catalog = USOTS_KEMCatalogLibrary::GetExecutionCatalog(Manager))
        {
            for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Catalog->ExecutionDefinitions)
            {
                AddSoftDefinition(SoftDef);
            }
        }

        auto LoadDefaultRegistryConfig = [&]() -> USOTS_KEM_ExecutionRegistryConfig*
        {
            if (Manager->DefaultRegistryConfig.IsValid())
            {
                return Manager->DefaultRegistryConfig.Get();
            }
            if (Manager->DefaultRegistryConfig.ToSoftObjectPath().IsValid())
            {
                return Manager->DefaultRegistryConfig.LoadSynchronous();
            }
            return nullptr;
        };

        if (USOTS_KEM_ExecutionRegistryConfig* RegistryConfig = LoadDefaultRegistryConfig())
        {
            for (const FSOTS_KEM_ExecutionRegistryEntry& Entry : RegistryConfig->Entries)
            {
                AddSoftDefinition(Entry.ExecutionDefinition);
            }
        }
    }

    FString GetDefinitionDisplayName(const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef, const USOTS_KEM_ExecutionDefinition* Def)
    {
        if (Def)
        {
            if (Def->ExecutionTag.IsValid())
            {
                return Def->ExecutionTag.ToString();
            }
            return Def->GetName();
        }

        if (SoftDef.IsValid())
        {
            const FString AssetName = SoftDef.GetAssetName();
            if (!AssetName.IsEmpty())
            {
                return AssetName;
            }
            return SoftDef.ToSoftObjectPath().ToString();
        }

        const FString Path = SoftDef.ToSoftObjectPath().ToString();
        return Path.IsEmpty() ? TEXT("UnknownExecutionDefinition") : Path;
    }

    FString GetExecutionFamilyName(ESOTS_KEM_ExecutionFamily Family)
    {
        if (const UEnum* Enum = StaticEnum<ESOTS_KEM_ExecutionFamily>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Family));
        }
        return TEXT("Unknown");
    }
}

USOTS_KEMManagerSubsystem::USOTS_KEMManagerSubsystem()
{
    CurrentState = ESOTS_KEMState::Ready;
}

USOTS_KEMManagerSubsystem* USOTS_KEMManagerSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject))
    {
        return GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* WorldGameInstance = World->GetGameInstance())
        {
            return WorldGameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
        }
    }

    return nullptr;
}

void USOTS_KEMManagerSubsystem::SetAbilityRequirementLibrary(USOTS_AbilityRequirementLibraryAsset* InLibrary)
{
    AbilityRequirementLibrary = InLibrary;
}

void USOTS_KEMManagerSubsystem::GetRecentKEMDebugRecords(TArray<FSOTS_KEMDebugRecord>& OutRecords) const
{
    OutRecords = RecentDebugRecords;
}

void USOTS_KEMManagerSubsystem::GetLastKEMCandidateDebug(TArray<FSOTS_KEMCandidateDebugRecord>& OutCandidates) const
{
    OutCandidates = LastCandidateDebug;
}

void USOTS_KEMManagerSubsystem::GetLastDecisionSnapshot(FSOTS_KEMDecisionSnapshot& OutSnapshot) const
{
    OutSnapshot = LastDecisionSnapshot;
}

void USOTS_KEMManagerSubsystem::GetLastKEMDecisionSummary(FString& OutSummary) const
{
    if (LastCandidateDebug.IsEmpty())
    {
        OutSummary = TEXT("No decision history available.");
        return;
    }

    const FString SourceLabel = LastDecisionSnapshot.SourceLabel.IsEmpty()
        ? TEXT("Unknown")
        : LastDecisionSnapshot.SourceLabel;

    const FString RequestedTag = LastDecisionSnapshot.RequestedTag.IsValid()
        ? LastDecisionSnapshot.RequestedTag.ToString()
        : TEXT("None");

    const FSOTS_KEMCandidateDebugRecord* SelectedCandidate = LastCandidateDebug.FindByPredicate(
        [](const FSOTS_KEMCandidateDebugRecord& Candidate)
        {
            return Candidate.bSelected;
        });

    const FString SelectedName = SelectedCandidate ? SelectedCandidate->ExecutionName : TEXT("None");
    const float SelectedScore = SelectedCandidate ? SelectedCandidate->Score : 0.f;

    OutSummary = FString::Printf(
        TEXT("Source=%s Request=%s Candidates=%d Selected=%s Score=%.2f"),
        *SourceLabel,
        *RequestedTag,
        LastCandidateDebug.Num(),
        *SelectedName,
        SelectedScore);
}

void USOTS_KEMManagerSubsystem::GetLastKEMCandidates(TArray<FString>& OutCandidates) const
{
    OutCandidates.Reset();
    OutCandidates.Reserve(LastCandidateDebug.Num());

    const UEnum* RejectEnum = StaticEnum<ESOTS_KEMRejectReason>();

    for (const FSOTS_KEMCandidateDebugRecord& Candidate : LastCandidateDebug)
    {
        const FString Status = Candidate.bSelected ? TEXT("Selected")
            : (Candidate.RejectReason == ESOTS_KEMRejectReason::None ? TEXT("Passed") : TEXT("Rejected"));

        const FString RejectName = RejectEnum
            ? RejectEnum->GetNameStringByValue(static_cast<int64>(Candidate.RejectReason))
            : TEXT("None");

        const FString Reason = Candidate.FailureReason.IsEmpty() ? TEXT("None") : Candidate.FailureReason;

        OutCandidates.Add(FString::Printf(
            TEXT("%s [%s] Reject=%s Score=%.2f Reason=%s"),
            *Candidate.ExecutionName,
            *Status,
            *RejectName,
            Candidate.Score,
            *Reason));
    }
}

void USOTS_KEMManagerSubsystem::NotifyExecutionEnded(bool bSuccess)
{
    EnterCooldownState(bSuccess);
}

void USOTS_KEMManagerSubsystem::NotifyExecutionEnded(
    const FSOTS_ExecutionContext& Context,
    const USOTS_KEM_ExecutionDefinition* Def,
    bool bWasSuccessful)
{
    const ESOTS_KEM_ExecutionResult Result =
        bWasSuccessful ? ESOTS_KEM_ExecutionResult::Succeeded
                       : ESOTS_KEM_ExecutionResult::Failed;

    BroadcastExecutionEvent(Result, Context, Def);
    TriggerExecutionFX(Result, Context, Def);

    if (RecentDebugRecords.IsValidIndex(ActiveDebugRecordIndex))
    {
        FSOTS_KEMDebugRecord& Record = RecentDebugRecords[ActiveDebugRecordIndex];
        Record.Result = Result;
        Record.Phase  = bWasSuccessful ? TEXT("Finished") : TEXT("Failed");
        if (!bWasSuccessful && Record.FailureReason.IsEmpty())
        {
            Record.FailureReason = TEXT("Execution ended with failure");
        }
    }
    ActiveDebugRecordIndex = INDEX_NONE;

    const float DistanceToTarget = FVector::Dist(Context.InstigatorLocation, Context.TargetLocation);
    const FVector ForwardDir = Context.InstigatorForward.GetSafeNormal();
    const FVector ToTarget = Context.TargetLocation - Context.InstigatorLocation;
    const float FacingAngleDeg = (!ToTarget.IsNearlyZero() && !ForwardDir.IsNearlyZero())
        ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ForwardDir, ToTarget.GetSafeNormal()), -1.f, 1.f)))
        : 0.f;
    const float HeightDelta = Context.HeightDelta;
    const FString SourceLabel = LastDecisionSnapshot.SourceLabel.IsEmpty()
        ? TEXT("Unknown")
        : LastDecisionSnapshot.SourceLabel;

    if (bHasPendingExecutionTelemetry)
    {
        PendingExecutionTelemetry.Outcome = bWasSuccessful
            ? ESOTS_KEMExecutionOutcome::Success
            : ESOTS_KEMExecutionOutcome::Failed_InternalError;
        BroadcastExecutionTelemetry(PendingExecutionTelemetry);
        ResetPendingTelemetry();
    }
    else
    {
        FSOTS_KEMExecutionTelemetry FallbackTelemetry = BuildTelemetryRecord(
            Def,
            Context,
            SourceLabel,
            DistanceToTarget,
            FacingAngleDeg,
            HeightDelta,
            0.f,
            nullptr,
            false);
        FallbackTelemetry.Outcome = bWasSuccessful
            ? ESOTS_KEMExecutionOutcome::Success
            : ESOTS_KEMExecutionOutcome::Failed_InternalError;
        BroadcastExecutionTelemetry(FallbackTelemetry);
    }

    EnterCooldownState(bWasSuccessful);
}

void USOTS_KEMManagerSubsystem::ForceResetState()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }

    CurrentState = ESOTS_KEMState::Ready;
}

void USOTS_KEMManagerSubsystem::BroadcastExecutionEvent(
    ESOTS_KEM_ExecutionResult Result,
    const FSOTS_ExecutionContext& Context,
    const USOTS_KEM_ExecutionDefinition* Def) const
{
    FSOTS_KEM_ExecutionEvent Event;
    Event.Result       = Result;
    Event.Instigator   = Context.Instigator;
    Event.Target       = Context.Target;
    Event.ExecutionTag = Def ? Def->ExecutionTag : FGameplayTag();
    Event.Definition   = Def;

    OnExecutionEvent.Broadcast(Event);
}

FSOTS_OnKEMExecutionTelemetry& USOTS_KEMManagerSubsystem::GetTelemetryDelegate()
{
    return OnExecutionTelemetry;
}

void USOTS_KEMManagerSubsystem::BroadcastExecutionTelemetry(const FSOTS_KEMExecutionTelemetry& Telemetry)
{
    OnExecutionTelemetry.Broadcast(Telemetry);
    OnExecutionTelemetryBP.Broadcast(Telemetry);

    if (!IsTelemetryLoggingEnabled())
    {
        return;
    }

    const UEnum* OutcomeEnum = StaticEnum<ESOTS_KEMExecutionOutcome>();
    const FString OutcomeName = OutcomeEnum
        ? OutcomeEnum->GetNameStringByValue(static_cast<int64>(Telemetry.Outcome))
        : TEXT("Unknown");

    const FString SourceLabel = Telemetry.SourceLabel.IsEmpty()
        ? TEXT("Unknown")
        : Telemetry.SourceLabel;

    const FString ExecutionTag = Telemetry.ExecutionTag.IsValid()
        ? Telemetry.ExecutionTag.ToString()
        : TEXT("None");

    const FString FamilyTag = Telemetry.ExecutionFamilyTag.IsValid()
        ? Telemetry.ExecutionFamilyTag.ToString()
        : TEXT("None");

    const FString PositionTag = Telemetry.ExecutionPositionTag.IsValid()
        ? Telemetry.ExecutionPositionTag.ToString()
        : TEXT("None");

    const FString InstigatorName = Telemetry.Instigator.IsValid()
        ? Telemetry.Instigator->GetName()
        : TEXT("None");

    const FString TargetName = Telemetry.Target.IsValid()
        ? Telemetry.Target->GetName()
        : TEXT("None");

    UE_LOG(LogSOTS_KEM, Log,
        TEXT("%s Outcome=%s Tag=%s Family=%s Position=%s Dist=%.1f Stealth=%d Anchor=%s OmniTrace=%s Dragon=%s Cutscene=%s Source=%s Instigator=%s Target=%s"),
        KEMTelemetryLogPrefix,
        *OutcomeName,
        *ExecutionTag,
        *FamilyTag,
        *PositionTag,
        Telemetry.DistanceToTarget,
        Telemetry.StealthTierAtRequest,
        Telemetry.bUsedAnchor ? TEXT("true") : TEXT("false"),
        Telemetry.bUsedOmniTrace ? TEXT("true") : TEXT("false"),
        Telemetry.bDragonControlled ? TEXT("true") : TEXT("false"),
        Telemetry.bFromCutscene ? TEXT("true") : TEXT("false"),
        *SourceLabel,
        *InstigatorName,
        *TargetName);

    for (const FSOTS_KEMDecisionStep& Step : Telemetry.DecisionSteps)
    {
        const FString Detail = Step.Detail.IsEmpty() ? TEXT("None") : Step.Detail;
        UE_LOG(LogSOTS_KEM, Log,
            TEXT("%s Step=%s Passed=%s Value=%.2f Detail=%s"),
            KEMTelemetryLogPrefix,
            *Step.StepName.ToString(),
            Step.bPassed ? TEXT("true") : TEXT("false"),
            Step.NumericValue,
            *Detail);
    }
}

void USOTS_KEMManagerSubsystem::TriggerExecutionFX(
    ESOTS_KEM_ExecutionResult Result,
    const FSOTS_ExecutionContext& Context,
    const USOTS_KEM_ExecutionDefinition* Def) const
{
    if (!Def)
    {
        return;
    }

    FGameplayTag FXTag;

    switch (Result)
    {
    case ESOTS_KEM_ExecutionResult::Started:
        FXTag = Def->FXTag_OnExecutionStarted;
        break;
    case ESOTS_KEM_ExecutionResult::Succeeded:
        FXTag = Def->FXTag_OnExecutionSucceeded;
        break;
    case ESOTS_KEM_ExecutionResult::Failed:
        FXTag = Def->FXTag_OnExecutionFailed;
        break;
    default:
        break;
    }

    if (!FXTag.IsValid())
    {
        return;
    }

    if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
    {
        AActor* InstActor   = Context.Instigator.Get();
        AActor* TargetActor = Context.Target.Get();
        const FVector FXLoc = TargetActor ? Context.TargetLocation : Context.InstigatorLocation;

        FX->TriggerFXByTag(
            GetWorld(),
            FXTag,
            InstActor,
            TargetActor,
            FXLoc,
            FRotator::ZeroRotator);
    }
}

void USOTS_KEMManagerSubsystem::BuildExecutionContext(
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    FSOTS_ExecutionContext& OutContext) const
{
    OutContext = FSOTS_ExecutionContext();

    if (!Instigator || !Target)
    {
        return;
    }

    OutContext.Instigator = Instigator;
    OutContext.Target = Target;
    OutContext.ContextTags = ContextTags;

    OutContext.InstigatorLocation = Instigator->GetActorLocation();
    OutContext.TargetLocation = Target->GetActorLocation();

    OutContext.InstigatorForward = Instigator->GetActorForwardVector();
    OutContext.TargetForward = Target->GetActorForwardVector();

    OutContext.HeightDelta = OutContext.TargetLocation.Z - OutContext.InstigatorLocation.Z;

    // Pull the latest global stealth snapshot so KEM can make informed
    // decisions (e.g., enabling special stealth executions only when the
    // player is sufficiently hidden).
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(Instigator))
    {
        const FSOTS_PlayerStealthState& StealthState = GSM->GetStealthState();
        OutContext.GlobalStealthScore01 = StealthState.GlobalStealthScore01;
        OutContext.ShadowLevel01       = StealthState.ShadowLevel01;
        OutContext.StealthTier         = StealthState.StealthTier;
    }

    if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
    {
        UE_LOG(LogSOTS_KEM, Log,
            TEXT("[KEM] BuildExecutionContext: Instigator=%s Target=%s Tags=%s"),
            *GetNameSafe(Instigator),
            *GetNameSafe(Target),
            *ContextTags.ToStringSimple());
    }
}

bool USOTS_KEMManagerSubsystem::EvaluateTagsAndHeight(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason) const
{
    OutRejectReason = ESOTS_KEMRejectReason::None;

    if (!Def)
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissingDefinition;
        OutFailReason = TEXT("Definition is null");
        return false;
    }

    if (!Context.ContextTags.HasAll(Def->RequiredContextTags))
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissionTagMismatch;
        OutFailReason = TEXT("Missing required context tags");
        return false;
    }

    if (Def->BlockedContextTags.Num() > 0 && Context.ContextTags.HasAny(Def->BlockedContextTags))
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissionTagMismatch;
        OutFailReason = TEXT("Blocked by context tags");
        return false;
    }

    const float AbsHeight = FMath::Abs(Context.HeightDelta);

    switch (Def->HeightMode)
    {
    case ESOTS_KEM_HeightMode::SamePlaneOnly:
        {
            const float MaxDelta = Def->CASConfig.MaxSamePlaneHeightDelta;
            if (AbsHeight > MaxDelta)
            {
                OutRejectReason = ESOTS_KEMRejectReason::HeightModeMismatch;
                OutFailReason = FString::Printf(TEXT("HeightDelta %.1f > MaxSamePlaneHeightDelta %.1f"),
                                                AbsHeight, MaxDelta);
                return false;
            }
            break;
        }
    case ESOTS_KEM_HeightMode::VerticalOnly:
        {
            const float MinVertical = Def->CASConfig.MaxSamePlaneHeightDelta;
            if (AbsHeight < MinVertical)
            {
                OutRejectReason = ESOTS_KEMRejectReason::HeightModeMismatch;
                OutFailReason = FString::Printf(TEXT("HeightDelta %.1f < MinVertical %.1f"),
                                                AbsHeight, MinVertical);
                return false;
            }
            break;
        }
    case ESOTS_KEM_HeightMode::Any:
    default:
        {
            break;
        }
    }

    return true;
}

bool USOTS_KEMManagerSubsystem::EvaluateCASDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    FSOTS_CASQueryResult& OutResult,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason) const
{
    OutRejectReason = ESOTS_KEMRejectReason::None;
    OutResult = FSOTS_CASQueryResult();

    if (!Def)
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissingDefinition;
        OutFailReason = TEXT("Definition is null");
        return false;
    }

    const FSOTS_KEM_CASConfig& CASCfg = Def->CASConfig;

    if (!CASCfg.Scene.IsValid() && !CASCfg.Scene.ToSoftObjectPath().IsValid())
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("CAS Scene is not set");
        return false;
    }

    const float AbsHeight = FMath::Abs(Context.HeightDelta);
    if (AbsHeight > CASCfg.MaxSamePlaneHeightDelta)
    {
        OutRejectReason = ESOTS_KEMRejectReason::HeightModeMismatch;
        OutFailReason = FString::Printf(TEXT("HeightDelta %.1f > CAS MaxSamePlaneHeightDelta %.1f"),
                                        AbsHeight, CASCfg.MaxSamePlaneHeightDelta);
        return false;
    }

    const float DistSq = FVector::DistSquared(Context.InstigatorLocation, Context.TargetLocation);
    const float MinDistSq = FMath::Square(CASCfg.MinDistance);
    const float MaxDistSq = FMath::Square(CASCfg.MaxDistance);

    if (DistSq < MinDistSq || DistSq > MaxDistSq)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DistanceOutOfRange;
        OutFailReason = FString::Printf(TEXT("Distance^2 %.1f not in [%.1f, %.1f]"),
                                        DistSq, MinDistSq, MaxDistSq);
        return false;
    }

    const FVector ToTarget = (Context.TargetLocation - Context.InstigatorLocation).GetSafeNormal();
    const FVector InstForward = Context.InstigatorForward.GetSafeNormal();

    const float Dot = FVector::DotProduct(InstForward, ToTarget);
    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    if (AngleDegrees > CASCfg.MaxFacingAngleDegrees)
    {
        OutRejectReason = ESOTS_KEMRejectReason::AngleOutOfRange;
        OutFailReason = FString::Printf(TEXT("Angle %.1f > MaxFacingAngleDegrees %.1f"),
                                        AngleDegrees, CASCfg.MaxFacingAngleDegrees);
        return false;
    }

    UContextualAnimSceneAsset* Scene = CASCfg.Scene.Get();
    if (!Scene)
    {
        Scene = CASCfg.Scene.LoadSynchronous();
    }
    if (!Scene)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("Failed to load CAS Scene");
        return false;
    }

    const FSOTS_KEM_CASOffsetConfig& OffsetCfg = CASCfg.OffsetConfig;
    const FVector TargetLoc = Context.TargetLocation;
    const FRotator TargetRot = Context.TargetForward.Rotation();
    const FTransform TargetXf(TargetRot, TargetLoc);

    const FVector LocalOffset = OffsetCfg.InstigatorLocalOffsetFromTarget;
    const FRotator LocalRotOffset = OffsetCfg.InstigatorLocalRotationOffset;
    const FTransform LocalXf(LocalRotOffset, LocalOffset);
    const FTransform WarpTarget = LocalXf * TargetXf;

    const float WarpDistSq = FVector::DistSquared(Context.InstigatorLocation, WarpTarget.GetLocation());
    const float MaxWarpDistSq = FMath::Square(OffsetCfg.MaxWarpDistance);

    if (WarpDistSq > MaxWarpDistSq)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DistanceOutOfRange;
        OutFailReason = FString::Printf(TEXT("WarpDist^2 %.1f > MaxWarpDist^2 %.1f"),
                                        WarpDistSq, MaxWarpDistSq);
        return false;
    }

    FTransform InstEntry(Context.InstigatorForward.Rotation(), Context.InstigatorLocation);
    FTransform TargetEntry(Context.TargetForward.Rotation(), Context.TargetLocation);

    OutResult.bIsValid = true;
    OutResult.Scene = Scene;
    OutResult.SectionName = CASCfg.SectionName;
    OutResult.InstigatorRole = CASCfg.InstigatorRoleName;
    OutResult.TargetRole = CASCfg.TargetRoleName;
    OutResult.InstigatorEntryTransform = InstEntry;
    OutResult.TargetEntryTransform = TargetEntry;
    OutResult.InstigatorWarpTarget = WarpTarget;

    OutFailReason = TEXT("OK");
    return true;
}

bool USOTS_KEMManagerSubsystem::EvaluateSequenceDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason) const
{
    OutRejectReason = ESOTS_KEMRejectReason::None;

    if (!Def)
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissingDefinition;
        OutFailReason = TEXT("LevelSequence: Definition is null");
        return false;
    }

    const FSOTS_KEM_LevelSequenceConfig& Cfg = Def->LevelSequenceConfig;

    if (!Cfg.SequenceAsset.IsValid() && !Cfg.SequenceAsset.ToSoftObjectPath().IsValid())
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("LevelSequence: SequenceAsset is not set");
        return false;
    }

    OutFailReason = TEXT("LevelSequence: Passed all checks");
    return true;
}

bool USOTS_KEMManagerSubsystem::EvaluateAISDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason) const
{
    OutRejectReason = ESOTS_KEMRejectReason::None;

    if (!Def)
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissingDefinition;
        OutFailReason = TEXT("AIS: Definition is null");
        return false;
    }

    const FSOTS_KEM_AISConfig& Cfg = Def->AISConfig;

    if (!Cfg.BehaviorTag.IsValid())
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("AIS: BehaviorTag is not set");
        return false;
    }

    OutFailReason = TEXT("AIS: Passed all checks");
    return true;
}


bool USOTS_KEMManagerSubsystem::EvaluateSpawnDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason) const
{
    OutRejectReason = ESOTS_KEMRejectReason::None;

    if (!Def)
    {
        OutRejectReason = ESOTS_KEMRejectReason::MissingDefinition;
        OutFailReason = TEXT("SpawnActor: Definition is null");
        return false;
    }

    const FSOTS_KEM_SpawnActorConfig& Cfg = Def->SpawnActorConfig;

    if (!Cfg.HelperClass)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("SpawnActor: HelperClass is not set");
        return false;
    }

    const USOTS_SpawnExecutionData* SpawnData = Cfg.ExecutionData.Get();
    if (!SpawnData && Cfg.ExecutionData.IsValid())
    {
        SpawnData = Cfg.ExecutionData.LoadSynchronous();
    }

    if (!SpawnData)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("SpawnActor: ExecutionData is not set");
        return false;
    }

    if (!SpawnData->InstigatorMontage)
    {
        OutRejectReason = ESOTS_KEMRejectReason::DataIncomplete;
        OutFailReason = TEXT("SpawnActor: InstigatorMontage missing in ExecutionData");
        return false;
    }

    if (Def->WarpPoints.Num() > 0)
    {
        FTransform Dummy;
        const FSOTS_KEM_WarpPointDef& FirstWarpDef = Def->WarpPoints[0];
        const FName DefaultWarpTarget = FirstWarpDef.WarpTargetName;
        if (!ResolveWarpPointByName(Def, DefaultWarpTarget, Context, Dummy, /*bIgnoreDistanceCheck=*/true))
        {
            OutRejectReason = ESOTS_KEMRejectReason::WarpPointMissing;
            OutFailReason = FString::Printf(
                TEXT("SpawnActor: Warp target '%s' could not be resolved (definition has %d warp points)"),
                *DefaultWarpTarget.ToString(),
                Def->WarpPoints.Num());
            return false;
        }
    }

    OutFailReason = TEXT("SpawnActor: Passed all checks");
    return true;
}

bool USOTS_KEMManagerSubsystem::EvaluateAbilityRequirementsForExecution(
    const UObject* WorldContextObject,
    const USOTS_KEM_ExecutionDefinition* ExecutionDef,
    FSOTS_AbilityRequirementCheckResult& OutResult) const
{
    OutResult = FSOTS_AbilityRequirementCheckResult();

    if (!ExecutionDef || !ExecutionDef->bUseAbilityRequirements)
    {
        OutResult.bMeetsAllRequirements = true;
        return true;
    }

    const UObject* ContextObject = WorldContextObject ? WorldContextObject : this;

    const FSOTS_AbilityRequirements& InlineReqs = ExecutionDef->InlineAbilityRequirements;

    const bool bHasInlineReqs =
        InlineReqs.RequiredSkillTags.Num() > 0 ||
        InlineReqs.RequiredPlayerTags.Num() > 0 ||
        InlineReqs.MinStealthTier >= 0 ||
        InlineReqs.MaxStealthTier >= 0 ||
        InlineReqs.bDisallowWhenCompromised ||
        InlineReqs.MaxStealthScore01 >= 0.0f;

    if (bHasInlineReqs)
    {
        OutResult = USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirements(
            ContextObject,
            InlineReqs);
        return OutResult.bMeetsAllRequirements;
    }

    if (!AbilityRequirementLibrary || !ExecutionDef->AbilityRequirementTag.IsValid())
    {
        OutResult.bMeetsAllRequirements = true;
        return true;
    }

    OutResult = USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirementsFromLibrary(
        ContextObject,
        AbilityRequirementLibrary,
        ExecutionDef->AbilityRequirementTag);

    return OutResult.bMeetsAllRequirements;
}


FSOTS_KEMValidationResult USOTS_KEMManagerSubsystem::ValidateExecutionDefinition(
    const USOTS_KEM_ExecutionDefinition* Def) const
{
    FSOTS_KEMValidationResult Result;

    if (!Def)
    {
        Result.AddError(TEXT("ExecutionDefinition is null"));
        return Result;
    }

    Result = Def->ValidateDefinition();

    const ESOTS_KEM_PositionKind PositionKind = Def->GetPositionKind();

    if (Def->ExecutionFamily != ESOTS_KEM_ExecutionFamily::Unknown &&
        PositionKind == ESOTS_KEM_PositionKind::Unknown &&
        Def->PositionTag.IsValid())
    {
        Result.AddWarning(TEXT("ExecutionFamily set but PositionTag is not recognized."));
    }

    if (Def->ExecutionFamily == ESOTS_KEM_ExecutionFamily::Unknown &&
        PositionKind != ESOTS_KEM_PositionKind::Unknown)
    {
        Result.AddWarning(TEXT("PositionTag is set but ExecutionFamily is Unknown."));
    }

    return Result;
}

bool USOTS_KEMManagerSubsystem::TryPlayFallbackMontage(AActor* Instigator) const
{
    if (!FallbackMontage || !Instigator)
    {
        return false;
    }

    if (USkeletalMeshComponent* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
    {
        if (UAnimInstance* Anim = Mesh->GetAnimInstance())
        {
            if (Anim->Montage_Play(FallbackMontage, 1.0f) > 0.f)
            {
                if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
                {
                    UE_LOG(LogSOTS_KEM, Log,
                        TEXT("[KEM][Fallback] Played fallback montage '%s' for Instigator=%s"),
                        *FallbackMontage->GetName(),
                        *Instigator->GetName());
                }
                return true;
            }
        }
    }

    if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
    {
        UE_LOG(LogSOTS_KEM, Verbose,
            TEXT("[KEM][Fallback] Unable to play montage '%s' on Instigator=%s"),
            *FallbackMontage->GetName(),
            *GetNameSafe(Instigator));
    }

    return false;
}


bool USOTS_KEMManagerSubsystem::ResolveWarpPointByName(
    const USOTS_KEM_ExecutionDefinition* Def,
    FName WarpTargetName,
    const FSOTS_ExecutionContext& Context,
    FTransform& OutTransform,
    bool bIgnoreDistanceCheck) const
{
    OutTransform = FTransform::Identity;

    if (!Def || WarpTargetName.IsNone())
    {
        return false;
    }

    const FSOTS_KEM_WarpPointDef* Found = nullptr;
    for (const FSOTS_KEM_WarpPointDef& WarpDef : Def->WarpPoints)
    {
        if (WarpDef.WarpTargetName == WarpTargetName)
        {
            Found = &WarpDef;
            break;
        }
    }

    if (!Found)
    {
        return false;
    }

    const FSOTS_KEM_WarpPointDef& WarpDef = *Found;

    const AActor* InstActor = Context.Instigator.Get();
    const AActor* TargetActor = Context.Target.Get();
    if (!InstActor || !TargetActor)
    {
        return false;
    }

    auto GetRefTransform = [&](ESOTS_KEM_WarpRef Ref, const AActor* InInst, const AActor* InTarget, const FSOTS_KEM_WarpPointDef& Def)->FTransform
    {
        const AActor* SourceActor = nullptr;
        switch (Ref)
        {
        case ESOTS_KEM_WarpRef::Instigator:
        case ESOTS_KEM_WarpRef::InstigatorBone:
            SourceActor = InInst;
            break;
        case ESOTS_KEM_WarpRef::Target:
        case ESOTS_KEM_WarpRef::TargetBone:
            SourceActor = InTarget;
            break;
        default:
            SourceActor = InTarget;
            break;
        }

        FTransform Base = SourceActor ? SourceActor->GetActorTransform() : FTransform::Identity;

        if (USkeletalMeshComponent* Skel = SourceActor ? SourceActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr)
        {
            if (!Def.BoneOrSocketName.IsNone() &&
                (Ref == ESOTS_KEM_WarpRef::InstigatorBone || Ref == ESOTS_KEM_WarpRef::TargetBone))
            {
                const FTransform BoneXf = Skel->GetSocketTransform(Def.BoneOrSocketName);
                Base = BoneXf;
            }
        }

        return Base;
    };

    FTransform RefXf = GetRefTransform(WarpDef.Reference, InstActor, TargetActor, WarpDef);

    // Apply local offset & rotation
    FTransform LocalXf(WarpDef.LocalRotationOffset, WarpDef.LocalOffset);
    OutTransform = LocalXf * RefXf;

    // Distance sanity check
    const FVector InstLoc = InstActor->GetActorLocation();
    const float DistSq = FVector::DistSquared(OutTransform.GetLocation(), InstLoc);
    if (!bIgnoreDistanceCheck &&
        WarpDef.MaxWarpDistance > 0.f &&
        DistSq > FMath::Square(WarpDef.MaxWarpDistance))
    {
        return false;
    }

    return true;
}

const FSOTS_KEM_OmniTraceTuning* USOTS_KEMManagerSubsystem::FindOmniTraceTuning(ESOTS_OmniTraceKEMPreset Preset) const
{
    if (Preset == ESOTS_OmniTraceKEMPreset::Unknown)
    {
        return nullptr;
    }

    const USOTS_KEM_OmniTraceTuningConfig* Config = OmniTraceTuningConfig.Get();
    if (!Config && OmniTraceTuningConfig.IsValid())
    {
        Config = OmniTraceTuningConfig.LoadSynchronous();
    }

    if (!Config)
    {
        return nullptr;
    }

    for (const FSOTS_KEM_OmniTraceTuning& Entry : Config->Entries)
    {
        if (Entry.PresetId == Preset)
        {
            return &Entry;
        }
    }

    return nullptr;
}

bool USOTS_KEMManagerSubsystem::ExecuteSpawnActorBackend(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    const ASOTS_KEMExecutionAnchor* Anchor) const
{
    UWorld* World = GetWorld();
    if (!Def || !World)
    {
        return false;
    }

    const FSOTS_KEM_SpawnActorConfig& Cfg = Def->SpawnActorConfig;
    if (!Cfg.HelperClass || !Cfg.ExecutionData.IsValid())
    {
        if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
        {
            UE_LOG(LogSOTS_KEM, Warning,
                TEXT("[KEM][Spawn] ExecuteSpawnActorBackend: Missing HelperClass or ExecutionData on Def=%s"),
                Def ? *Def->GetName() : TEXT("None"));
        }
        return false;
    }

    if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
    {
        UE_LOG(LogSOTS_KEM, Log,
            TEXT("[KEM][Spawn] ExecuteSpawnActorBackend: Def=%s HelperClass=%s UseOmniTrace=%s"),
            *Def->GetName(),
            *GetNameSafe(Cfg.HelperClass.Get()),
            Cfg.bUseOmniTraceForWarp ? TEXT("true") : TEXT("false"));
    }

    // Base spawn transform: start at the target, facing target forward.
    FTransform SpawnXf = FTransform::Identity;

    if (Anchor)
    {
        SpawnXf = Anchor->GetActorTransform();
    }
    else if (Context.Target.IsValid())
    {
        SpawnXf.SetLocation(Context.TargetLocation);
        SpawnXf.SetRotation(Context.TargetForward.Rotation().Quaternion());
    }

    // Optional OmniTrace refinement.
    FSOTS_KEM_OmniTraceWarpResult WarpResult;
    if (Cfg.bUseOmniTraceForWarp)
    {
        const FGameplayTag EffectiveTag = DetermineEffectivePositionTag(Def);
        ESOTS_OmniTraceKEMPreset SelectedPreset = Cfg.OmniTracePreset;
        if (SelectedPreset == ESOTS_OmniTraceKEMPreset::Unknown)
        {
            SelectedPreset = DetermineKEMPresetForTag(EffectiveTag);
        }

        const FSOTS_OmniTraceKEMPresetEntry* PresetEntry = nullptr;
        FSOTS_OmniTraceKEMPresetEntry TunedPresetEntry;
        const FSOTS_OmniTraceKEMPresetEntry* EntryToUse = nullptr;
        const FSOTS_KEM_OmniTraceTuning* AppliedTuning = nullptr;

        if (OmniTracePresetLibrary)
        {
            PresetEntry = OmniTracePresetLibrary->FindEntry(SelectedPreset);
        }

        if (PresetEntry)
        {
            AppliedTuning = FindOmniTraceTuning(SelectedPreset);
            if (AppliedTuning)
            {
                TunedPresetEntry = *PresetEntry;
                if (AppliedTuning->MaxDistanceOverride > 0.f)
                {
                    TunedPresetEntry.TraceDistance = AppliedTuning->MaxDistanceOverride;
                }
                if (AppliedTuning->MaxVerticalOffsetOverride > 0.f)
                {
                    TunedPresetEntry.VerticalTraceDistance = AppliedTuning->MaxVerticalOffsetOverride;
                }
                EntryToUse = &TunedPresetEntry;
            }
            else
            {
                EntryToUse = PresetEntry;
            }
        }

        FSOTS_KEM_OmniTraceBridge OmniTraceBridge;
        if (EntryToUse)
        {
            OmniTraceBridge.ConfigureForPresetEntry(EntryToUse);
        }
        else
        {
            OmniTraceBridge.ConfigureForPositionTag(EffectiveTag);
        }

        const bool bWarpOk = OmniTraceBridge.ComputeWarpForSpawnExecution(
            Cfg,
            Def,
            Context,
            SpawnXf,
            WarpResult);

        if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
        {
            UE_LOG(LogSOTS_KEM, Log,
                TEXT("[KEM][Spawn] OmniTrace: bOk=%s bHasHelperSpawn=%s NumWarpTargets=%d HelperLoc=%s"),
                bWarpOk ? TEXT("true") : TEXT("false"),
                WarpResult.bHasHelperSpawn ? TEXT("true") : TEXT("false"),
                WarpResult.WarpTargets.Num(),
                *WarpResult.HelperSpawnTransform.GetLocation().ToString());
        }
    }

    // Spawn the helper actor deferred so it can consume the final spawn transform.
    ASOTS_ExecutionHelperActor* Helper =
        World->SpawnActorDeferred<ASOTS_ExecutionHelperActor>(
            Cfg.HelperClass, SpawnXf, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Helper)
    {
        if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
        {
            UE_LOG(LogSOTS_KEM, Warning,
                TEXT("[KEM][Spawn] Failed to spawn helper for Def=%s"), *Def->GetName());
        }
        return false;
    }

    if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
    {
        UE_LOG(LogSOTS_KEM, Log,
            TEXT("[KEM][Spawn] Helper spawned: %s at %s"),
            *GetNameSafe(Helper),
            *SpawnXf.GetLocation().ToString());
    }

    // Pass OmniTrace warp result through to the helper.
    Helper->Initialize(Context, Cfg.ExecutionData.Get(), SpawnXf, Def, WarpResult);
    UGameplayStatics::FinishSpawningActor(Helper, SpawnXf);

    // If we have a runtime warp target from OmniTrace, push it into the
    // MotionWarping component on the instigator so the execution montage
    // can consume it without Blueprint glue.
    if (Context.Instigator.IsValid() && WarpResult.WarpTargets.Num() > 0)
    {
        AActor* InstActor = Context.Instigator.Get();
        if (UMotionWarpingComponent* MotionWarp = InstActor->FindComponentByClass<UMotionWarpingComponent>())
        {
            const FSOTS_KEM_WarpRuntimeTarget& RuntimeTarget = WarpResult.WarpTargets[0];
            MotionWarp->AddOrUpdateWarpTargetFromTransform(RuntimeTarget.TargetName, RuntimeTarget.TargetTransform);

            if (IsDebugAtLeast(EKEMDebugVerbosity::Basic))
            {
                UE_LOG(LogSOTS_KEM, Log,
                    TEXT("[KEM][Spawn] MotionWarping target set: Name=%s Loc=%s"),
                    *RuntimeTarget.TargetName.ToString(),
                    *RuntimeTarget.TargetTransform.GetLocation().ToString());
            }
        }
    }

    return true;
}

void USOTS_KEMManagerSubsystem::EnterCooldownState(bool bWasSuccessful)
{
    const float Duration = bWasSuccessful ? SuccessCooldownDuration : FailureCooldownDuration;

    if (Duration <= 0.f)
    {
        // Immediate reset to Ready; no timers required.
        CurrentState = ESOTS_KEMState::Ready;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(CooldownTimerHandle);
        }
        return;
    }

    CurrentState = bWasSuccessful ? ESOTS_KEMState::SuccessCooldown : ESOTS_KEMState::FailureCooldown;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CooldownTimerHandle,
            this,
            &USOTS_KEMManagerSubsystem::OnCooldownExpired,
            Duration,
            false);
    }
    else
    {
        // If there is no world, just snap back to Ready to avoid getting stuck.
        CurrentState = ESOTS_KEMState::Ready;
    }
}

void USOTS_KEMManagerSubsystem::OnCooldownExpired()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }

    CurrentState = ESOTS_KEMState::Ready;
}

bool USOTS_KEMManagerSubsystem::EvaluateDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    float& OutScore,
    FSOTS_CASQueryResult& OutCASResult,
    ESOTS_KEMRejectReason& OutRejectReason,
    FString& OutFailReason)
{
    OutScore = 0.f;
    OutRejectReason = ESOTS_KEMRejectReason::None;
    OutFailReason = FString();

    const FString ExecName = (Def && Def->ExecutionTag.IsValid())
        ? Def->ExecutionTag.ToString()
        : (Def ? Def->GetName() : FString(TEXT("None")));

    if (!EvaluateTagsAndHeight(Def, Context, OutRejectReason, OutFailReason))
    {
        LogDebugForDefinition(Def, Context, false, OutFailReason);
        return false;
    }

    switch (Def->BackendType)
    {
    case ESOTS_KEM_BackendType::CAS:
        if (!EvaluateCASDefinition(Def, Context, OutCASResult, OutRejectReason, OutFailReason))
        {
            LogDebugForDefinition(Def, Context, false, OutFailReason);
            return false;
        }
        break;
    case ESOTS_KEM_BackendType::LevelSequence:
        if (!EvaluateSequenceDefinition(Def, Context, OutRejectReason, OutFailReason))
        {
            LogDebugForDefinition(Def, Context, false, OutFailReason);
            return false;
        }
        break;
    case ESOTS_KEM_BackendType::AIS:
        if (!EvaluateAISDefinition(Def, Context, OutRejectReason, OutFailReason))
        {
            LogDebugForDefinition(Def, Context, false, OutFailReason);
            return false;
        }
        break;
    case ESOTS_KEM_BackendType::SpawnActor:
        if (!EvaluateSpawnDefinition(Def, Context, OutRejectReason, OutFailReason))
        {
            LogDebugForDefinition(Def, Context, false, OutFailReason);
            return false;
        }
        break;
    default:
        OutRejectReason = ESOTS_KEMRejectReason::Other;
        OutFailReason = TEXT("Unknown backend type");
        LogDebugForDefinition(Def, Context, false, OutFailReason);
        return false;
    }

    FSOTS_AbilityRequirementCheckResult AbilityReqResult;
    if (!EvaluateAbilityRequirementsForExecution(Context.Instigator.Get(), Def, AbilityReqResult))
    {
        OutRejectReason = ESOTS_KEMRejectReason::AbilityRequirementFailed;
        OutFailReason = FString::Printf(TEXT("Rejected by ability requirements: %s"),
            *USOTS_GAS_AbilityRequirementLibrary::DescribeAbilityRequirementCheckResult(AbilityReqResult).ToString());

        LogDebugForDefinition(Def, Context, false, OutFailReason);
        return false;
    }

    float Score = Def->BaseScore;

    if (Context.GlobalStealthScore01 > Def->MaxGlobalStealthScore01 + KINDA_SMALL_NUMBER)
    {
        OutRejectReason = ESOTS_KEMRejectReason::StealthBlocked;
        OutFailReason = FString::Printf(TEXT("StealthScore %.2f > MaxGlobalStealthScore01 %.2f"),
            Context.GlobalStealthScore01,
            Def->MaxGlobalStealthScore01);

        LogDebugForDefinition(Def, Context, false, OutFailReason);
        return false;
    }

    if (Def->MinShadowLevel01 > 0.0f && Context.ShadowLevel01 >= Def->MinShadowLevel01)
    {
        const float ShadowBonus = (Context.ShadowLevel01 - Def->MinShadowLevel01) * 0.5f;
        Score += FMath::Max(0.0f, ShadowBonus);
    }

    const int32 RequiredCount = Def->RequiredContextTags.Num();
    const int32 ContextCount  = Context.ContextTags.Num();

    if (RequiredCount > 0)
    {
        if (Context.ContextTags == Def->RequiredContextTags)
        {
            Score += 1.0f;
        }
        else if (ContextCount > RequiredCount)
        {
            const int32 ExtraCount = ContextCount - RequiredCount;
            const float ExtraPenalty = FMath::Clamp(static_cast<float>(ExtraCount) * 0.1f, 0.0f, 1.0f);
            Score -= ExtraPenalty;
        }
    }

    OutScore = Score;
    OutFailReason = FString::Printf(TEXT("Passed all checks (Score=%.2f)"), Score);

    LogDebugForDefinition(Def, Context, true, OutFailReason);
    return true;
}

bool USOTS_KEMManagerSubsystem::RequestExecution(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    const USOTS_KEM_ExecutionDefinition* ExecutionOverride,
    const FString& SourceLabel)
{
    return RequestExecutionInternal(WorldContextObject ? WorldContextObject : this,
                                    Instigator,
                                    Target,
                                    ContextTags,
                                    ExecutionOverride,
                                    SourceLabel,
                                    true);
}

bool USOTS_KEMManagerSubsystem::RequestExecution_Blessed(UObject* WorldContextObject,
                                                         AActor* InstigatorActor,
                                                         AActor* TargetActor,
                                                         FGameplayTag ExecutionTag,
                                                         FName ContextReason,
                                                         bool bAllowFallback)
{
    if (!WorldContextObject || !InstigatorActor || !TargetActor || !ExecutionTag.IsValid())
    {
        UE_LOG(LogSOTSKEM, VeryVerbose,
            TEXT("RequestExecution_Blessed: invalid input (Ctx=%s Instigator=%s Target=%s Tag=%s)"),
            *GetNameSafe(WorldContextObject),
            *GetNameSafe(InstigatorActor),
            *GetNameSafe(TargetActor),
            *ExecutionTag.ToString());
        return false;
    }

    FGameplayTagContainer ContextTagsToUse = PlayerContextTags;
    if (ContextReason == FName(TEXT("Dragon")))
    {
        ContextTagsToUse = DragonContextTags;
    }
    else if (ContextReason == FName(TEXT("Cinematic")) ||
             ContextReason == FName(TEXT("Cutscene"))  ||
             ContextReason == FName(TEXT("Sequencer")))
    {
        ContextTagsToUse = CinematicContextTags;
    }
    else if (ContextReason == FName(TEXT("Player")))
    {
        ContextTagsToUse = PlayerContextTags;
    }

    USOTS_KEM_ExecutionDefinition* OverrideDef = FindExecutionDefinitionById(ExecutionTag.GetTagName());
    const FString SourceLabel = ContextReason != NAME_None
        ? ContextReason.ToString()
        : ExecutionTag.ToString();

    return RequestExecutionInternal(WorldContextObject,
                                    InstigatorActor,
                                    TargetActor,
                                    ContextTagsToUse,
                                    OverrideDef,
                                    SourceLabel,
                                    bAllowFallback);
}

bool USOTS_KEMManagerSubsystem::RequestExecution_FromPlayer(AActor* Instigator, AActor* Target)
{
    const UObject* ContextObject = Instigator
        ? static_cast<const UObject*>(Instigator)
        : static_cast<const UObject*>(this);
    return RequestExecution(ContextObject,
                            Instigator,
                            Target,
                            PlayerContextTags,
                            nullptr,
                            TEXT("FromPlayer"));
}

bool USOTS_KEMManagerSubsystem::RequestExecution_FromDragon(AActor* Instigator, AActor* Target)
{
    const UObject* ContextObject = Instigator
        ? static_cast<const UObject*>(Instigator)
        : static_cast<const UObject*>(this);
    return RequestExecution(ContextObject,
                            Instigator,
                            Target,
                            DragonContextTags,
                            nullptr,
                            TEXT("FromDragon"));
}

bool USOTS_KEMManagerSubsystem::RequestExecution_FromCinematic(AActor* Instigator,
                                                              AActor* Target,
                                                              UObject* ExecutionDefinitionOverride)
{
    const UObject* ContextObject = Instigator
        ? static_cast<const UObject*>(Instigator)
        : static_cast<const UObject*>(this);
    const USOTS_KEM_ExecutionDefinition* OverrideDef = Cast<USOTS_KEM_ExecutionDefinition>(ExecutionDefinitionOverride);
    const FString InputSourceLabel = OverrideDef && OverrideDef->ExecutionTag.IsValid()
        ? OverrideDef->ExecutionTag.ToString()
        : TEXT("FromCinematic");

    return RequestExecution(ContextObject,
                            Instigator,
                            Target,
                            CinematicContextTags,
                            OverrideDef,
                            InputSourceLabel);
}

bool USOTS_KEMManagerSubsystem::RequestExecutionInternal(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    const USOTS_KEM_ExecutionDefinition* ExecutionOverride,
    const FString& SourceLabel,
    bool bAllowFallback)
{
    ResetPendingTelemetry();

    if (!Instigator || !Target)
    {
        UE_LOG(LogSOTSKEM, Warning, TEXT("RequestExecution: Instigator or Target is null."));
        return false;
    }

    if (CurrentState != ESOTS_KEMState::Ready)
    {
        UE_LOG(LogSOTSKEM, Verbose, TEXT("RequestExecution: KEM not in Ready state."));
        return false;
    }

    const UObject* ContextObject = WorldContextObject ? WorldContextObject : this;

    // Reset per-request debug state.
    LastCandidateDebug.Empty();
    LastDecisionSnapshot = FSOTS_KEMDecisionSnapshot();
    LastDecisionSnapshot.Instigator = Instigator;
    LastDecisionSnapshot.Target = Target;
    LastDecisionSnapshot.ContextTags = ContextTags;
    LastDecisionSnapshot.SourceLabel = SourceLabel.IsEmpty() ? TEXT("RequestExecution") : SourceLabel;
    LastDecisionSnapshot.RequestedTag = FGameplayTag();

    FSOTS_KEMDebugRecord NewRecord;
    NewRecord.Instigator = Instigator;
    NewRecord.Target     = Target;
    NewRecord.Phase      = TEXT("Requested");
    NewRecord.Result     = ESOTS_KEM_ExecutionResult::Started;
    ActiveDebugRecordIndex = 0;

    RecentDebugRecords.Insert(NewRecord, 0);
    if (RecentDebugRecords.Num() > KEM_MaxDebugRecords)
    {
        RecentDebugRecords.SetNum(KEM_MaxDebugRecords);
    }

    FSOTS_ExecutionContext ExecContext;
    BuildExecutionContext(Instigator, Target, ContextTags, ExecContext);

    const float DistanceToTarget = FVector::Dist(ExecContext.InstigatorLocation, ExecContext.TargetLocation);
    const FVector ForwardDir = ExecContext.InstigatorForward.GetSafeNormal();
    const FVector ToTarget = ExecContext.TargetLocation - ExecContext.InstigatorLocation;
    const float FacingAngleDeg = (!ToTarget.IsNearlyZero() && !ForwardDir.IsNearlyZero())
        ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ForwardDir, ToTarget.GetSafeNormal()), -1.f, 1.f)))
        : 0.f;
    const float HeightDelta = ExecContext.HeightDelta;
    const bool bWithinFallbackDistance = DistanceToTarget <= FallbackTriggerDistance;

    TArray<const USOTS_KEM_ExecutionDefinition*> CandidateDefinitions;
    CandidateDefinitions.Reserve(ExecutionDefinitions.Num() + (ExecutionOverride ? 1 : 0));

    if (ExecutionOverride)
    {
        CandidateDefinitions.Add(ExecutionOverride);
    }

    for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : ExecutionDefinitions)
    {
        const USOTS_KEM_ExecutionDefinition* Def = SoftDef.Get();
        if (!Def)
        {
            Def = SoftDef.LoadSynchronous();
        }

        if (!Def)
        {
            UE_LOG(LogSOTSKEM, Warning, TEXT("RequestExecution: Failed to load an ExecutionDefinition."));
            continue;
        }

        if (ExecutionOverride && ExecutionOverride == Def)
        {
            continue;
        }

        CandidateDefinitions.Add(Def);
    }

    TArray<FSOTS_KEMCandidateDebugRecord> CandidateRecords;
    CandidateRecords.Reserve(CandidateDefinitions.Num());

    const UEnum* RejectReasonEnum = StaticEnum<ESOTS_KEMRejectReason>();
    const UEnum* ExecutionFamilyEnum = StaticEnum<ESOTS_KEM_ExecutionFamily>();

    const USOTS_KEM_ExecutionDefinition* BestDef = nullptr;
    FSOTS_CASQueryResult BestCASResult;
    float BestScore = 0.f;
    bool bHasCandidate = false;
    int32 BestCandidateIndex = INDEX_NONE;
    ASOTS_KEMExecutionAnchor* BestAnchor = nullptr;

    TArray<ASOTS_KEMExecutionAnchor*> NearbyAnchors;
    const float SearchRadius = AnchorSearchRadius > 0.f ? AnchorSearchRadius : KEM_DefaultAnchorSearchRadius;
    FindNearbyExecutionAnchors(Instigator, Target, SearchRadius, NearbyAnchors);

    for (const USOTS_KEM_ExecutionDefinition* Def : CandidateDefinitions)
    {
        if (!Def)
        {
            continue;
        }

        const FString ExecName = Def->ExecutionTag.IsValid()
            ? Def->ExecutionTag.ToString()
            : Def->GetName();

        float Score = 0.f;
        FSOTS_CASQueryResult CASResult;
        ESOTS_KEMRejectReason RejectReason = ESOTS_KEMRejectReason::None;
        FString FailReason;
        const bool bPassed = EvaluateDefinition(Def, ExecContext, Score, CASResult, RejectReason, FailReason);

        ASOTS_KEMExecutionAnchor* CandidateAnchor = nullptr;
        const float AnchorBonus = ComputeAnchorScoreBonus(Def, NearbyAnchors, CandidateAnchor);
        Score += AnchorBonus;

        FSOTS_KEMCandidateDebugRecord Candidate;
        Candidate.ExecutionName = ExecName;
        Candidate.Score         = Score;
        Candidate.bSelected     = false;
        Candidate.RejectReason  = RejectReason;
        Candidate.FailureReason = FailReason;
        Candidate.DistanceToTarget = DistanceToTarget;
        Candidate.FacingAngleDeg   = FacingAngleDeg;
        Candidate.HeightDelta      = HeightDelta;

        CandidateRecords.Add(Candidate);

        if (IsDebugAtLeast(EKEMDebugVerbosity::Verbose) && AnchorBonus > 0.f && CandidateAnchor)
        {
            const FString FamilyName = ExecutionFamilyEnum
                ? ExecutionFamilyEnum->GetNameStringByValue(static_cast<int64>(CandidateAnchor->GetExecutionFamily()))
                : TEXT("Unknown");
            UE_LOG(LogSOTSKEM, Verbose,
                TEXT("[KEM Anchor] %s matched anchor '%s' (Family=%s) bonus %.2f"),
                *Candidate.ExecutionName,
                *GetNameSafe(CandidateAnchor),
                *FamilyName,
                AnchorBonus);
        }

        const int32 CandidateIndex = CandidateRecords.Num() - 1;

        if (!bPassed)
        {
            continue;
        }

        if (!bHasCandidate || Score > BestScore)
        {
            bHasCandidate = true;
            BestScore = Score;
            BestDef = Def;
            BestCASResult = CASResult;
            BestCandidateIndex = CandidateIndex;
            BestAnchor = CandidateAnchor;
        }
    }

    if (BestCandidateIndex != INDEX_NONE && CandidateRecords.IsValidIndex(BestCandidateIndex))
    {
        CandidateRecords[BestCandidateIndex].bSelected = true;
    }

    LastCandidateDebug = CandidateRecords;
    LastDecisionSnapshot.CandidateRecords = CandidateRecords;

    if (IsDebugAtLeast(EKEMDebugVerbosity::Verbose))
    {
        const FString SourceLabelLogged = LastDecisionSnapshot.SourceLabel.IsEmpty()
            ? TEXT("Unknown")
            : LastDecisionSnapshot.SourceLabel;
        const FString InstigatorName = GetNameSafe(Instigator);
        const FString TargetName = GetNameSafe(Target);
        const FString WinnerName = BestDef
            ? (BestDef->ExecutionTag.IsValid() ? BestDef->ExecutionTag.ToString() : BestDef->GetName())
            : TEXT("None");
        const float WinnerScore = bHasCandidate ? BestScore : 0.f;

        for (const FSOTS_KEMCandidateDebugRecord& CandidateRecord : CandidateRecords)
        {
            const FString RejectName = RejectReasonEnum
                ? RejectReasonEnum->GetNameStringByValue(static_cast<int64>(CandidateRecord.RejectReason))
                : TEXT("Unknown");
            const FString FailureReason = CandidateRecord.FailureReason.IsEmpty()
                ? TEXT("None")
                : CandidateRecord.FailureReason;

            UE_LOG(LogSOTSKEM, Verbose,
                TEXT("KEM: Candidate '%s' Score=%.2f Selected=%s Reject=%s Reason=%s Dist=%.1f Angle=%.1f Height=%.1f"),
                *CandidateRecord.ExecutionName,
                CandidateRecord.Score,
                CandidateRecord.bSelected ? TEXT("true") : TEXT("false"),
                *RejectName,
                *FailureReason,
                CandidateRecord.DistanceToTarget,
                CandidateRecord.FacingAngleDeg,
                CandidateRecord.HeightDelta);
        }

        UE_LOG(LogSOTSKEM, Verbose,
            TEXT("KEM: Request Source=%s Instigator=%s Target=%s Candidates=%d Winner=%s Score=%.2f"),
            *SourceLabelLogged,
            *InstigatorName,
            *TargetName,
            CandidateRecords.Num(),
            *WinnerName,
            WinnerScore);
    }

    if (!bHasCandidate || !BestDef)
    {
        if (IsDebugAtLeast(EKEMDebugVerbosity::Verbose))
        {
            UE_LOG(LogSOTSKEM, Verbose,
                TEXT("KEM: No valid execution for Instigator=%s Target=%s Candidates=%d"),
                *GetNameSafe(Instigator),
                *GetNameSafe(Target),
                CandidateRecords.Num());
        }

        if (bAllowFallback && bWithinFallbackDistance && Instigator && TryPlayFallbackMontage(Instigator))
        {
            UE_LOG(LogSOTSKEM, Log,
                TEXT("KEM: Fallback montage triggered for Instigator=%s"),
                *Instigator->GetName());
        }

        if (IsDebugAtLeast(EKEMDebugVerbosity::Verbose) && CandidateRecords.Num() > 0)
        {
            UE_LOG(LogSOTSKEM, Verbose,
                TEXT("[KEM] Candidate breakdown recorded (%d entries) for the failed request."),
                CandidateRecords.Num());
        }

        if (RecentDebugRecords.IsValidIndex(ActiveDebugRecordIndex))
        {
            FSOTS_KEMDebugRecord& Record = RecentDebugRecords[ActiveDebugRecordIndex];
            Record.ExecutionName = TEXT("None");
            Record.Score         = 0.f;
            Record.Phase         = TEXT("Failed");
            Record.Result        = ESOTS_KEM_ExecutionResult::Failed;
            Record.FailureReason = TEXT("No valid execution found for this context");
        }
        ActiveDebugRecordIndex = INDEX_NONE;

        FSOTS_KEMExecutionTelemetry FailureTelemetry = BuildTelemetryRecord(
            nullptr,
            ExecContext,
            LastDecisionSnapshot.SourceLabel,
            DistanceToTarget,
            FacingAngleDeg,
            HeightDelta,
            0.f,
            nullptr,
            false);
        FailureTelemetry.Outcome = ESOTS_KEMExecutionOutcome::Failed_InternalError;
        FSOTS_KEMDecisionStep FailureStep;
        FailureStep.StepName = FName(TEXT("SelectionFailure"));
        FailureStep.Detail = TEXT("No valid execution found for this context");
        FailureStep.bPassed = false;
        FailureTelemetry.DecisionSteps.Add(FailureStep);
        BroadcastExecutionTelemetry(FailureTelemetry);
        ResetPendingTelemetry();

        return false;
    }

    LastDecisionSnapshot.RequestedTag = BestDef->ExecutionTag;

    // Update debug record with the chosen execution.
    if (RecentDebugRecords.IsValidIndex(ActiveDebugRecordIndex))
    {
        FSOTS_KEMDebugRecord& Record = RecentDebugRecords[ActiveDebugRecordIndex];
        Record.RequestedTag = BestDef->ExecutionTag;
        Record.ExecutionName = BestDef->ExecutionTag.IsValid()
            ? BestDef->ExecutionTag.ToString()
            : BestDef->GetName();
        Record.Score  = BestScore;
        Record.Phase  = TEXT("Executing");
        Record.Result = ESOTS_KEM_ExecutionResult::Started;
    }

    const bool bUsedOmniTraceForExec = (BestDef->BackendType == ESOTS_KEM_BackendType::SpawnActor) &&
        BestDef->SpawnActorConfig.bUseOmniTraceForWarp;
    PrepareExecutionTelemetryForSelection(
        BestDef,
        ExecContext,
        LastDecisionSnapshot.SourceLabel,
        DistanceToTarget,
        FacingAngleDeg,
        HeightDelta,
        BestScore,
        BestAnchor,
        bUsedOmniTraceForExec);

    // Lifecycle + FX: execution has been chosen.
    BroadcastExecutionEvent(ESOTS_KEM_ExecutionResult::Started, ExecContext, BestDef);
    TriggerExecutionFX(ESOTS_KEM_ExecutionResult::Started, ExecContext, BestDef);

    switch (BestDef->BackendType)
    {
    case ESOTS_KEM_BackendType::CAS:
        {
            UE_LOG(LogSOTSKEM, Log, TEXT("RequestExecution: Selected CAS execution '%s' (Score=%.2f)."),
                *BestDef->ExecutionTag.ToString(), BestScore);

            OnCASExecutionChosen.Broadcast(
                const_cast<USOTS_KEM_ExecutionDefinition*>(BestDef),
                BestCASResult.Scene,
                Instigator,
                Target,
                BestCASResult.SectionName,
                BestCASResult.InstigatorRole,
                BestCASResult.TargetRole,
                BestCASResult.InstigatorWarpTarget);
            break;
        }
    case ESOTS_KEM_BackendType::SpawnActor:
        {
            UE_LOG(LogSOTSKEM, Log, TEXT("RequestExecution: Selected SpawnActor execution '%s' (Score=%.2f)."),
                *BestDef->ExecutionTag.ToString(), BestScore);

            if (!ExecuteSpawnActorBackend(const_cast<USOTS_KEM_ExecutionDefinition*>(BestDef), ExecContext, BestAnchor))
            {
                UE_LOG(LogSOTSKEM, Warning, TEXT("RequestExecution: ExecuteSpawnActorBackend failed."));
                return false;
            }
            break;
        }
    case ESOTS_KEM_BackendType::LevelSequence:
        {
            UE_LOG(LogSOTSKEM, Log, TEXT("RequestExecution: Selected LevelSequence execution '%s' (Score=%.2f)."),
                *BestDef->ExecutionTag.ToString(), BestScore);

            OnLevelSequenceExecutionChosen.Broadcast(const_cast<USOTS_KEM_ExecutionDefinition*>(BestDef), Instigator, Target, ExecContext);
            break;
        }
    case ESOTS_KEM_BackendType::AIS:
        {
            UE_LOG(LogSOTSKEM, Log, TEXT("RequestExecution: Selected AIS execution '%s' (Score=%.2f)."),
                *BestDef->ExecutionTag.ToString(), BestScore);

            OnAISExecutionChosen.Broadcast(const_cast<USOTS_KEM_ExecutionDefinition*>(BestDef), Instigator, Target, ExecContext);
            break;
        }
    default:
        {
            UE_LOG(LogSOTSKEM, Log, TEXT("RequestExecution: Selected backend %d is not implemented."),
                static_cast<int32>(BestDef->BackendType));
            return false;
        }
    }

    CurrentState = ESOTS_KEMState::Executing;
    return true;
}

void USOTS_KEMManagerSubsystem::RunKEMDebug(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags) const
{
    FSOTS_ExecutionContext ExecContext;
    const_cast<USOTS_KEMManagerSubsystem*>(this)->BuildExecutionContext(Instigator, Target, ContextTags, ExecContext);

    UE_LOG(LogSOTSKEM, Log, TEXT("=== KEM Debug Start ==="));
    UE_LOG(LogSOTSKEM, Log, TEXT("Instigator=%s Target=%s HeightDelta=%.1f"),
        Instigator ? *Instigator->GetName() : TEXT("None"),
        Target ? *Target->GetName() : TEXT("None"),
        ExecContext.HeightDelta);

    for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : ExecutionDefinitions)
    {
        USOTS_KEM_ExecutionDefinition* Def = SoftDef.Get();
        if (!Def)
        {
            Def = SoftDef.LoadSynchronous();
        }
        if (!Def)
        {
            UE_LOG(LogSOTSKEM, Warning, TEXT("KEM Debug: Failed to load ExecutionDefinition."));
            continue;
        }

        float Score = 0.f;
        FSOTS_CASQueryResult CASResult;
        ESOTS_KEMRejectReason RejectReason = ESOTS_KEMRejectReason::None;
        FString FailReason;
        const_cast<USOTS_KEMManagerSubsystem*>(this)->EvaluateDefinition(Def, ExecContext, Score, CASResult, RejectReason, FailReason);
    }

    UE_LOG(LogSOTSKEM, Log, TEXT("=== KEM Debug End ==="));
}

void USOTS_KEMManagerSubsystem::FindNearbyExecutionAnchors(
    AActor* Instigator,
    AActor* Target,
    float SearchRadius,
    TArray<ASOTS_KEMExecutionAnchor*>& OutAnchors) const
{
    OutAnchors.Reset();

    if (!Instigator && !Target)
    {
        return;
    }

    if (SearchRadius <= 0.f)
    {
        return;
    }

    UWorld* World = Instigator ? Instigator->GetWorld() : (Target ? Target->GetWorld() : nullptr);
    if (!World)
    {
        return;
    }

    for (TActorIterator<ASOTS_KEMExecutionAnchor> It(World); It; ++It)
    {
        ASOTS_KEMExecutionAnchor* Anchor = *It;
        if (!Anchor)
        {
            continue;
        }

        const FVector AnchorLocation = Anchor->GetActorLocation();
        const float EffectiveRadius = FMath::Max(SearchRadius, Anchor->GetUseRadius());
        const float EffectiveRadiusSq = FMath::Square(EffectiveRadius);

        float ClosestDistSq = FLT_MAX;
        if (Target)
        {
            ClosestDistSq = FMath::Min(ClosestDistSq, FVector::DistSquared(AnchorLocation, Target->GetActorLocation()));
        }
        if (Instigator)
        {
            ClosestDistSq = FMath::Min(ClosestDistSq, FVector::DistSquared(AnchorLocation, Instigator->GetActorLocation()));
        }

        if (ClosestDistSq <= EffectiveRadiusSq)
        {
            OutAnchors.Add(Anchor);
        }
    }
}

float USOTS_KEMManagerSubsystem::ComputeAnchorScoreBonus(
    const USOTS_KEM_ExecutionDefinition* Def,
    const TArray<ASOTS_KEMExecutionAnchor*>& Anchors,
    ASOTS_KEMExecutionAnchor*& OutMatchingAnchor) const
{
    OutMatchingAnchor = nullptr;
    if (!Def || Anchors.IsEmpty())
    {
        return 0.f;
    }

    float BestBonus = 0.f;
    for (ASOTS_KEMExecutionAnchor* Anchor : Anchors)
    {
        if (!Anchor)
        {
            continue;
        }

        float Bonus = 0.f;
        if (Def->PositionTag.IsValid() && Anchor->GetPositionTag().IsValid() &&
            Anchor->GetPositionTag() == Def->PositionTag)
        {
            Bonus = 1.f;
        }
        else if (Def->ExecutionFamily != ESOTS_KEM_ExecutionFamily::Unknown &&
                 Anchor->GetExecutionFamily() == Def->ExecutionFamily)
        {
            Bonus = 0.75f;
        }

        if (Bonus > BestBonus)
        {
            BestBonus = Bonus;
            OutMatchingAnchor = Anchor;
            if (FMath::IsNearlyEqual(Bonus, 1.f))
            {
                break;
            }
        }
    }

    return BestBonus;
}

void USOTS_KEMManagerSubsystem::KEM_ToggleAnchorDebug()
{
    bAnchorDebugDraw = !bAnchorDebugDraw;
    UE_LOG(LogSOTSKEM, Log, TEXT("KEM_ToggleAnchorDebug: Anchor debug %s."),
        bAnchorDebugDraw ? TEXT("Enabled") : TEXT("Disabled"));
}

void USOTS_KEMManagerSubsystem::GetNearbyAnchorsForDebug(
    AActor* CenterActor,
    float Radius,
    TArray<FSOTS_KEMAnchorDebugInfo>& OutAnchors) const
{
    OutAnchors.Reset();
    if (!CenterActor)
    {
        return;
    }

    const float SearchRadius = Radius > 0.f ? Radius : AnchorSearchRadius;
    TArray<ASOTS_KEMExecutionAnchor*> Found;
    FindNearbyExecutionAnchors(CenterActor, CenterActor, SearchRadius, Found);

    for (ASOTS_KEMExecutionAnchor* Anchor : Found)
    {
        if (!Anchor)
        {
            continue;
        }

        FSOTS_KEMAnchorDebugInfo Info;
        Info.AnchorName = Anchor->GetName();
        Info.PositionTag = Anchor->GetPositionTag();
        Info.ExecutionFamily = Anchor->GetExecutionFamily();
        Info.UseRadius = Anchor->GetUseRadius();
        Info.Location = Anchor->GetActorLocation();
        Info.Forward = Anchor->GetActorForwardVector();
        OutAnchors.Add(Info);
    }
}

void USOTS_KEMManagerSubsystem::DrawAnchorDebugVisualization(AActor* CenterActor, float Radius) const
{
    if (!bAnchorDebugDraw || !CenterActor)
    {
        return;
    }

#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
    return;
#endif

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float SearchRadius = Radius > 0.f ? Radius : AnchorSearchRadius;
    TArray<ASOTS_KEMExecutionAnchor*> Found;
    FindNearbyExecutionAnchors(CenterActor, CenterActor, SearchRadius, Found);

    const float ArrowLength = 75.f;
    const float ArrowThickness = 2.f;

    for (ASOTS_KEMExecutionAnchor* Anchor : Found)
    {
        if (!Anchor)
        {
            continue;
        }

        const FVector Location = Anchor->GetActorLocation();
        const FVector Forward = Anchor->GetActorForwardVector().GetSafeNormal();
        const float RadiusToDraw = FMath::Max(Anchor->GetUseRadius(), 30.f);
        const FLinearColor Color = GetColorForFamily(Anchor->GetExecutionFamily());

        DrawDebugSphere(World, Location, RadiusToDraw, 12, Color.ToFColor(true), false, 0.1f, 0, 2.f);
        DrawDebugDirectionalArrow(World, Location, Location + Forward * ArrowLength, 30.f, Color.ToFColor(true), false, 0.1f, 0, ArrowThickness);
        DrawDebugString(World, Location + FVector(0.f, 0.f, 30.f), Anchor->GetName(), nullptr, Color.ToFColor(true), 0.1f, false);
    }
}

// DevTools: KEM Python coverage tools may parse the log output of KEM_SelfTest and KEM_DumpCoverage for CI-style checks.
void USOTS_KEMManagerSubsystem::KEM_SelfTest()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_SelfTest: No World."));
        return;
    }

    USOTS_KEM_ExecutionCatalog* Catalog = USOTS_KEMCatalogLibrary::GetExecutionCatalog(World);
    if (!Catalog)
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_SelfTest: No ExecutionCatalog found."));
        return;
    }

    if (Catalog->ExecutionDefinitions.IsEmpty())
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_SelfTest: ExecutionCatalog contains no definitions."));
    }

    TMap<FString, FSOTS_KEMValidationResult> Results;
    for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Catalog->ExecutionDefinitions)
    {
        USOTS_KEM_ExecutionDefinition* Def = SoftDef.LoadSynchronous();
        const FString EntryName = GetDefinitionDisplayName(SoftDef, Def);

        if (!Def)
        {
            FSOTS_KEMValidationResult InvalidResult;
            InvalidResult.AddError(TEXT("Failed to load definition."));
            Results.Add(EntryName, InvalidResult);
            continue;
        }

        Results.Add(EntryName, ValidateExecutionDefinition(Def));
    }

    const int32 Total = Results.Num();
    int32 ValidCount = 0;
    for (const auto& Pair : Results)
    {
        if (Pair.Value.bIsValid)
        {
            ++ValidCount;
        }
    }

    UE_LOG(LogSOTS_KEM, Display,
        TEXT("KEM_SelfTest: %d definitions, %d valid, %d invalid."),
        Total, ValidCount, Total - ValidCount);

    for (const auto& Pair : Results)
    {
        const FSOTS_KEMValidationResult& Result = Pair.Value;
        if (Result.bIsValid && Result.Warnings.Num() == 0)
        {
            continue;
        }

        UE_LOG(LogSOTS_KEM, Display, TEXT("  %s:"), *Pair.Key);
        for (const FString& Error : Result.Errors)
        {
            UE_LOG(LogSOTS_KEM, Display, TEXT("    Error: %s"), *Error);
        }
        for (const FString& Warning : Result.Warnings)
        {
            UE_LOG(LogSOTS_KEM, Display, TEXT("    Warn:  %s"), *Warning);
        }
    }
}

void USOTS_KEMManagerSubsystem::KEM_DumpCoverage()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_DumpCoverage: No World."));
        return;
    }

    USOTS_KEM_ExecutionCatalog* Catalog = USOTS_KEMCatalogLibrary::GetExecutionCatalog(World);
    if (!Catalog)
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_DumpCoverage: No ExecutionCatalog found."));
        return;
    }

    if (Catalog->ExecutionDefinitions.IsEmpty())
    {
        UE_LOG(LogSOTS_KEM, Warning, TEXT("KEM_DumpCoverage: ExecutionCatalog contains no definitions."));
    }

    TMap<FString, int32> FamilyCounts;
    TMap<FString, int32> PositionCounts;

    for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Catalog->ExecutionDefinitions)
    {
        USOTS_KEM_ExecutionDefinition* Def = SoftDef.LoadSynchronous();
        if (!Def)
        {
            continue;
        }

        const FString FamilyName = GetExecutionFamilyName(Def->ExecutionFamily);
        FamilyCounts.FindOrAdd(FamilyName) += 1;

        TSet<FString> PositionNames;
        if (const FGameplayTag Effective = DetermineEffectivePositionTag(Def); Effective.IsValid())
        {
            PositionNames.Add(Effective.ToString());
        }
        if (Def->PositionTag.IsValid())
        {
            PositionNames.Add(Def->PositionTag.ToString());
        }
        for (const FGameplayTag& Additional : Def->AdditionalPositionTags)
        {
            if (Additional.IsValid())
            {
                PositionNames.Add(Additional.ToString());
            }
        }
        if (PositionNames.IsEmpty())
        {
            PositionNames.Add(TEXT("Unspecified"));
        }
        for (const FString& PositionName : PositionNames)
        {
            PositionCounts.FindOrAdd(PositionName) += 1;
        }
    }

    UE_LOG(LogSOTS_KEM, Display, TEXT("[KEM Coverage]"));
    UE_LOG(LogSOTS_KEM, Display, TEXT("Families:"));
    TArray<FString> FamilyKeys;
    FamilyCounts.GetKeys(FamilyKeys);
    FamilyKeys.Sort();
    for (const FString& Family : FamilyKeys)
    {
        UE_LOG(LogSOTS_KEM, Display, TEXT("  %s: %d"), *Family, FamilyCounts[Family]);
    }

    UE_LOG(LogSOTS_KEM, Display, TEXT("Positions:"));
    TArray<FString> PositionKeys;
    PositionCounts.GetKeys(PositionKeys);
    PositionKeys.Sort();
    for (const FString& Position : PositionKeys)
    {
        UE_LOG(LogSOTS_KEM, Display, TEXT("  %s: %d"), *Position, PositionCounts[Position]);
    }
}

void USOTS_KEMManagerSubsystem::LogDebugForDefinition(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    bool bPassed,
    const FString& Reason) const
{
    if (!Def)
    {
        return;
    }

    const FString NameStr = Def->ExecutionTag.IsValid() ? Def->ExecutionTag.ToString() : Def->GetName();
    const TCHAR* StatusStr = bPassed ? TEXT("PASS") : TEXT("FAIL");

    UE_LOG(LogSOTSKEM, Log, TEXT("[KEM Debug] %s: %s - %s"),
        *NameStr, StatusStr, *Reason);
}

bool USOTS_KEMManagerSubsystem::IsTelemetryLoggingEnabled() const
{
    if (bEnableTelemetryLogging)
    {
        return true;
    }

    return CVarSOTSKEMTelemetryLogging.GetValueOnGameThread() != 0;
}

void USOTS_KEMManagerSubsystem::ResetPendingTelemetry()
{
    PendingExecutionTelemetry = FSOTS_KEMExecutionTelemetry();
    bHasPendingExecutionTelemetry = false;
}

void USOTS_KEMManagerSubsystem::PrepareExecutionTelemetryForSelection(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    const FString& SourceLabel,
    float DistanceToTarget,
    float FacingAngleDeg,
    float HeightDelta,
    float Score,
    ASOTS_KEMExecutionAnchor* Anchor,
    bool bUsedOmniTrace)
{
    PendingExecutionTelemetry = BuildTelemetryRecord(Def,
                                                    Context,
                                                    SourceLabel,
                                                    DistanceToTarget,
                                                    FacingAngleDeg,
                                                    HeightDelta,
                                                    Score,
                                                    Anchor,
                                                    bUsedOmniTrace);

    PendingExecutionTelemetry.Outcome = ESOTS_KEMExecutionOutcome::Failed_InternalError;
    bHasPendingExecutionTelemetry = true;
}

FSOTS_KEMExecutionTelemetry USOTS_KEMManagerSubsystem::BuildTelemetryRecord(
    const USOTS_KEM_ExecutionDefinition* Def,
    const FSOTS_ExecutionContext& Context,
    const FString& SourceLabel,
    float DistanceToTarget,
    float FacingAngleDeg,
    float HeightDelta,
    float Score,
    ASOTS_KEMExecutionAnchor* Anchor,
    bool bUsedOmniTrace) const
{
    FSOTS_KEMExecutionTelemetry Telemetry;
    Telemetry.ExecutionTag = Def ? Def->ExecutionTag : FGameplayTag();
    Telemetry.ExecutionFamilyTag = Def ? Def->ExecutionFamilyTag : FGameplayTag();
    Telemetry.ExecutionPositionTag = DetermineEffectivePositionTag(Def);
    Telemetry.Instigator = Context.Instigator;
    Telemetry.Target = Context.Target;
    Telemetry.DistanceToTarget = DistanceToTarget;
    Telemetry.StealthTierAtRequest = static_cast<int32>(Context.StealthTier);
    Telemetry.bDragonControlled = SourceLabel.Contains(TEXT("Dragon"), ESearchCase::IgnoreCase);
    Telemetry.bFromCutscene = SourceLabel.Contains(TEXT("Cinematic"), ESearchCase::IgnoreCase) ||
        SourceLabel.Contains(TEXT("Cutscene"), ESearchCase::IgnoreCase);
    Telemetry.bUsedAnchor = Anchor != nullptr;
    Telemetry.bUsedOmniTrace = bUsedOmniTrace;
    Telemetry.SourceLabel = SourceLabel.IsEmpty() ? TEXT("Unknown") : SourceLabel;

    auto AddStep = [&](const FName& StepName, const FString& Detail, bool bPassed, float Value)
    {
        FSOTS_KEMDecisionStep Step;
        Step.StepName = StepName;
        Step.Detail = Detail;
        Step.bPassed = bPassed;
        Step.NumericValue = Value;
        Telemetry.DecisionSteps.Add(Step);
    };

    AddStep(FName(TEXT("SelectionScore")), FString::Printf(TEXT("Score=%.2f"), Score), true, Score);
    AddStep(FName(TEXT("Distance")), FString::Printf(TEXT("Distance=%.1f"), DistanceToTarget), true, DistanceToTarget);
    AddStep(FName(TEXT("FacingAngle")), FString::Printf(TEXT("Angle=%.1f"), FacingAngleDeg), true, FacingAngleDeg);
    AddStep(FName(TEXT("HeightDelta")), FString::Printf(TEXT("Height=%.1f"), HeightDelta), true, HeightDelta);
    AddStep(FName(TEXT("StealthTier")), FString::Printf(TEXT("Tier=%d"), Telemetry.StealthTierAtRequest), true, Telemetry.StealthTierAtRequest);
    AddStep(FName(TEXT("Anchor")), Telemetry.bUsedAnchor ? TEXT("Matched anchor") : TEXT("No anchor"), Telemetry.bUsedAnchor, Telemetry.bUsedAnchor ? 1.f : 0.f);
    AddStep(FName(TEXT("OmniTrace")), Telemetry.bUsedOmniTrace ? TEXT("OmniTrace active") : TEXT("OmniTrace inactive"), Telemetry.bUsedOmniTrace, Telemetry.bUsedOmniTrace ? 1.f : 0.f);
    AddStep(FName(TEXT("Source")), Telemetry.SourceLabel, true, 0.f);

    return Telemetry;
}


void USOTS_KEMManagerSubsystem::RegisterExecutionDefinition(FName ExecutionId, USOTS_KEM_ExecutionDefinition* Definition)
{
    if (ExecutionId.IsNone() || !Definition)
    {
        return;
    }

    // Update the ID map
    ExecutionDefinitionsById.FindOrAdd(ExecutionId) = Definition;

    // Make sure the main list used by RequestExecution also contains it
    if (!ExecutionDefinitions.Contains(Definition))
    {
        ExecutionDefinitions.Add(Definition);
    }
}

void USOTS_KEMManagerSubsystem::RegisterExecutionDefinitionsFromConfig(USOTS_KEM_ExecutionRegistryConfig* Config)
{
    ExecutionDefinitions.Empty();
    ExecutionDefinitionsById.Empty();

    if (!Config)
    {
        UE_LOG(LogSOTSKEM, Warning, TEXT("RegisterExecutionDefinitionsFromConfig: Config is null."));
        return;
    }

    for (const FSOTS_KEM_ExecutionRegistryEntry& Entry : Config->Entries)
    {
        if (!Entry.ExecutionDefinition.IsValid() && !Entry.ExecutionDefinition.ToSoftObjectPath().IsValid())
        {
            continue;
        }

        USOTS_KEM_ExecutionDefinition* Def = Entry.ExecutionDefinition.Get();
        if (!Def)
        {
            Def = Entry.ExecutionDefinition.LoadSynchronous();
        }

        if (!Def)
        {
            UE_LOG(LogSOTSKEM, Warning, TEXT("RegisterExecutionDefinitionsFromConfig: Failed to load ExecutionDefinition for Id '%s'."),
                *Entry.ExecutionId.ToString());
            continue;
        }

        // If no explicit ID is provided, fall back to the execution tag name if possible.
        FName IdToUse = Entry.ExecutionId;
        if (IdToUse.IsNone() && Def->ExecutionTag.IsValid())
        {
            IdToUse = Def->ExecutionTag.GetTagName();
        }

        if (IdToUse.IsNone())
        {
            // As a last resort, use the asset name.
            IdToUse = *Def->GetName();
        }

        RegisterExecutionDefinition(IdToUse, Def);
    }

    UE_LOG(LogSOTSKEM, Log, TEXT("RegisterExecutionDefinitionsFromConfig: Registered %d execution definitions (Entries=%d)."),
        ExecutionDefinitions.Num(), Config->Entries.Num());
}

void USOTS_KEMManagerSubsystem::InitializeFromDefaultRegistryConfig()
{
    if (!DefaultRegistryConfig.ToSoftObjectPath().IsValid())
    {
        UE_LOG(LogSOTSKEM, Verbose, TEXT("InitializeFromDefaultRegistryConfig: DefaultRegistryConfig is not set."));
        return;
    }

    USOTS_KEM_ExecutionRegistryConfig* Config = DefaultRegistryConfig.Get();
    if (!Config)
    {
        Config = DefaultRegistryConfig.LoadSynchronous();
    }

    if (!Config)
    {
        UE_LOG(LogSOTSKEM, Warning, TEXT("InitializeFromDefaultRegistryConfig: Failed to load DefaultRegistryConfig."));
        return;
    }

    RegisterExecutionDefinitionsFromConfig(Config);
}

USOTS_KEM_ExecutionDefinition* USOTS_KEMManagerSubsystem::FindExecutionDefinitionById(FName ExecutionId)
{
    if (ExecutionId.IsNone())
    {
        return nullptr;
    }

    if (TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>* FoundPtr = ExecutionDefinitionsById.Find(ExecutionId))
    {
        USOTS_KEM_ExecutionDefinition* Def = FoundPtr->Get();
        if (!Def)
        {
            Def = FoundPtr->LoadSynchronous();
        }
        return Def;
    }

    return nullptr;
}
