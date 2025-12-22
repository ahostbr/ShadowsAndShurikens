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
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystemComponent.h"

DEFINE_LOG_CATEGORY(LogSOTS_FX);

TWeakObjectPtr<USOTS_FXManagerSubsystem> USOTS_FXManagerSubsystem::SingletonInstance = nullptr;

void USOTS_FXManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SingletonInstance = this;

    CueMap.Reset();
    TagPools.Reset();
    NiagaraComponentTags.Reset();
    AudioComponentTags.Reset();
    ActiveNiagaraCounts.Reset();
    ActiveAudioCounts.Reset();
    TotalPooledNiagara = 0;
    TotalPooledAudio = 0;
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
    TagPools.Reset();
    NiagaraComponentTags.Reset();
    AudioComponentTags.Reset();
    ActiveNiagaraCounts.Reset();
    ActiveAudioCounts.Reset();
    TotalPooledNiagara = 0;
    TotalPooledAudio = 0;
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Rejecting library registration (null or pending kill)."));
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
                UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Updated library %s priority to %d (order %d)."), *Library->GetName(), EffectivePriority, Existing->RegistrationOrder);
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
            UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Registered library %s (Priority=%d, Order=%d)."), *Library->GetName(), EffectivePriority, NewEntry.RegistrationOrder);
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Soft library entry is empty; skipping."));
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Soft library %s failed to load."), *Entry.Library.ToString());
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Dropping invalid library entry %s."), *Name);
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
        UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Library order => %s"), *BuildLibraryOrderDebugString());
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

    UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Library order => %s"), *BuildLibraryOrderDebugString());
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
                    UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Duplicate FX tag %s; keeping %s (P=%d,Order=%d), skipping %s."), *Def.FXTag.ToString(), *FirstLib, Existing->Priority, Existing->RegistrationOrder, *Entry.Library->GetName());
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Registry not ready; trigger ignored."));
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

    for (const TPair<FGameplayTag, FSOTS_FXTagPool>& Pair : TagPools)
    {
        for (const FSOTS_FXPoolEntry& Entry : Pair.Value.NiagaraEntries)
        {
            if (const UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(Entry.Component.Get()))
            {
                if (NiagaraComp->IsActive())
                {
                    ++OutActiveNiagara;
                }
            }
        }

        for (const FSOTS_FXPoolEntry& Entry : Pair.Value.AudioEntries)
        {
            if (const UAudioComponent* AudioComp = Cast<UAudioComponent>(Entry.Component.Get()))
            {
                if (AudioComp->IsPlaying())
                {
                    ++OutActiveAudio;
                }
            }
        }
    }
}

FSOTS_FXPoolStats USOTS_FXManagerSubsystem::GetPoolStats() const
{
    FSOTS_FXPoolStats Stats;

    for (const TPair<FGameplayTag, FSOTS_FXTagPool>& Pair : TagPools)
    {
        for (const FSOTS_FXPoolEntry& Entry : Pair.Value.NiagaraEntries)
        {
            const UNiagaraComponent* Comp = Cast<UNiagaraComponent>(Entry.Component.Get());
            if (!Comp)
            {
                continue;
            }

            ++Stats.TotalPooledNiagara;
            if (Comp->IsActive())
            {
                ++Stats.ActiveNiagara;
            }
            else
            {
                ++Stats.FreeNiagara;
            }
        }

        for (const FSOTS_FXPoolEntry& Entry : Pair.Value.AudioEntries)
        {
            const UAudioComponent* Comp = Cast<UAudioComponent>(Entry.Component.Get());
            if (!Comp)
            {
                continue;
            }

            ++Stats.TotalPooledAudio;
            if (Comp->IsPlaying())
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

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
void USOTS_FXManagerSubsystem::DumpPoolStatsToLog() const
{
    if (!bDebugDumpPoolStats)
    {
        return;
    }

    FSOTS_FXPoolStats Stats = GetPoolStats();
    UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Pool Stats => Niagara Total=%d Active=%d Free=%d | Audio Total=%d Active=%d Free=%d"),
        Stats.TotalPooledNiagara, Stats.ActiveNiagara, Stats.FreeNiagara,
        Stats.TotalPooledAudio, Stats.ActiveAudio, Stats.FreeAudio);

    // Top cues by active counts (simple dump)
    for (const TPair<FGameplayTag, int32>& Pair : ActiveNiagaraCounts)
    {
        const int32 AudioActive = ActiveAudioCounts.Contains(Pair.Key) ? ActiveAudioCounts[Pair.Key] : 0;
        if (Pair.Value > 0 || AudioActive > 0)
        {
            UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX]   Tag %s => NiagaraActive=%d AudioActive=%d"), *Pair.Key.ToString(), Pair.Value, AudioActive);
        }
    }
    for (const TPair<FGameplayTag, int32>& Pair : ActiveAudioCounts)
    {
        if (!ActiveNiagaraCounts.Contains(Pair.Key) && Pair.Value > 0)
        {
            UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX]   Tag %s => NiagaraActive=0 AudioActive=%d"), *Pair.Key.ToString(), Pair.Value);
        }
    }
}
#endif

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
    UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] FX request failed (%s): Tag=%s Result=%s %s"), Reason, *TagStr, *StatusStr, *Debug);
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
            UE_LOG(LogSOTS_FX, Log, TEXT("[SOTS_FX] Resolved %s via %s (P=%d,Order=%d)."), *FXTag.ToString(), *LibName, Entry->Priority, Entry->RegistrationOrder);
        }
#endif
        OutDefinition = &Entry->Definition;
        return ESOTS_FXRequestResult::Success;
    }

    OutFailReason = FString::Printf(TEXT("Definition not found (searched %d libraries)"), LibraryCount);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogCueResolution)
    {
        UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Failed to resolve %s after searching %d libraries. Order: %s"), *FXTag.ToString(), LibraryCount, *BuildLibraryOrderDebugString());
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
    USceneComponent* AttachComponent = Request.AttachComponent;
    if (!AttachComponent && Request.SpawnSpace != ESOTS_FXSpawnSpace::World)
    {
        if (Request.Target && Request.Target->GetRootComponent())
        {
            AttachComponent = Request.Target->GetRootComponent();
        }
        else if (Request.Instigator && Request.Instigator->GetRootComponent())
        {
            AttachComponent = Request.Instigator->GetRootComponent();
        }
    }

    Params.AttachComponent = AttachComponent;
    Params.AttachSocketName = Request.AttachSocketName;
    Params.bHasSurfaceNormal = Request.bHasSurfaceNormal;
    Params.SurfaceNormal = Request.SurfaceNormal;
    Params.SpawnSpace = (Request.SpawnSpace != ESOTS_FXSpawnSpace::World && AttachComponent)
        ? Request.SpawnSpace
        : (Definition ? Definition->DefaultSpace : ESOTS_FXSpawnSpace::World);
    Params.bAttach = Params.SpawnSpace != ESOTS_FXSpawnSpace::World && Params.AttachComponent != nullptr;
    Params.Location = Request.Location;
    Params.Rotation = Request.Rotation;
    if (Params.bAttach && Params.AttachComponent)
    {
        const FTransform ParentXf = Params.AttachComponent->GetSocketTransform(Params.AttachSocketName, RTS_World);
        if (Params.Location.IsNearlyZero())
        {
            Params.Location = ParentXf.GetLocation();
        }

        if (Params.Rotation.IsNearlyZero())
        {
            Params.Rotation = ParentXf.Rotator();
        }
    }
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

void USOTS_FXManagerSubsystem::ApplyAudioTuning(UAudioComponent* AudioComp, const FSOTS_FXDefinition* Definition) const
{
    if (!AudioComp || !Definition)
    {
        return;
    }

    USoundAttenuation* Attenuation = Definition->SoundAttenuation.IsNull() ? nullptr : Definition->SoundAttenuation.LoadSynchronous();
    USoundConcurrency* Concurrency = Definition->SoundConcurrency.IsNull() ? nullptr : Definition->SoundConcurrency.LoadSynchronous();

    AudioComp->AttenuationSettings = Attenuation;
    AudioComp->ConcurrencySet.Reset();
    if (Concurrency)
    {
        AudioComp->ConcurrencySet.Add(Concurrency);
    }

    AudioComp->SetVolumeMultiplier(Definition->VolumeMultiplier);
    AudioComp->SetPitchMultiplier(Definition->PitchMultiplier);
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

    USceneComponent* AttachComponent = Params.AttachComponent;
    const bool bAttached = Params.bAttach && AttachComponent != nullptr && Params.SpawnSpace != ESOTS_FXSpawnSpace::World;

    FVector WorldLocation = Params.Location;
    FRotator WorldRotation = Params.Rotation;

    ApplySurfaceAlignment(Definition, Params, WorldLocation, WorldRotation);

    FVector SpawnLocation = WorldLocation;
    FRotator SpawnRotation = WorldRotation;
    const float SpawnScale = Params.Scale > 0.0f ? Params.Scale : 1.0f;
    const bool bShouldTriggerCameraShake = Definition && Definition->CameraShakeClass && Params.bAllowCameraShake;
    const bool bHasCameraShakeConfigured = Definition && Definition->CameraShakeClass;

    if (bAttached)
    {
        const FTransform ParentXf = AttachComponent->GetSocketTransform(Params.AttachSocketName, RTS_World);
        const FTransform WorldXf(WorldRotation.Quaternion(), WorldLocation, FVector::OneVector);
        const FTransform RelativeXf = WorldXf.GetRelativeTransform(ParentXf);
        SpawnLocation = RelativeXf.GetLocation();
        SpawnRotation = RelativeXf.Rotator();
        ApplyOffsets(Definition, true, SpawnLocation, SpawnRotation);

        const FTransform RelativeWithOffset(SpawnRotation.Quaternion(), SpawnLocation, FVector::OneVector);
        const FTransform FinalWorldXf = RelativeWithOffset * ParentXf;
        WorldLocation = FinalWorldXf.GetLocation();
        WorldRotation = FinalWorldXf.Rotator();
    }
    else
    {
        ApplyOffsets(Definition, false, SpawnLocation, SpawnRotation);
        WorldLocation = SpawnLocation;
        WorldRotation = SpawnRotation;
    }

    bool bNiagaraRejected = false;
    bool bAudioRejected = false;

    UNiagaraComponent* NiagaraComp = AcquireNiagaraComponent(Definition, Params, World, bNiagaraRejected);
    if (NiagaraComp)
    {
        NiagaraComp->SetHiddenInGame(false);
        if (bAttached && AttachComponent)
        {
            NiagaraComp->AttachToComponent(
                AttachComponent,
                FAttachmentTransformRules::KeepRelativeTransform,
                Params.AttachSocketName);
            NiagaraComp->SetRelativeLocationAndRotation(SpawnLocation, SpawnRotation);
            NiagaraComp->SetRelativeScale3D(FVector(SpawnScale));
        }
        else
        {
            NiagaraComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            NiagaraComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
            NiagaraComp->SetWorldScale3D(FVector(SpawnScale));
        }

        ApplyNiagaraParameters(NiagaraComp, Definition);
        NiagaraComp->Activate(true);
    }

    UAudioComponent* AudioComp = AcquireAudioComponent(Definition, Params, World, bAudioRejected);
    if (AudioComp)
    {
        ApplyAudioTuning(AudioComp, Definition);

        if (bAttached && AttachComponent)
        {
            AudioComp->AttachToComponent(
                AttachComponent,
                FAttachmentTransformRules::KeepRelativeTransform,
                Params.AttachSocketName);
            AudioComp->SetRelativeLocationAndRotation(SpawnLocation, SpawnRotation);
        }
        else
        {
            AudioComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            AudioComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
        }

        AudioComp->Play();
    }
    if (bShouldTriggerCameraShake)
    {
        TriggerCameraShake(Definition, World);
    }

    Report.SpawnedObject = NiagaraComp ? static_cast<UObject*>(NiagaraComp) : static_cast<UObject*>(AudioComp);

    const bool bAnyFXSpawned = (NiagaraComp || AudioComp);
    const bool bRejectedByPolicy = bNiagaraRejected || bAudioRejected;

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
        else if (!bAnyFXSpawned && bRejectedByPolicy)
        {
        Report.Result = ESOTS_FXRequestResult::DisabledByPolicy;
    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        const FString RejectNote = TEXT("MaxActivePerCue");
        Report.DebugMessage = Report.DebugMessage.IsEmpty() ? RejectNote : FString::Printf(TEXT("%s %s"), *Report.DebugMessage, *RejectNote);
    #endif
        }
        else
    {
        Report.Result = ESOTS_FXRequestResult::FailedToSpawn;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        Report.DebugMessage = TEXT("No FX assets spawned.");
        if (bLogFXRequestFailures)
        {
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Failed to spawn FX for %s (no assets)."), *Params.RequestedTag.ToString());
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

    Legacy = ConvertReportToLegacy(Report, OriginalRequest, WorldLocation, WorldRotation, SpawnScale, Definition);

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
            WorldLocation,
            WorldRotation,
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

FSOTS_FXTagPool& USOTS_FXManagerSubsystem::GetOrCreatePool(const FGameplayTag& CueTag)
{
    return TagPools.FindOrAdd(CueTag);
}

FSOTS_FXPoolEntry* USOTS_FXManagerSubsystem::FindFreeEntry(TArray<FSOTS_FXPoolEntry>& Entries) const
{
    for (FSOTS_FXPoolEntry& Entry : Entries)
    {
        if (!Entry.Component.IsValid())
        {
            continue;
        }

        if (!Entry.bActive)
        {
            return &Entry;
        }
    }

    return nullptr;
}

FSOTS_FXPoolEntry* USOTS_FXManagerSubsystem::ReclaimOldestActiveEntry(TArray<FSOTS_FXPoolEntry>& Entries, bool bDestroyOldest)
{
    FSOTS_FXPoolEntry* Oldest = nullptr;

    for (FSOTS_FXPoolEntry& Entry : Entries)
    {
        if (!Entry.Component.IsValid())
        {
            continue;
        }

        if (Entry.bActive && (!Oldest || Entry.LastUseTime < Oldest->LastUseTime))
        {
            Oldest = &Entry;
        }
    }

    if (!Oldest)
    {
        return nullptr;
    }

    if (bDestroyOldest)
    {
        if (UActorComponent* Comp = Oldest->Component.Get())
        {
            Comp->DestroyComponent();
        }
        Oldest->Component.Reset();
        Oldest->bActive = false;
        Oldest->LastUseTime = 0.0;
        return nullptr;
    }

    if (UActorComponent* Comp = Oldest->Component.Get())
    {
        if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(Comp))
        {
            NiagaraComp->DeactivateImmediate();
            NiagaraComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            NiagaraComp->SetHiddenInGame(true);
        }
        else if (UAudioComponent* AudioComp = Cast<UAudioComponent>(Comp))
        {
            AudioComp->Stop();
            AudioComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        }
    }

    Oldest->bActive = false;
    Oldest->LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    return Oldest;
}

UNiagaraComponent* USOTS_FXManagerSubsystem::SpawnFreshNiagaraComponent(UNiagaraSystem* NiagaraSystem, UWorld* World)
{
    if (!NiagaraSystem || !World)
    {
        return nullptr;
    }

    UNiagaraComponent* NewComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World,
        NiagaraSystem,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        FVector::OneVector,
        /*bAutoDestroy=*/false,
        /*bAutoActivate=*/false,
        ENCPoolMethod::None,
        /*bPreCullCheck=*/true);

    if (NewComp)
    {
        NewComp->SetAutoDestroy(false);
        NewComp->OnSystemFinished.RemoveDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
        NewComp->OnSystemFinished.AddUniqueDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
    }

    return NewComp;
}

UAudioComponent* USOTS_FXManagerSubsystem::SpawnFreshAudioComponent(USoundBase* Sound, UWorld* World)
{
    if (!Sound || !World)
    {
        return nullptr;
    }

    UAudioComponent* NewComp = UGameplayStatics::SpawnSoundAtLocation(
        World,
        Sound,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        1.0f,
        1.0f,
        0.0f,
        nullptr,
        nullptr,
        /*bAutoDestroy=*/false);

    if (NewComp)
    {
        NewComp->bAutoDestroy = false;
        NewComp->OnAudioFinishedNative.RemoveAll(this);
        NewComp->OnAudioFinishedNative.AddUObject(this, &USOTS_FXManagerSubsystem::HandleAudioFinishedNative);
    }

    return NewComp;
}

void USOTS_FXManagerSubsystem::PruneExcessPoolEntries()
{
    int32 NewTotalNiagara = 0;
    int32 NewTotalAudio = 0;

    auto Cull = [&](TArray<FSOTS_FXPoolEntry>& Entries, int32 MaxAllowed, int32& NewTotal)
    {
        // Remove invalid entries outright.
        Entries.RemoveAll([](FSOTS_FXPoolEntry& Entry)
        {
            if (!Entry.Component.IsValid())
            {
                return true;
            }
            return false;
        });

        // Count valid entries.
        for (const FSOTS_FXPoolEntry& Entry : Entries)
        {
            if (Entry.Component.IsValid())
            {
                ++NewTotal;
            }
        }

        if (MaxAllowed < 0)
        {
            MaxAllowed = 0;
        }

        int32 Excess = NewTotal - MaxAllowed;
        if (Excess <= 0)
        {
            return;
        }

        // Remove inactive entries first until within limit.
        Entries.RemoveAll([&](FSOTS_FXPoolEntry& Entry)
        {
            if (Excess <= 0)
            {
                return false;
            }

            if (!Entry.bActive && Entry.Component.IsValid())
            {
                if (UActorComponent* Comp = Entry.Component.Get())
                {
                    Comp->DestroyComponent();
                }
                --Excess;
                --NewTotal;
                return true;
            }
            return false;
        });
    };

    for (TPair<FGameplayTag, FSOTS_FXTagPool>& Pair : TagPools)
    {
        Cull(Pair.Value.NiagaraEntries, MaxPooledNiagaraComponents, NewTotalNiagara);
        Cull(Pair.Value.AudioEntries, MaxPooledAudioComponents, NewTotalAudio);
    }

    TotalPooledNiagara = NewTotalNiagara;
    TotalPooledAudio = NewTotalAudio;
}

UNiagaraComponent* USOTS_FXManagerSubsystem::AcquireNiagaraComponent(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, UWorld* World, bool& bOutRejectedByPolicy)
{
    bOutRejectedByPolicy = false;

    if (!Definition || !World)
    {
        return nullptr;
    }

    UNiagaraSystem* NiagaraSystem = Definition->NiagaraSystem.LoadSynchronous();
    if (!NiagaraSystem)
    {
        return nullptr;
    }

    if (!bEnablePooling)
    {
        UNiagaraComponent* Temp = SpawnFreshNiagaraComponent(NiagaraSystem, World);
        return Temp;
    }

    FSOTS_FXTagPool& Pool = GetOrCreatePool(Params.ResolvedTag);
    const float NowSeconds = World->GetTimeSeconds();
    int32& ActiveCount = ActiveNiagaraCounts.FindOrAdd(Params.ResolvedTag);

    if (ActiveCount >= MaxActivePerCue)
    {
        switch (OverflowPolicy)
        {
            case ESOTS_FXPoolOverflowPolicy::RejectNew:
                bOutRejectedByPolicy = true;
                LogPoolOverflow(Params.ResolvedTag, TEXT("Niagara"), FString::Printf(TEXT("Active limit (%d/%d) reached"), ActiveCount, MaxActivePerCue));
                return nullptr;
            case ESOTS_FXPoolOverflowPolicy::DestroyOldest:
                ReclaimOldestActiveEntry(Pool.NiagaraEntries, true);
                ActiveCount = FMath::Max(0, ActiveCount - 1);
                break;
            case ESOTS_FXPoolOverflowPolicy::ReuseOldest:
            default:
                ReclaimOldestActiveEntry(Pool.NiagaraEntries, false);
                ActiveCount = FMath::Max(0, ActiveCount - 1);
                break;
        }
    }

    if (FSOTS_FXPoolEntry* Free = FindFreeEntry(Pool.NiagaraEntries))
    {
        UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(Free->Component.Get());
        if (NiagaraComp)
        {
            NiagaraComp->SetAutoDestroy(false);
            NiagaraComp->OnSystemFinished.RemoveDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
            NiagaraComp->OnSystemFinished.AddUniqueDynamic(this, &USOTS_FXManagerSubsystem::HandleNiagaraFinished);
            NiagaraComp->SetAsset(NiagaraSystem);
            NiagaraComp->SetHiddenInGame(true);

            Free->bActive = true;
            Free->LastUseTime = NowSeconds;
            ++ActiveCount;
            NiagaraComponentTags.FindOrAdd(NiagaraComp) = Params.ResolvedTag;
            return NiagaraComp;
        }
    }

    if (TotalPooledNiagara >= MaxPooledNiagaraComponents)
    {
        bOutRejectedByPolicy = true;
        LogPoolOverflow(Params.ResolvedTag, TEXT("Niagara"), FString::Printf(TEXT("Total pooled limit (%d/%d) reached"), TotalPooledNiagara, MaxPooledNiagaraComponents));
        return nullptr;
    }

    if (UNiagaraComponent* NewComp = SpawnFreshNiagaraComponent(NiagaraSystem, World))
    {
        FSOTS_FXPoolEntry& Entry = Pool.NiagaraEntries.AddDefaulted_GetRef();
        Entry.Component = NewComp;
        Entry.bActive = true;
        Entry.LastUseTime = NowSeconds;
        ++TotalPooledNiagara;
        ++ActiveCount;
        NiagaraComponentTags.FindOrAdd(NewComp) = Params.ResolvedTag;
        return NewComp;
    }

    return nullptr;
}

void USOTS_FXManagerSubsystem::ReleaseNiagaraComponent(UNiagaraComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FGameplayTag* FoundTag = NiagaraComponentTags.Find(Component);
    if (!FoundTag)
    {
        return;
    }

    FSOTS_FXTagPool& Pool = GetOrCreatePool(*FoundTag);
    int32& ActiveCount = ActiveNiagaraCounts.FindOrAdd(*FoundTag);

    for (FSOTS_FXPoolEntry& Entry : Pool.NiagaraEntries)
    {
        if (Entry.Component == Component)
        {
            Component->DeactivateImmediate();
            Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            Component->SetHiddenInGame(true);
            Entry.bActive = false;
            Entry.LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            ActiveCount = FMath::Max(0, ActiveCount - 1);
            break;
        }
    }

    PruneExcessPoolEntries();
}

UAudioComponent* USOTS_FXManagerSubsystem::AcquireAudioComponent(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, UWorld* World, bool& bOutRejectedByPolicy)
{
    bOutRejectedByPolicy = false;

    if (!Definition || !World)
    {
        return nullptr;
    }

    USoundBase* Sound = Definition->Sound.LoadSynchronous();
    if (!Sound)
    {
        return nullptr;
    }

    if (!bEnablePooling)
    {
        return SpawnFreshAudioComponent(Sound, World);
    }

    FSOTS_FXTagPool& Pool = GetOrCreatePool(Params.ResolvedTag);
    const float NowSeconds = World->GetTimeSeconds();
    int32& ActiveCount = ActiveAudioCounts.FindOrAdd(Params.ResolvedTag);

    if (ActiveCount >= MaxActivePerCue)
    {
        switch (OverflowPolicy)
        {
            case ESOTS_FXPoolOverflowPolicy::RejectNew:
                bOutRejectedByPolicy = true;
                LogPoolOverflow(Params.ResolvedTag, TEXT("Audio"), FString::Printf(TEXT("Active limit (%d/%d) reached"), ActiveCount, MaxActivePerCue));
                return nullptr;
            case ESOTS_FXPoolOverflowPolicy::DestroyOldest:
                ReclaimOldestActiveEntry(Pool.AudioEntries, true);
                ActiveCount = FMath::Max(0, ActiveCount - 1);
                break;
            case ESOTS_FXPoolOverflowPolicy::ReuseOldest:
            default:
                ReclaimOldestActiveEntry(Pool.AudioEntries, false);
                ActiveCount = FMath::Max(0, ActiveCount - 1);
                break;
        }
    }

    if (FSOTS_FXPoolEntry* Free = FindFreeEntry(Pool.AudioEntries))
    {
        if (UAudioComponent* AudioComp = Cast<UAudioComponent>(Free->Component.Get()))
        {
            AudioComp->Stop();
            AudioComp->SetSound(Sound);
            AudioComp->bAutoDestroy = false;
            AudioComp->OnAudioFinishedNative.RemoveAll(this);
            AudioComp->OnAudioFinishedNative.AddUObject(this, &USOTS_FXManagerSubsystem::HandleAudioFinishedNative);

            Free->bActive = true;
            Free->LastUseTime = NowSeconds;
            ++ActiveCount;
            AudioComponentTags.FindOrAdd(AudioComp) = Params.ResolvedTag;
            return AudioComp;
        }
    }

    if (TotalPooledAudio >= MaxPooledAudioComponents)
    {
        bOutRejectedByPolicy = true;
        LogPoolOverflow(Params.ResolvedTag, TEXT("Audio"), FString::Printf(TEXT("Total pooled limit (%d/%d) reached"), TotalPooledAudio, MaxPooledAudioComponents));
        return nullptr;
    }

    if (UAudioComponent* NewComp = SpawnFreshAudioComponent(Sound, World))
    {
        FSOTS_FXPoolEntry& Entry = Pool.AudioEntries.AddDefaulted_GetRef();
        Entry.Component = NewComp;
        Entry.bActive = true;
        Entry.LastUseTime = NowSeconds;
        ++TotalPooledAudio;
        ++ActiveCount;
        AudioComponentTags.FindOrAdd(NewComp) = Params.ResolvedTag;
        return NewComp;
    }

    return nullptr;
}

void USOTS_FXManagerSubsystem::ReleaseAudioComponent(UAudioComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FGameplayTag* FoundTag = AudioComponentTags.Find(Component);
    if (!FoundTag)
    {
        return;
    }

    FSOTS_FXTagPool& Pool = GetOrCreatePool(*FoundTag);
    int32& ActiveCount = ActiveAudioCounts.FindOrAdd(*FoundTag);

    for (FSOTS_FXPoolEntry& Entry : Pool.AudioEntries)
    {
        if (Entry.Component == Component)
        {
            Component->Stop();
            Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            Entry.bActive = false;
            Entry.LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            ActiveCount = FMath::Max(0, ActiveCount - 1);
            break;
        }
    }

    PruneExcessPoolEntries();
}

void USOTS_FXManagerSubsystem::LogPoolEvent(const FString& Message) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogPoolActions)
    {
        UE_LOG(LogSOTS_FX, Log, TEXT("%s"), *Message);
    }
#else
    (void)Message;
#endif
}

void USOTS_FXManagerSubsystem::LogPoolOverflow(const FGameplayTag& CueTag, const TCHAR* ComponentType, const FString& Detail) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const FString TagStr = CueTag.IsValid() ? CueTag.ToString() : TEXT("<none>");
    UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] %s pool overflow for %s: %s"), ComponentType, *TagStr, *Detail);
#else
    (void)CueTag;
    (void)ComponentType;
    (void)Detail;
#endif
}

void USOTS_FXManagerSubsystem::HandleNiagaraFinished(UNiagaraComponent* FinishedComponent)
{
    ReleaseNiagaraComponent(FinishedComponent);
    LogPoolEvent(TEXT("[SOTS_FX] Niagara component finished and returned to pool."));
}

void USOTS_FXManagerSubsystem::HandleAudioFinished()
{
    // Dynamic delegate no longer provides the component; prefer native binding.
}

void USOTS_FXManagerSubsystem::HandleAudioFinishedNative(UAudioComponent* FinishedComponent)
{
    ReleaseAudioComponent(FinishedComponent);
    LogPoolEvent(TEXT("[SOTS_FX] Audio component finished and returned to pool."));
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

    // ---- Build execution params from legacy context ----
    FSOTS_FXExecutionParams ExecParams;
    ExecParams.WorldContextObject = this;
    ExecParams.RequestedTag = CueDefinition->CueTag;
    ExecParams.ResolvedTag = CueDefinition->CueTag;
    ExecParams.Location = Context.Location;
    ExecParams.Rotation = Context.Rotation;
    ExecParams.Scale = Context.Scale;
    ExecParams.SpawnSpace = Context.bAttach ? ESOTS_FXSpawnSpace::AttachToComponent : ESOTS_FXSpawnSpace::World;
    ExecParams.AttachComponent = Context.AttachComponent;
    ExecParams.AttachSocketName = Context.AttachSocketName;
    ExecParams.bAttach = Context.bAttach;
    ExecParams.Instigator = Context.Instigator;
    ExecParams.Target = Context.Target;

    // Build a minimal definition view so pooled helpers can reuse the unified execution path.
    FSOTS_FXDefinition DefinitionFromCue;
    DefinitionFromCue.FXTag = CueDefinition->CueTag;
    DefinitionFromCue.NiagaraSystem = CueDefinition->NiagaraSystem;
    DefinitionFromCue.Sound = CueDefinition->Sound;
    DefinitionFromCue.DefaultSpace = ExecParams.SpawnSpace;
    DefinitionFromCue.DefaultScale = ExecParams.Scale;
    DefinitionFromCue.CameraShakeClass = CueDefinition->CameraShakeClass;
    DefinitionFromCue.CameraShakeScale = CueDefinition->CameraShakeScale;
    DefinitionFromCue.ToggleBehavior = CueDefinition->ToggleBehavior;
    DefinitionFromCue.bIsBloodFX = CueDefinition->bIsBloodFX;
    DefinitionFromCue.bIsHighIntensityFX = CueDefinition->bIsHighIntensityFX;
    DefinitionFromCue.bCameraShakeIgnoresGlobalToggle = CueDefinition->bCameraShakeIgnoresGlobalToggle;
    const FSOTS_FXDefinition* Definition = &DefinitionFromCue;

    // ---- VFX (Niagara, pooled) ----
    if (CueDefinition->NiagaraSystem)
    {
        bool bRejected = false;
        UNiagaraComponent* NiagaraComp = AcquireNiagaraComponent(Definition, ExecParams, World, bRejected);
        if (NiagaraComp)
        {
            FVector SpawnLocation = ExecParams.Location;
            FRotator SpawnRotation = ExecParams.Rotation;
            const bool bAttached = ExecParams.SpawnSpace != ESOTS_FXSpawnSpace::World && ExecParams.AttachComponent;
            ApplySurfaceAlignment(Definition, ExecParams, SpawnLocation, SpawnRotation);
            ApplyOffsets(Definition, bAttached, SpawnLocation, SpawnRotation);

            if (bAttached)
            {
                NiagaraComp->AttachToComponent(ExecParams.AttachComponent, FAttachmentTransformRules::KeepRelativeTransform, ExecParams.AttachSocketName);
                NiagaraComp->SetRelativeLocationAndRotation(SpawnLocation, SpawnRotation);
            }
            else
            {
                NiagaraComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
            }

            NiagaraComp->SetWorldScale3D(FVector(ExecParams.Scale));
            ApplyNiagaraParameters(NiagaraComp, Definition);
            NiagaraComp->Activate(true);
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
        bool bRejected = false;
        UAudioComponent* AudioComp = AcquireAudioComponent(Definition, ExecParams, World, bRejected);
        if (AudioComp)
        {
            const bool bAttached = ExecParams.SpawnSpace != ESOTS_FXSpawnSpace::World && ExecParams.AttachComponent;

            if (bAttached)
            {
                AudioComp->AttachToComponent(ExecParams.AttachComponent, FAttachmentTransformRules::KeepRelativeTransform, ExecParams.AttachSocketName);
                AudioComp->SetRelativeLocationAndRotation(ExecParams.Location, ExecParams.Rotation);
            }
            else
            {
                AudioComp->SetWorldLocationAndRotation(ExecParams.Location, ExecParams.Rotation);
            }

            AudioComp->SetVolumeMultiplier(Definition->VolumeMultiplier);
            AudioComp->SetPitchMultiplier(Definition->PitchMultiplier);
            ApplyAudioTuning(AudioComp, Definition);
            AudioComp->Play();
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Duplicate FX tag %s detected during validation; first definition wins."), *Def.FXTag.ToString());
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
        UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] FX tag %s has no Niagara or Sound asset set."), *Def.FXTag.ToString());
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
            UE_LOG(LogSOTS_FX, Warning, TEXT("[SOTS_FX] Validation skipped null library reference."));
        }
    }
#endif
}
