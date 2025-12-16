#include "SOTS_FXManagerSubsystem.h"

#include "SOTS_FXCueDefinition.h"
#include "SOTS_FXDefinitionLibrary.h"
#include "SOTS_ProfileSubsystem.h"

#include "Algo/Sort.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystemComponent.h"

TWeakObjectPtr<USOTS_FXManagerSubsystem> USOTS_FXManagerSubsystem::SingletonInstance = nullptr;

void USOTS_FXManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SingletonInstance = this;

    CueMap.Reset();
    CuePools.Reset();
    RegisteredLibraries.Reset();
    SortedLibraries.Reset();
    RegisteredLibraryDefinitions.Reset();
    NextRegistrationOrder = 0;
    bRegistryReady = false;

    SeedLibrariesFromConfig();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->RegisterProvider(this, 0);
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bValidateFXRegistryOnInit)
    {
        ValidateLibraryDefinitions();
    }
#endif
}

void USOTS_FXManagerSubsystem::Deinitialize()
{
    SortedLibraries.Reset();
    RegisteredLibraries.Reset();
    RegisteredLibraryDefinitions.Reset();
    bRegistryReady = false;
    CuePools.Reset();
    CueMap.Reset();
    NextRegistrationOrder = 0;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->UnregisterProvider(this);
        }
    }

    SingletonInstance = nullptr;

    Super::Deinitialize();
}

USOTS_FXManagerSubsystem* USOTS_FXManagerSubsystem::Get()
{
    return SingletonInstance.Get();
}

bool USOTS_FXManagerSubsystem::RegisterLibrary(USOTS_FXDefinitionLibrary* Library, int32 Priority)
{
    return RegisterLibraryInternal(Library, Priority, true);
}

bool USOTS_FXManagerSubsystem::RegisterLibraries(const TArray<USOTS_FXDefinitionLibrary*>& InLibraries, int32 Priority)
{
    bool bChanged = false;
    for (USOTS_FXDefinitionLibrary* Library : InLibraries)
    {
        bChanged |= RegisterLibraryInternal(Library, Priority, false);
    }

    if (bChanged)
    {
        RebuildLibraryOrderAndRegistry();
    }

    return bChanged;
}

bool USOTS_FXManagerSubsystem::UnregisterLibrary(USOTS_FXDefinitionLibrary* Library)
{
    if (!Library)
    {
        return false;
    }

    const int32 Removed = RegisteredLibraries.RemoveAll([Library](const FSOTS_FXRegisteredLibrary& Entry)
    {
        return Entry.Library == Library;
    });

    if (Removed > 0)
    {
        RebuildLibraryOrderAndRegistry();
        return true;
    }

    return false;
}

bool USOTS_FXManagerSubsystem::RegisterLibrarySoft(TSoftObjectPtr<USOTS_FXDefinitionLibrary> Library, int32 Priority)
{
    FSOTS_FXSoftLibraryRegistration Entry;
    Entry.Library = Library;
    Entry.Priority = Priority;

    return RegisterSoftLibraryInternal(Entry, true);
}

bool USOTS_FXManagerSubsystem::RegisterLibraryInternal(USOTS_FXDefinitionLibrary* Library, int32 Priority, bool bRebuildRegistry)
{
    if (!IsValid(Library))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bLogLibraryRegistration)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Rejecting library registration (null or pending kill)."));
        }
#endif
        return false;
    }

    bool bChanged = false;
    const int32 EffectivePriority = Priority;

    if (FSOTS_FXRegisteredLibrary* Existing = RegisteredLibraries.FindByPredicate([Library](const FSOTS_FXRegisteredLibrary& Entry)
    {
        return Entry.Library == Library;
    }))
    {
        if (Existing->Priority != EffectivePriority)
        {
            Existing->Priority = EffectivePriority;
            bChanged = true;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bLogLibraryRegistration)
            {
                UE_LOG(LogTemp, Log, TEXT("[SOTS_FX] Updated library %s priority to %d (order %d)."), *Library->GetName(), EffectivePriority, Existing->RegistrationOrder);
            }
#endif
        }
    }
    else
    {
        FSOTS_FXRegisteredLibrary& NewEntry = RegisteredLibraries.AddDefaulted_GetRef();
        NewEntry.Library = Library;
        NewEntry.Priority = EffectivePriority;
        NewEntry.RegistrationOrder = NextRegistrationOrder++;
        bChanged = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bLogLibraryRegistration)
        {
            UE_LOG(LogTemp, Log, TEXT("[SOTS_FX] Registered library %s (Priority=%d, Order=%d)."), *Library->GetName(), EffectivePriority, NewEntry.RegistrationOrder);
        }
#endif
    }

    if (bChanged && bRebuildRegistry)
    {
        RebuildLibraryOrderAndRegistry();
    }

    return bChanged;
}

bool USOTS_FXManagerSubsystem::RegisterSoftLibraryInternal(const FSOTS_FXSoftLibraryRegistration& Entry, bool bRebuildRegistry)
{
    if (!Entry.Library.IsValid() && !Entry.Library.ToSoftObjectPath().IsValid())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bLogLibraryRegistration)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Soft library entry is empty; skipping."));
        }
#endif
        return false;
    }

    USOTS_FXDefinitionLibrary* Loaded = Entry.Library.LoadSynchronous();
    if (!Loaded)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bLogLibraryRegistration)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Soft library %s failed to load."), *Entry.Library.ToString());
        }
#endif
        return false;
    }

    return RegisterLibraryInternal(Loaded, Entry.Priority, bRebuildRegistry);
}

void USOTS_FXManagerSubsystem::SeedLibrariesFromConfig()
{
    RegisteredLibraries.Reset();
    SortedLibraries.Reset();
    RegisteredLibraryDefinitions.Reset();
    bRegistryReady = false;

    for (const FSOTS_FXLibraryRegistration& Entry : LibraryRegistrations)
    {
        RegisterLibraryInternal(Entry.Library, Entry.Priority, false);
    }

    for (const TObjectPtr<USOTS_FXDefinitionLibrary>& Library : Libraries)
    {
        RegisterLibraryInternal(Library.Get(), Library ? Library->Priority : 0, false);
    }

    for (const FSOTS_FXSoftLibraryRegistration& Entry : SoftLibraryRegistrations)
    {
        RegisterSoftLibraryInternal(Entry, false);
    }

    RebuildLibraryOrderAndRegistry();
}

void USOTS_FXManagerSubsystem::RebuildSortedLibraries()
{
    SortedLibraries.Reset();

    RegisteredLibraries.RemoveAll([this](const FSOTS_FXRegisteredLibrary& Entry)
    {
        const bool bInvalid = !Entry.IsValid();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bInvalid && bLogLibraryRegistration)
        {
            const FString Name = Entry.Library ? Entry.Library->GetName() : TEXT("<null>");
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Dropping invalid library entry %s."), *Name);
        }
#endif
        return bInvalid;
    });

    SortedLibraries = RegisteredLibraries;

    SortedLibraries.StableSort([](const FSOTS_FXRegisteredLibrary& A, const FSOTS_FXRegisteredLibrary& B)
    {
        if (A.Priority != B.Priority)
        {
            return A.Priority > B.Priority;
        }
        return A.RegistrationOrder < B.RegistrationOrder;
    });

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogCueResolution)
    {
        UE_LOG(LogTemp, Log, TEXT("[SOTS_FX] Library order => %s"), *BuildLibraryOrderDebugString());
    }
#endif
}

void USOTS_FXManagerSubsystem::RebuildLibraryOrderAndRegistry()
{
    bRegistryReady = false;
    RebuildSortedLibraries();
    BuildRegistryFromLibraries();
    bRegistryReady = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    LogLibraryOrderDebug();
#endif
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
FString USOTS_FXManagerSubsystem::BuildLibraryOrderDebugString() const
{
    TArray<FString> Parts;
    Parts.Reserve(SortedLibraries.Num());

    for (const FSOTS_FXRegisteredLibrary& Entry : SortedLibraries)
    {
        const FString Name = Entry.Library ? Entry.Library->GetName() : TEXT("<null>");
        Parts.Add(FString::Printf(TEXT("%s(P=%d,Idx=%d)"), *Name, Entry.Priority, Entry.RegistrationOrder));
    }

    return FString::Join(Parts, TEXT(" -> "));
}
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
void USOTS_FXManagerSubsystem::LogLibraryOrderDebug() const
{
    if (!bDebugLogCueResolution)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SOTS_FX] Library order => %s"), *BuildLibraryOrderDebugString());
}
#endif

void USOTS_FXManagerSubsystem::BuildRegistryFromLibraries()
{
    RegisteredLibraryDefinitions.Reset();

    for (const FSOTS_FXRegisteredLibrary& Entry : SortedLibraries)
    {
        if (!Entry.IsValid())
        {
            continue;
        }

        for (const FSOTS_FXDefinition& Def : Entry.Library->Definitions)
        {
            if (!Def.FXTag.IsValid())
            {
                continue;
            }

            if (FSOTS_FXResolvedDefinitionEntry* Existing = RegisteredLibraryDefinitions.Find(Def.FXTag))
            {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
                if (bWarnOnDuplicateFXTags)
                {
                    const FString FirstLib = Existing->SourceLibrary ? Existing->SourceLibrary->GetName() : TEXT("<unknown>");
                    UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Duplicate FX tag %s; keeping %s (P=%d,Order=%d), skipping %s."), *Def.FXTag.ToString(), *FirstLib, Existing->Priority, Existing->RegistrationOrder, *Entry.Library->GetName());
                }
#endif
                continue;
            }

            FSOTS_FXResolvedDefinitionEntry& Added = RegisteredLibraryDefinitions.Add(Def.FXTag);
            Added.Definition = Def;
            Added.SourceLibrary = Entry.Library;
            Added.Priority = Entry.Priority;
            Added.RegistrationOrder = Entry.RegistrationOrder;
        }
    }
}

bool USOTS_FXManagerSubsystem::EnsureRegistryReady() const
{
    if (bRegistryReady)
    {
        return true;
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bWarnOnTriggerBeforeRegistryReady)
    {
        static bool bWarned = false;
        if (!bWarned)
        {
            bWarned = true;
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Registry not ready; trigger ignored."));
        }
    }
#endif
    return false;
}

void USOTS_FXManagerSubsystem::RegisterCue(USOTS_FXCueDefinition* CueDefinition)
{
    if (!CueDefinition || !CueDefinition->CueTag.IsValid())
    {
        return;
    }

    CueMap.FindOrAdd(CueDefinition->CueTag) = CueDefinition;
}

FSOTS_FXHandle USOTS_FXManagerSubsystem::PlayCueByTag(FGameplayTag CueTag, const FSOTS_FXContext& Context)
{
    FSOTS_FXHandle Handle;

    UWorld* World = GetWorld();
    if (!World || !EnsureRegistryReady())
    {
        return Handle;
    }

    if (TObjectPtr<USOTS_FXCueDefinition>* FoundDefPtr = CueMap.Find(CueTag))
    {
        USOTS_FXCueDefinition* CueDef = FoundDefPtr->Get();
        if (CueDef)
        {
            Handle = SpawnCue_Internal(World, CueDef, Context);
            Handle.CueTag = CueTag;
        }
    }

    return Handle;
}

FSOTS_FXHandle USOTS_FXManagerSubsystem::PlayCueDefinition(USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context)
{
    FSOTS_FXHandle Handle;

    UWorld* World = GetWorld();
    if (!World || !CueDefinition || !EnsureRegistryReady())
    {
        return Handle;
    }

    Handle = SpawnCue_Internal(World, CueDefinition, Context);
    Handle.CueTag = CueDefinition->CueTag;

    return Handle;
}

void USOTS_FXManagerSubsystem::GetActiveFXCountsInternal(int32& OutActiveNiagara, int32& OutActiveAudio) const
{
    OutActiveNiagara = 0;
    OutActiveAudio = 0;

    for (const TPair<TObjectPtr<USOTS_FXCueDefinition>, FSOTS_FXCuePool>& Pair : CuePools)
    {
        const FSOTS_FXCuePool& Pool = Pair.Value;

        for (const FSOTS_FXPooledNiagaraEntry& Entry : Pool.NiagaraComponents)
        {
            UNiagaraComponent* NiagaraComp = Entry.Component.Get();
            if (NiagaraComp && NiagaraComp->IsActive())
            {
                ++OutActiveNiagara;
            }
        }

        for (const FSOTS_FXPooledAudioEntry& Entry : Pool.AudioComponents)
        {
            UAudioComponent* AudioComp = Entry.Component.Get();
            if (AudioComp && AudioComp->IsPlaying())
            {
                ++OutActiveAudio;
            }
        }
    }
}

FSOTS_FXPoolStats USOTS_FXManagerSubsystem::GetPoolStats() const
{
    FSOTS_FXPoolStats Stats;

    for (const TPair<TObjectPtr<USOTS_FXCueDefinition>, FSOTS_FXCuePool>& Pair : CuePools)
    {
        const FSOTS_FXCuePool& Pool = Pair.Value;

        for (const FSOTS_FXPooledNiagaraEntry& Entry : Pool.NiagaraComponents)
        {
            if (!Entry.Component)
            {
                continue;
            }

            ++Stats.TotalPooledNiagara;
            if (Entry.Component->IsActive())
            {
                ++Stats.ActiveNiagara;
            }
            else
            {
                ++Stats.FreeNiagara;
            }
        }

        for (const FSOTS_FXPooledAudioEntry& Entry : Pool.AudioComponents)
        {
            if (!Entry.Component)
            {
                continue;
            }

            ++Stats.TotalPooledAudio;
            if (Entry.Component->IsPlaying())
            {
                ++Stats.ActiveAudio;
            }
            else
            {
                ++Stats.FreeAudio;
            }
        }
    }

    return Stats;
}

void USOTS_FXManagerSubsystem::GetRegisteredCuesInternal(TArray<FGameplayTag>& OutTags, TArray<USOTS_FXCueDefinition*>& OutDefinitions) const
{
    OutTags.Reset();
    OutDefinitions.Reset();

    for (const TPair<FGameplayTag, TObjectPtr<USOTS_FXCueDefinition>>& Pair : CueMap)
    {
        OutTags.Add(Pair.Key);
        OutDefinitions.Add(Pair.Value.Get());
    }
}

FString USOTS_FXManagerSubsystem::GetFXSettingsSummary() const
{
    return FString::Printf(
        TEXT("FX Settings => Blood=%s HighIntensity=%s CameraMotion=%s"),
        bBloodEnabled ? TEXT("On") : TEXT("Off"),
        bHighIntensityFX ? TEXT("On") : TEXT("Off"),
        bCameraMotionFXEnabled ? TEXT("On") : TEXT("Off"));
}

void USOTS_FXManagerSubsystem::SetBloodEnabled(bool bEnabled)
{
    bBloodEnabled = bEnabled;
}

void USOTS_FXManagerSubsystem::SetHighIntensityFXEnabled(bool bEnabled)
{
    bHighIntensityFX = bEnabled;
}

void USOTS_FXManagerSubsystem::SetCameraMotionFXEnabled(bool bEnabled)
{
    bCameraMotionFXEnabled = bEnabled;
}

void USOTS_FXManagerSubsystem::BuildProfileData(FSOTS_FXProfileData& OutData) const
{
    OutData.bBloodEnabled = bBloodEnabled;
    OutData.bHighIntensityFX = bHighIntensityFX;
    OutData.bCameraMotionFXEnabled = bCameraMotionFXEnabled;
}

void USOTS_FXManagerSubsystem::ApplyProfileData(const FSOTS_FXProfileData& InData)
{
    bBloodEnabled = InData.bBloodEnabled;
    bHighIntensityFX = InData.bHighIntensityFX;
    bCameraMotionFXEnabled = InData.bCameraMotionFXEnabled;
}

void USOTS_FXManagerSubsystem::BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot)
{
    BuildProfileData(InOutSnapshot.FX);
}

void USOTS_FXManagerSubsystem::ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot)
{
    ApplyProfileData(Snapshot.FX);
}

void USOTS_FXManagerSubsystem::ApplyFXProfileSettings(const FSOTS_FXProfileData& InData)
{
    ApplyProfileData(InData);
}

FSOTS_FXRequestReport USOTS_FXManagerSubsystem::RequestFXCue(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target)
{
    FSOTS_FXRequest Request;
    Request.FXTag = FXCueTag;
    Request.Instigator = Instigator;
    Request.Target = Target;
    Request.SpawnSpace = ESOTS_FXSpawnSpace::World;
    Request.Rotation = FRotator::ZeroRotator;

    if (Target)
    {
        Request.Location = Target->GetActorLocation();
    }
    else if (Instigator)
    {
        Request.Location = Instigator->GetActorLocation();
    }

    return ProcessFXRequest(Request, nullptr);
}

FSOTS_FXRequestReport USOTS_FXManagerSubsystem::RequestFXCueWithReport(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target)
{
    return RequestFXCue(FXCueTag, Instigator, Target);
}

// -------------------------
// Tag-driven FX job router
// -------------------------

const FSOTS_FXDefinition* USOTS_FXManagerSubsystem::FindDefinition(FGameplayTag FXTag) const
{
    const FSOTS_FXDefinition* Definition = nullptr;
    FGameplayTag ResolvedTag;
    FString FailReason;

    if (TryResolveCue(FXTag, Definition, ResolvedTag, FailReason) == ESOTS_FXRequestResult::Success)
    {
        return Definition;
    }

    return nullptr;
}

void USOTS_FXManagerSubsystem::BroadcastResolvedFX(
    FGameplayTag FXTag,
    AActor* Instigator,
    AActor* Target,
    const FSOTS_FXDefinition* Definition,
    const FVector& Location,
    const FRotator& Rotation,
    ESOTS_FXSpawnSpace Space,
    USceneComponent* AttachComponent,
    FName AttachSocketName,
    float RequestedScale)
{
    if (!FXTag.IsValid())
    {
        return;
    }

    if (!EnsureRegistryReady())
    {
        return;
    }

    FSOTS_FXResolvedRequest Resolved;
    Resolved.FXTag          = FXTag;
    Resolved.Location       = Location;
    Resolved.Rotation       = Rotation;
    Resolved.SpawnSpace     = Space;
    Resolved.Instigator     = Instigator;
    Resolved.Target         = Target;
    Resolved.AttachComponent = AttachComponent;
    Resolved.AttachSocketName = AttachSocketName;

    if (Definition)
    {
        Resolved.NiagaraSystem = Definition->NiagaraSystem;
        Resolved.Sound         = Definition->Sound;
        Resolved.Scale         = RequestedScale > 0.0f ? RequestedScale : Definition->DefaultScale;
    }
    else
    {
        Resolved.Scale = RequestedScale > 0.0f ? RequestedScale : 1.0f;
    }

    OnFXTriggered.Broadcast(Resolved);
}

FSOTS_FXRequestResult USOTS_FXManagerSubsystem::TriggerFXByTagWithReport(
    UObject* WorldContextObject,
    const FSOTS_FXRequest& Request)
{
    (void)WorldContextObject;
    FSOTS_FXRequestResult LegacyResult;
    ProcessFXRequest(Request, &LegacyResult);
    return LegacyResult;
}

void USOTS_FXManagerSubsystem::TriggerFXByTag(
    UObject* WorldContextObject,
    FGameplayTag FXTag,
    AActor* Instigator,
    AActor* Target,
    FVector Location,
    FRotator Rotation)
{
    (void)WorldContextObject;

    FSOTS_FXRequest Request;
    Request.FXTag = FXTag;
    Request.Instigator = Instigator;
    Request.Target = Target;
    Request.Location = Location;
    Request.Rotation = Rotation;
    Request.SpawnSpace = ESOTS_FXSpawnSpace::World;

    ProcessFXRequest(Request, nullptr);
}

void USOTS_FXManagerSubsystem::TriggerAttachedFXByTag(
    UObject* WorldContextObject,
    FGameplayTag FXTag,
    AActor* Instigator,
    AActor* Target,
    USceneComponent* AttachComponent,
    FName AttachSocketName)
{
    (void)WorldContextObject;

    FSOTS_FXRequest Request;
    Request.FXTag = FXTag;
    Request.Instigator = Instigator;
    Request.Target = Target;
    Request.AttachComponent = AttachComponent;
    Request.AttachSocketName = AttachSocketName;
    Request.SpawnSpace = ESOTS_FXSpawnSpace::AttachToComponent;

    if (AttachComponent)
    {
        const FTransform Xf = AttachComponent->GetComponentTransform();
        Request.Location = Xf.GetLocation();
        Request.Rotation = Xf.Rotator();
    }

    ProcessFXRequest(Request, nullptr);
}

FSOTS_FXRequestReport USOTS_FXManagerSubsystem::ProcessFXRequest(const FSOTS_FXRequest& Request, FSOTS_FXRequestResult* OutLegacyResult)
{
    FSOTS_FXRequestReport Report;
    Report.RequestedCueTag = Request.FXTag;
    Report.ResolvedCueTag = Request.FXTag;

    FSOTS_FXRequestResult Legacy;
    Legacy.FXTag = Request.FXTag;
    Legacy.SpawnSpace = Request.SpawnSpace;
    Legacy.AttachComponent = Request.AttachComponent;
    Legacy.AttachSocketName = Request.AttachSocketName;
    Legacy.Instigator = Request.Instigator;
    Legacy.Target = Request.Target;
    Legacy.Location = Request.Location;
    Legacy.Rotation = Request.Rotation;
    Legacy.ResolvedScale = Request.Scale;

    auto SetFailure = [&](ESOTS_FXRequestResult ResultCode, ESOTS_FXRequestStatus LegacyStatus, const FString& DebugMsg)
    {
        Report.Result = ResultCode;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        Report.DebugMessage = DebugMsg;
#else
        (void)DebugMsg;
#endif
        Legacy.Status = LegacyStatus;
        Legacy.bSucceeded = false;
        MaybeLogFXFailure(Report, *DebugMsg);
    };

    UWorld* World = GetWorld();
    if (!World)
    {
        SetFailure(ESOTS_FXRequestResult::InvalidWorld, ESOTS_FXRequestStatus::MissingContext, TEXT("Invalid world context"));
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

    const FSOTS_FXDefinition* Definition = nullptr;
    FGameplayTag ResolvedTag;
    FString FailReason;

    const ESOTS_FXRequestResult ResolveResult = TryResolveCue(Request.FXTag, Definition, ResolvedTag, FailReason);
    Report.ResolvedCueTag = ResolvedTag;
    if (ResolveResult != ESOTS_FXRequestResult::Success)
    {
        ESOTS_FXRequestStatus LegacyStatus = ESOTS_FXRequestStatus::InvalidTag;
        if (ResolveResult == ESOTS_FXRequestResult::InvalidWorld)
        {
            LegacyStatus = ESOTS_FXRequestStatus::RegistryNotReady;
        }
        else if (ResolveResult == ESOTS_FXRequestResult::NotFound)
        {
            LegacyStatus = ESOTS_FXRequestStatus::DefinitionNotFound;
        }
        else
        {
            LegacyStatus = ESOTS_FXRequestStatus::InvalidTag;
        }

        SetFailure(ResolveResult, LegacyStatus, FailReason);
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

    FString PolicyFail;
    ESOTS_FXRequestResult PolicyResult = ESOTS_FXRequestResult::Success;
    bool bAllowCameraShake = true;

    // Validate attachment requirements.
    if (Request.SpawnSpace != ESOTS_FXSpawnSpace::World && !Request.AttachComponent)
    {
        SetFailure(ESOTS_FXRequestResult::InvalidParams, ESOTS_FXRequestStatus::MissingAttachment, TEXT("Missing attach target"));
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

    FSOTS_FXExecutionParams ExecParams = BuildExecutionParams(Request, Definition, ResolvedTag);

    if (!EvaluateFXPolicy(*Definition, ExecParams, PolicyResult, PolicyFail, bAllowCameraShake))
    {
        SetFailure(PolicyResult, ESOTS_FXRequestStatus::MissingContext, PolicyFail);
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

    ExecParams.bAllowCameraShake = bAllowCameraShake;
    const FSOTS_FXRequestReport ExecReport = ExecuteCue(ExecParams, Definition, Request, OutLegacyResult ? OutLegacyResult : &Legacy);
    (void)Legacy;

    if (ExecReport.Result != ESOTS_FXRequestResult::Success)
    {
        MaybeLogFXFailure(ExecReport, TEXT("ExecuteCue"));
    }

    return ExecReport;
}

void USOTS_FXManagerSubsystem::MaybeLogFXFailure(const FSOTS_FXRequestReport& Report, const TCHAR* Reason) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bLogFXRequestFailures)
    {
        return;
    }

    const FString StatusStr = StaticEnum<ESOTS_FXRequestResult>()->GetNameStringByValue(static_cast<int64>(Report.Result));
    const FString TagStr = Report.RequestedCueTag.IsValid() ? Report.RequestedCueTag.ToString() : TEXT("None");
    const FString Debug = Report.DebugMessage.IsEmpty() ? TEXT("") : Report.DebugMessage;
    UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] FX request failed (%s): Tag=%s Result=%s %s"), Reason, *TagStr, *StatusStr, *Debug);
#endif
}

FSOTS_FXRequestResult USOTS_FXManagerSubsystem::ConvertReportToLegacy(const FSOTS_FXRequestReport& Report, const FSOTS_FXRequest& Request, const FVector& ResolvedLocation, const FRotator& ResolvedRotation, float ResolvedScale, const FSOTS_FXDefinition* Definition) const
{
    FSOTS_FXRequestResult Legacy;
    Legacy.FXTag = Report.ResolvedCueTag.IsValid() ? Report.ResolvedCueTag : Report.RequestedCueTag;
    Legacy.Location = ResolvedLocation;
    Legacy.Rotation = ResolvedRotation;
    Legacy.SpawnSpace = Request.SpawnSpace;
    Legacy.AttachComponent = Request.AttachComponent;
    Legacy.AttachSocketName = Request.AttachSocketName;
    Legacy.Instigator = Request.Instigator;
    Legacy.Target = Request.Target;
    Legacy.ResolvedScale = ResolvedScale;
    Legacy.NiagaraSystem = Definition ? Definition->NiagaraSystem : nullptr;
    Legacy.Sound = Definition ? Definition->Sound : nullptr;

    switch (Report.Result)
    {
        case ESOTS_FXRequestResult::Success:
            Legacy.Status = ESOTS_FXRequestStatus::Success;
            Legacy.bSucceeded = true;
            break;
        case ESOTS_FXRequestResult::NotFound:
            Legacy.Status = ESOTS_FXRequestStatus::DefinitionNotFound;
            Legacy.bSucceeded = false;
            break;
        case ESOTS_FXRequestResult::InvalidWorld:
            Legacy.Status = ESOTS_FXRequestStatus::RegistryNotReady;
            Legacy.bSucceeded = false;
            break;
        case ESOTS_FXRequestResult::InvalidParams:
            Legacy.Status = ESOTS_FXRequestStatus::InvalidTag;
            Legacy.bSucceeded = false;
            break;
        case ESOTS_FXRequestResult::DisabledByPolicy:
        case ESOTS_FXRequestResult::FailedToSpawn:
        default:
            Legacy.Status = ESOTS_FXRequestStatus::MissingContext;
            Legacy.bSucceeded = false;
            break;
    }

    return Legacy;
}

bool USOTS_FXManagerSubsystem::EvaluateFXPolicy(const FSOTS_FXDefinition& Definition, const FSOTS_FXExecutionParams& Params, ESOTS_FXRequestResult& OutResult, FString& OutFailReason, bool& bOutAllowCameraShake) const
{
    (void)Params;

    OutResult = ESOTS_FXRequestResult::Success;
    OutFailReason.Reset();
    bOutAllowCameraShake = true;

    switch (Definition.ToggleBehavior)
    {
        case ESOTS_FXToggleBehavior::ForceDisable:
            OutResult = ESOTS_FXRequestResult::DisabledByPolicy;
            OutFailReason = TEXT("ForceDisabled");
            return false;
        case ESOTS_FXToggleBehavior::IgnoreGlobalToggles:
            // Do nothing – ignore global gating entirely.
            break;
        case ESOTS_FXToggleBehavior::RespectGlobalToggles:
        default:
            if (!bBloodEnabled && Definition.bIsBloodFX)
            {
                OutResult = ESOTS_FXRequestResult::DisabledByPolicy;
                OutFailReason = TEXT("BloodDisabled");
                return false;
            }

            if (!bHighIntensityFX && Definition.bIsHighIntensityFX)
            {
                OutResult = ESOTS_FXRequestResult::DisabledByPolicy;
                OutFailReason = TEXT("HighIntensityDisabled");
                return false;
            }

            if (!bCameraMotionFXEnabled && Definition.CameraShakeClass && !Definition.bCameraShakeIgnoresGlobalToggle)
            {
                bOutAllowCameraShake = false;
            }
            break;
    }

    return true;
}

ESOTS_FXRequestResult USOTS_FXManagerSubsystem::TryResolveCue(FGameplayTag FXTag, const FSOTS_FXDefinition*& OutDefinition, FGameplayTag& OutResolvedTag, FString& OutFailReason) const
{
    OutDefinition = nullptr;
    OutResolvedTag = FXTag;
    OutFailReason.Reset();

    const int32 LibraryCount = SortedLibraries.Num();

    if (!FXTag.IsValid())
    {
        OutFailReason = TEXT("Invalid tag");
        return ESOTS_FXRequestResult::InvalidParams;
    }

    if (!EnsureRegistryReady())
    {
        OutFailReason = TEXT("Registry not ready");
        return ESOTS_FXRequestResult::InvalidWorld;
    }

    if (const FSOTS_FXResolvedDefinitionEntry* Entry = RegisteredLibraryDefinitions.Find(FXTag))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebugLogCueResolution)
        {
            const FString LibName = Entry->SourceLibrary ? Entry->SourceLibrary->GetName() : TEXT("<unknown>");
            UE_LOG(LogTemp, Log, TEXT("[SOTS_FX] Resolved %s via %s (P=%d,Order=%d)."), *FXTag.ToString(), *LibName, Entry->Priority, Entry->RegistrationOrder);
        }
#endif
        OutDefinition = &Entry->Definition;
        return ESOTS_FXRequestResult::Success;
    }

    OutFailReason = FString::Printf(TEXT("Definition not found (searched %d libraries)"), LibraryCount);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogCueResolution)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Failed to resolve %s after searching %d libraries. Order: %s"), *FXTag.ToString(), LibraryCount, *BuildLibraryOrderDebugString());
    }
#endif
    return ESOTS_FXRequestResult::NotFound;
}

FSOTS_FXExecutionParams USOTS_FXManagerSubsystem::BuildExecutionParams(const FSOTS_FXRequest& Request, const FSOTS_FXDefinition* Definition, const FGameplayTag& ResolvedTag) const
{
    FSOTS_FXExecutionParams Params;
    Params.WorldContextObject = const_cast<USOTS_FXManagerSubsystem*>(this);
    Params.RequestedTag = Request.FXTag;
    Params.ResolvedTag = ResolvedTag;
    Params.Instigator = Request.Instigator;
    Params.Target = Request.Target;
    Params.AttachComponent = Request.AttachComponent;
    Params.AttachSocketName = Request.AttachSocketName;
    Params.bHasSurfaceNormal = Request.bHasSurfaceNormal;
    Params.SurfaceNormal = Request.SurfaceNormal;
    Params.SpawnSpace = (Request.SpawnSpace != ESOTS_FXSpawnSpace::World)
        ? Request.SpawnSpace
        : (Definition ? Definition->DefaultSpace : ESOTS_FXSpawnSpace::World);
    Params.Location = Request.Location;
    Params.Rotation = Request.Rotation;
    Params.Scale = Request.Scale > 0.0f ? Request.Scale : 1.0f;
    Params.bAllowCameraShake = true;
    if (Definition)
    {
        Params.Scale *= Definition->DefaultScale;
    }

    return Params;
}

void USOTS_FXManagerSubsystem::ApplySurfaceAlignment(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, FVector& InOutLocation, FRotator& InOutRotation) const
{
    if (!Definition || !Definition->bAlignToSurfaceNormal || !Params.bHasSurfaceNormal || Params.SurfaceNormal.IsNearlyZero())
    {
        return;
    }

    const FVector Normal = Params.SurfaceNormal.GetSafeNormal();
    InOutRotation = FRotationMatrix::MakeFromZ(Normal).Rotator();
    (void)InOutLocation;
}

void USOTS_FXManagerSubsystem::ApplyOffsets(const FSOTS_FXDefinition* Definition, bool bAttached, FVector& InOutLocation, FRotator& InOutRotation) const
{
    if (!Definition)
    {
        return;
    }

    const FVector Offset = Definition->LocationOffset;
    const FRotator RotOffset = Definition->RotationOffset;

    if (bAttached)
    {
        InOutLocation += Offset;
        InOutRotation += RotOffset;
    }
    else
    {
        InOutLocation += InOutRotation.RotateVector(Offset);
        InOutRotation += RotOffset;
    }
}

UNiagaraComponent* USOTS_FXManagerSubsystem::SpawnNiagara(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, const FVector& SpawnLocation, const FRotator& SpawnRotation, float SpawnScale, UWorld* World) const
{
    if (!Definition || !World)
    {
        return nullptr;
    }

    UNiagaraSystem* NiagaraSystem = Definition->NiagaraSystem.LoadSynchronous();
    if (!NiagaraSystem)
    {
        return nullptr;
    }

    UNiagaraComponent* NiagaraComp = nullptr;
    if (Params.SpawnSpace != ESOTS_FXSpawnSpace::World && Params.AttachComponent)
    {
        NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
            NiagaraSystem,
            Params.AttachComponent,
            Params.AttachSocketName,
            SpawnLocation,
            SpawnRotation,
            EAttachLocation::KeepRelativeOffset,
            /*bAutoDestroy=*/true,
            /*bAutoActivate=*/true,
            ENCPoolMethod::None,
            /*bPreCullCheck=*/true);

        if (NiagaraComp)
        {
            NiagaraComp->SetRelativeScale3D(FVector(SpawnScale));
        }
    }
    else
    {
        NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            NiagaraSystem,
            SpawnLocation,
            SpawnRotation,
            FVector(SpawnScale),
            /*bAutoDestroy=*/true,
            /*bAutoActivate=*/true,
            ENCPoolMethod::None,
            /*bPreCullCheck=*/true);
    }

    if (NiagaraComp)
    {
        ApplyNiagaraParameters(NiagaraComp, Definition);
    }

    return NiagaraComp;
}

void USOTS_FXManagerSubsystem::ApplyNiagaraParameters(UNiagaraComponent* NiagaraComp, const FSOTS_FXDefinition* Definition) const
{
    if (!NiagaraComp || !Definition)
    {
        return;
    }

    for (const TPair<FName, float>& Pair : Definition->NiagaraFloatParameters)
    {
        NiagaraComp->SetVariableFloat(Pair.Key, Pair.Value);
    }

    for (const TPair<FName, FVector>& Pair : Definition->NiagaraVectorParameters)
    {
        NiagaraComp->SetVariableVec3(Pair.Key, Pair.Value);
    }

    for (const TPair<FName, FLinearColor>& Pair : Definition->NiagaraColorParameters)
    {
        NiagaraComp->SetVariableLinearColor(Pair.Key, Pair.Value);
    }
}

UAudioComponent* USOTS_FXManagerSubsystem::SpawnAudio(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, const FVector& SpawnLocation, const FRotator& SpawnRotation, UWorld* World) const
{
    if (!Definition || !World)
    {
        return nullptr;
    }

    USoundBase* Sound = Definition->Sound.LoadSynchronous();
    if (!Sound)
    {
        return nullptr;
    }

    USoundAttenuation* Attenuation = Definition->SoundAttenuation.IsNull() ? nullptr : Definition->SoundAttenuation.LoadSynchronous();
    USoundConcurrency* Concurrency = Definition->SoundConcurrency.IsNull() ? nullptr : Definition->SoundConcurrency.LoadSynchronous();

    if (Params.SpawnSpace != ESOTS_FXSpawnSpace::World && Params.AttachComponent)
    {
        return UGameplayStatics::SpawnSoundAttached(
            Sound,
            Params.AttachComponent,
            Params.AttachSocketName,
            SpawnLocation,
            EAttachLocation::KeepRelativeOffset,
            /*bStopWhenAttachedToDestroyed=*/false,
            Definition->VolumeMultiplier,
            Definition->PitchMultiplier,
            /*StartTime=*/0.0f,
            Attenuation,
            Concurrency,
            /*bAutoDestroy=*/true);
    }

    return UGameplayStatics::SpawnSoundAtLocation(
        World,
        Sound,
        SpawnLocation,
        SpawnRotation,
        Definition->VolumeMultiplier,
        Definition->PitchMultiplier,
        /*StartTime=*/0.0f,
        Attenuation,
        Concurrency,
        /*bAutoDestroy=*/true);
}

void USOTS_FXManagerSubsystem::TriggerCameraShake(const FSOTS_FXDefinition* Definition, UWorld* World) const
{
    if (!Definition || !World || !Definition->CameraShakeClass)
    {
        return;
    }

    if (!bCameraMotionFXEnabled)
    {
        return;
    }

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        PC->ClientStartCameraShake(Definition->CameraShakeClass, Definition->CameraShakeScale);
    }
}

FSOTS_FXRequestReport USOTS_FXManagerSubsystem::ExecuteCue(const FSOTS_FXExecutionParams& Params, const FSOTS_FXDefinition* Definition, const FSOTS_FXRequest& OriginalRequest, FSOTS_FXRequestResult* OutLegacyResult)
{
    FSOTS_FXRequestReport Report;
    Report.RequestedCueTag = Params.RequestedTag;
    Report.ResolvedCueTag = Params.ResolvedTag;

    FSOTS_FXRequestResult Legacy;
    Legacy.FXTag = Params.ResolvedTag.IsValid() ? Params.ResolvedTag : Params.RequestedTag;
    Legacy.SpawnSpace = Params.SpawnSpace;
    Legacy.AttachComponent = Params.AttachComponent;
    Legacy.AttachSocketName = Params.AttachSocketName;
    Legacy.Instigator = Params.Instigator;
    Legacy.Target = Params.Target;
    Legacy.Location = Params.Location;
    Legacy.Rotation = Params.Rotation;
    Legacy.ResolvedScale = Params.Scale;

    UWorld* World = GetWorld();
    if (!World)
    {
        Report.Result = ESOTS_FXRequestResult::InvalidWorld;
        Legacy.Status = ESOTS_FXRequestStatus::MissingContext;
        Legacy.bSucceeded = false;
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

    FVector SpawnLocation = Params.Location;
    FRotator SpawnRotation = Params.Rotation;

    ApplySurfaceAlignment(Definition, Params, SpawnLocation, SpawnRotation);
    ApplyOffsets(Definition, Params.SpawnSpace != ESOTS_FXSpawnSpace::World, SpawnLocation, SpawnRotation);

    const float SpawnScale = Params.Scale > 0.0f ? Params.Scale : 1.0f;
    const bool bShouldTriggerCameraShake = Definition && Definition->CameraShakeClass && Params.bAllowCameraShake;
    const bool bHasCameraShakeConfigured = Definition && Definition->CameraShakeClass;

    UNiagaraComponent* NiagaraComp = SpawnNiagara(Definition, Params, SpawnLocation, SpawnRotation, SpawnScale, World);
    UAudioComponent* AudioComp = SpawnAudio(Definition, Params, SpawnLocation, SpawnRotation, World);
    if (bShouldTriggerCameraShake)
    {
        TriggerCameraShake(Definition, World);
    }

    Report.SpawnedObject = NiagaraComp ? static_cast<UObject*>(NiagaraComp) : static_cast<UObject*>(AudioComp);

    const bool bAnyFXSpawned = (NiagaraComp || AudioComp);

    if (!bShouldTriggerCameraShake && !bAnyFXSpawned && bHasCameraShakeConfigured && !Params.bAllowCameraShake)
    {
        Report.Result = ESOTS_FXRequestResult::DisabledByPolicy;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        const FString ShakeNote = TEXT("ShakeSkipped(CameraMotionDisabled)");
        Report.DebugMessage = Report.DebugMessage.IsEmpty() ? ShakeNote : FString::Printf(TEXT("%s %s"), *Report.DebugMessage, *ShakeNote);
#endif
    }
    else if (bAnyFXSpawned || bShouldTriggerCameraShake)
    {
        Report.Result = ESOTS_FXRequestResult::Success;
    }
    else
    {
        Report.Result = ESOTS_FXRequestResult::FailedToSpawn;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        Report.DebugMessage = TEXT("No FX assets spawned.");
        if (bLogFXRequestFailures)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Failed to spawn FX for %s (no assets)."), *Params.RequestedTag.ToString());
        }
#endif
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!Params.bAllowCameraShake && bHasCameraShakeConfigured)
    {
        const FString ShakeNote = TEXT("ShakeSkipped(CameraMotionDisabled)");
        Report.DebugMessage = Report.DebugMessage.IsEmpty() ? ShakeNote : FString::Printf(TEXT("%s %s"), *Report.DebugMessage, *ShakeNote);
    }
#endif

    Legacy = ConvertReportToLegacy(Report, OriginalRequest, SpawnLocation, SpawnRotation, SpawnScale, Definition);

    if (OutLegacyResult)
    {
        *OutLegacyResult = Legacy;
    }

    if (Report.Result == ESOTS_FXRequestResult::Success)
    {
        BroadcastResolvedFX(
            Params.ResolvedTag,
            Params.Instigator,
            Params.Target,
            Definition,
            SpawnLocation,
            SpawnRotation,
            Params.SpawnSpace,
            Params.AttachComponent,
            Params.AttachSocketName,
            SpawnScale);
    }

    return Report;
}

// -------------------------
// Pool helpers
// -------------------------

UNiagaraComponent* USOTS_FXManagerSubsystem::AcquireNiagaraComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition)
{
    if (!World || !CueDefinition || !CueDefinition->NiagaraSystem)
    {
        return nullptr;
    }

    FSOTS_FXCuePool& Pool = CuePools.FindOrAdd(CueDefinition);
    CleanupInvalidPoolEntries(Pool);

    const float NowSeconds = World->GetTimeSeconds();
    const int32 PoolLimit = FMath::Max(1, MaxNiagaraPoolSizePerCue);

    int32 ActiveCount = 0;

    for (FSOTS_FXPooledNiagaraEntry& Entry : Pool.NiagaraComponents)
    {
        UNiagaraComponent* Comp = Entry.Component.Get();
        if (!Comp)
        {
            continue;
        }

        if (Comp->IsActive())
        {
            ++ActiveCount;
        }
        else
        {
            Entry.bInUse = false;
        }

        if (!Entry.bInUse && !Comp->IsActive())
        {
            Comp->OnSystemFinished.RemoveDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
            Comp->OnSystemFinished.AddUniqueDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);

            Entry.bInUse = true;
            Entry.LastUseTime = NowSeconds;
            return Comp;
        }
    }

    const bool bAtActiveCap = ActiveCount >= MaxActiveNiagaraPerCue;

    if (bAtActiveCap)
    {
        if (bRecycleWhenExhausted)
        {
            if (UNiagaraComponent* ReclaimedActive = ReclaimNiagaraComponent(Pool, *CueDefinition->GetName(), NowSeconds))
            {
                return ReclaimedActive;
            }
        }
        else
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bLogPoolActions)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Niagara active cap hit for %s (Active=%d, Cap=%d)."), *CueDefinition->GetName(), ActiveCount, MaxActiveNiagaraPerCue);
            }
#endif
            return nullptr;
        }
    }

    if (Pool.NiagaraComponents.Num() < PoolLimit)
    {
        UNiagaraComponent* NewComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            CueDefinition->NiagaraSystem,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            FVector::OneVector,
            /*bAutoDestroy=*/false,
            /*bAutoActivate=*/false
        );

        if (NewComp)
        {
            NewComp->OnSystemFinished.AddUniqueDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);

            FSOTS_FXPooledNiagaraEntry& NewEntry = Pool.NiagaraComponents.AddDefaulted_GetRef();
            NewEntry.Component = NewComp;
            NewEntry.bInUse = true;
            NewEntry.LastUseTime = NowSeconds;
            return NewComp;
        }
    }

    if (Pool.NiagaraComponents.Num() >= PoolLimit)
    {
        if (bRecycleWhenExhausted)
        {
            if (UNiagaraComponent* Reclaimed = ReclaimNiagaraComponent(Pool, *CueDefinition->GetName(), NowSeconds))
            {
                return Reclaimed;
            }
        }
        else
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bLogPoolActions)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Niagara pool cap reached for %s (Size=%d, Cap=%d)."), *CueDefinition->GetName(), Pool.NiagaraComponents.Num(), PoolLimit);
            }
#endif
        }
    }

    return nullptr;
}

UAudioComponent* USOTS_FXManagerSubsystem::AcquireAudioComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition)
{
    if (!World || !CueDefinition || !CueDefinition->Sound)
    {
        return nullptr;
    }

    FSOTS_FXCuePool& Pool = CuePools.FindOrAdd(CueDefinition);
    CleanupInvalidPoolEntries(Pool);

    const float NowSeconds = World->GetTimeSeconds();
    const int32 PoolLimit = FMath::Max(1, MaxAudioPoolSizePerCue);

    int32 ActiveCount = 0;

    for (FSOTS_FXPooledAudioEntry& Entry : Pool.AudioComponents)
    {
        UAudioComponent* Comp = Entry.Component.Get();
        if (!Comp)
        {
            continue;
        }

        if (Comp->IsPlaying())
        {
            ++ActiveCount;
        }
        else
        {
            Entry.bInUse = false;
        }

        if (!Entry.bInUse && !Comp->IsPlaying())
        {
            Comp->OnAudioFinishedNative.RemoveAll(this);
            Comp->OnAudioFinishedNative.AddUObject(this, &USOTS_FXManagerSubsystem::HandleAudioFinishedNative);

            Entry.bInUse = true;
            Entry.LastUseTime = NowSeconds;
            return Comp;
        }
    }

    const bool bAtActiveCap = ActiveCount >= MaxActiveAudioPerCue;

    if (bAtActiveCap)
    {
        if (bRecycleWhenExhausted)
        {
            if (UAudioComponent* ReclaimedActive = ReclaimAudioComponent(Pool, *CueDefinition->GetName(), NowSeconds))
            {
                return ReclaimedActive;
            }
        }
        else
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bLogPoolActions)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Audio active cap hit for %s (Active=%d, Cap=%d)."), *CueDefinition->GetName(), ActiveCount, MaxActiveAudioPerCue);
            }
#endif
            return nullptr;
        }
    }

    if (Pool.AudioComponents.Num() < PoolLimit)
    {
        UAudioComponent* NewComp = UGameplayStatics::SpawnSoundAtLocation(
            World,
            CueDefinition->Sound,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            1.0f,
            1.0f,
            0.0f,
            nullptr,
            nullptr,
            false
        );

        if (NewComp)
        {
            NewComp->bAutoDestroy = false;
            NewComp->OnAudioFinishedNative.RemoveAll(this);
            NewComp->OnAudioFinishedNative.AddUObject(this, &USOTS_FXManagerSubsystem::HandleAudioFinishedNative);

            FSOTS_FXPooledAudioEntry& NewEntry = Pool.AudioComponents.AddDefaulted_GetRef();
            NewEntry.Component = NewComp;
            NewEntry.bInUse = true;
            NewEntry.LastUseTime = NowSeconds;
            return NewComp;
        }
    }

    if (Pool.AudioComponents.Num() >= PoolLimit)
    {
        if (bRecycleWhenExhausted)
        {
            if (UAudioComponent* Reclaimed = ReclaimAudioComponent(Pool, *CueDefinition->GetName(), NowSeconds))
            {
                return Reclaimed;
            }
        }
        else
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bLogPoolActions)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Audio pool cap reached for %s (Size=%d, Cap=%d)."), *CueDefinition->GetName(), Pool.AudioComponents.Num(), PoolLimit);
            }
#endif
        }
    }

    return nullptr;
}

void USOTS_FXManagerSubsystem::CleanupInvalidPoolEntries(FSOTS_FXCuePool& Pool) const
{
    Pool.NiagaraComponents.RemoveAll([](const FSOTS_FXPooledNiagaraEntry& Entry)
    {
        return !IsValid(Entry.Component.Get());
    });

    Pool.AudioComponents.RemoveAll([](const FSOTS_FXPooledAudioEntry& Entry)
    {
        return !IsValid(Entry.Component.Get());
    });
}

UNiagaraComponent* USOTS_FXManagerSubsystem::ReclaimNiagaraComponent(FSOTS_FXCuePool& Pool, const TCHAR* CueName, float NowSeconds)
{
    FSOTS_FXPooledNiagaraEntry* Oldest = nullptr;

    for (FSOTS_FXPooledNiagaraEntry& Entry : Pool.NiagaraComponents)
    {
        if (!Entry.Component)
        {
            continue;
        }

        if (!Oldest || Entry.LastUseTime < Oldest->LastUseTime)
        {
            Oldest = &Entry;
        }
    }

    if (!Oldest || !Oldest->Component)
    {
        return nullptr;
    }

    Oldest->Component->DeactivateImmediate();
    Oldest->Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Oldest->Component->OnSystemFinished.AddUniqueDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
    Oldest->Component->SetHiddenInGame(true);
    Oldest->bInUse = true;
    Oldest->LastUseTime = NowSeconds;

    LogPoolEvent(FString::Printf(TEXT("[SOTS_FX] Reclaimed Niagara component for cue %s."), CueName));

    return Oldest->Component;
}

UAudioComponent* USOTS_FXManagerSubsystem::ReclaimAudioComponent(FSOTS_FXCuePool& Pool, const TCHAR* CueName, float NowSeconds)
{
    FSOTS_FXPooledAudioEntry* Oldest = nullptr;

    for (FSOTS_FXPooledAudioEntry& Entry : Pool.AudioComponents)
    {
        if (!Entry.Component)
        {
            continue;
        }

        if (!Oldest || Entry.LastUseTime < Oldest->LastUseTime)
        {
            Oldest = &Entry;
        }
    }

    if (!Oldest || !Oldest->Component)
    {
        return nullptr;
    }

    Oldest->Component->Stop();
    Oldest->Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Oldest->Component->OnAudioFinishedNative.RemoveAll(this);
    Oldest->Component->OnAudioFinishedNative.AddUObject(this, &USOTS_FXManagerSubsystem::HandleAudioFinishedNative);
    Oldest->bInUse = true;
    Oldest->LastUseTime = NowSeconds;

    LogPoolEvent(FString::Printf(TEXT("[SOTS_FX] Reclaimed audio component for cue %s."), CueName));

    return Oldest->Component;
}

void USOTS_FXManagerSubsystem::MarkNiagaraEntryFree(UNiagaraComponent* Component)
{
    if (!Component)
    {
        return;
    }

    for (TPair<TObjectPtr<USOTS_FXCueDefinition>, FSOTS_FXCuePool>& Pair : CuePools)
    {
        for (FSOTS_FXPooledNiagaraEntry& Entry : Pair.Value.NiagaraComponents)
        {
            if (Entry.Component == Component)
            {
                Entry.bInUse = false;
                Entry.LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
                Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
                Component->SetHiddenInGame(true);
                return;
            }
        }
    }
}

void USOTS_FXManagerSubsystem::MarkAudioEntryFree(UAudioComponent* Component)
{
    if (!Component)
    {
        return;
    }

    for (TPair<TObjectPtr<USOTS_FXCueDefinition>, FSOTS_FXCuePool>& Pair : CuePools)
    {
        for (FSOTS_FXPooledAudioEntry& Entry : Pair.Value.AudioComponents)
        {
            if (Entry.Component == Component)
            {
                Entry.bInUse = false;
                Entry.LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
                Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
                return;
            }
        }
    }
}

void USOTS_FXManagerSubsystem::LogPoolEvent(const FString& Message) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogPoolActions)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
    }
#else
    (void)Message;
#endif
}

void USOTS_FXManagerSubsystem::HandleNiagaraFinished(UNiagaraComponent* FinishedComponent)
{
    MarkNiagaraEntryFree(FinishedComponent);
    LogPoolEvent(TEXT("[SOTS_FX] Niagara component finished and returned to pool."));
}

void USOTS_FXManagerSubsystem::HandleAudioFinished()
{
    // Dynamic delegate no longer provides the component; prefer native binding.
}

void USOTS_FXManagerSubsystem::HandleAudioFinishedNative(UAudioComponent* FinishedComponent)
{
    MarkAudioEntryFree(FinishedComponent);
    LogPoolEvent(TEXT("[SOTS_FX] Audio component finished and returned to pool."));
}

void USOTS_FXManagerSubsystem::ApplyContextToNiagara(UNiagaraComponent* NiagaraComp, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context)
{
    if (!NiagaraComp || !CueDefinition)
    {
        return;
    }

    NiagaraComp->SetAsset(CueDefinition->NiagaraSystem);
    NiagaraComp->SetAutoDestroy(false);

    if (Context.bAttach && Context.AttachComponent)
    {
        NiagaraComp->AttachToComponent(
            Context.AttachComponent,
            FAttachmentTransformRules::KeepRelativeTransform,
            Context.AttachSocketName
        );
        NiagaraComp->SetRelativeLocation(Context.Location);
        NiagaraComp->SetRelativeRotation(Context.Rotation);
        NiagaraComp->SetRelativeScale3D(FVector(Context.Scale));
    }
    else
    {
        NiagaraComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        NiagaraComp->SetWorldLocation(Context.Location);
        NiagaraComp->SetWorldRotation(Context.Rotation);
        NiagaraComp->SetWorldScale3D(FVector(Context.Scale));
    }

    NiagaraComp->SetHiddenInGame(false);
    NiagaraComp->Activate(true); // reset & play
}

void USOTS_FXManagerSubsystem::ApplyContextToAudio(UAudioComponent* AudioComp, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context)
{
    if (!AudioComp || !CueDefinition)
    {
        return;
    }

    AudioComp->SetSound(CueDefinition->Sound);
    AudioComp->bAutoDestroy = false;

    // Ensure it's stopped before repositioning
    AudioComp->Stop();
    AudioComp->SetComponentTickEnabled(true);

    if (Context.bAttach && Context.AttachComponent)
    {
        AudioComp->AttachToComponent(
            Context.AttachComponent,
            FAttachmentTransformRules::KeepRelativeTransform,
            Context.AttachSocketName
        );
        AudioComp->SetRelativeLocation(Context.Location);
        AudioComp->SetRelativeRotation(Context.Rotation);
    }
    else
    {
        AudioComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        AudioComp->SetWorldLocation(Context.Location);
        AudioComp->SetWorldRotation(Context.Rotation);
    }

    AudioComp->Play();
}

// -------------------------
// Spawn internal
// -------------------------

FSOTS_FXHandle USOTS_FXManagerSubsystem::SpawnCue_Internal(UWorld* World, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context)
{
    FSOTS_FXHandle Handle;

    if (!World || !CueDefinition)
    {
        return Handle;
    }

    bool bAllowCameraShake = true;

    switch (CueDefinition->ToggleBehavior)
    {
        case ESOTS_FXToggleBehavior::ForceDisable:
            return Handle;
        case ESOTS_FXToggleBehavior::IgnoreGlobalToggles:
            break;
        case ESOTS_FXToggleBehavior::RespectGlobalToggles:
        default:
            if (!bBloodEnabled && CueDefinition->bIsBloodFX)
            {
                return Handle;
            }

            if (!bHighIntensityFX && CueDefinition->bIsHighIntensityFX)
            {
                return Handle;
            }

            if (!bCameraMotionFXEnabled && CueDefinition->CameraShakeClass && !CueDefinition->bCameraShakeIgnoresGlobalToggle)
            {
                bAllowCameraShake = false;
            }
            break;
    }

    // ---- VFX (Niagara, pooled) ----
    if (CueDefinition->NiagaraSystem)
    {
        UNiagaraComponent* NiagaraComp = AcquireNiagaraComponent(World, CueDefinition);
        if (NiagaraComp)
        {
            ApplyContextToNiagara(NiagaraComp, CueDefinition, Context);
            Handle.NiagaraComponent = NiagaraComp;
        }
    }
    else if (CueDefinition->CascadeSystem)
    {
        // Optional legacy fallback – NOT pooled
        UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
            World,
            CueDefinition->CascadeSystem,
            Context.Location,
            Context.Rotation,
            FVector(Context.Scale),
            /*bAutoDestroy=*/true
        );
        (void)PSC;
    }

    // ---- Audio (pooled) ----
    if (CueDefinition->Sound)
    {
        UAudioComponent* AudioComp = AcquireAudioComponent(World, CueDefinition);
        if (AudioComp)
        {
            ApplyContextToAudio(AudioComp, CueDefinition, Context);
            Handle.AudioComponent = AudioComp;
        }
    }

    // ---- Camera Shake (non-pooled, cheap enough) ----
    if (CueDefinition->CameraShakeClass && bAllowCameraShake)
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->ClientStartCameraShake(CueDefinition->CameraShakeClass, CueDefinition->CameraShakeScale);
        }
    }

    return Handle;
}
void USOTS_FXManagerSubsystem::ValidateCueDefinition(const FSOTS_FXDefinition& Def, TSet<FGameplayTag>& SeenTags) const
{
    if (!Def.FXTag.IsValid())
    {
        return;
    }

    const bool bIsDuplicate = SeenTags.Contains(Def.FXTag);
    if (bIsDuplicate)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnDuplicateFXTags)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Duplicate FX tag %s detected during validation; first definition wins."), *Def.FXTag.ToString());
        }
#endif
        return;
    }

    SeenTags.Add(Def.FXTag);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const bool bMissingNiagara = !Def.NiagaraSystem.IsValid() && !Def.NiagaraSystem.ToSoftObjectPath().IsValid();
    const bool bMissingSound = !Def.Sound.IsValid() && !Def.Sound.ToSoftObjectPath().IsValid();
    if (bWarnOnMissingFXAssets && bMissingNiagara && bMissingSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] FX tag %s has no Niagara or Sound asset set."), *Def.FXTag.ToString());
    }
#endif
}

void USOTS_FXManagerSubsystem::ValidateLibraryDefinitions() const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    TSet<FGameplayTag> SeenTags;

    for (const TPair<FGameplayTag, FSOTS_FXResolvedDefinitionEntry>& Pair : RegisteredLibraryDefinitions)
    {
        ValidateCueDefinition(Pair.Value.Definition, SeenTags);
    }

    for (const FSOTS_FXRegisteredLibrary& Entry : RegisteredLibraries)
    {
        if (!Entry.Library && bWarnOnMissingFXAssets)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Validation skipped null library reference."));
        }
    }
#endif
}
