#include "SOTS_FXManagerSubsystem.h"

#include "SOTS_FXCueDefinition.h"
#include "SOTS_FXDefinitionLibrary.h"

#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
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
    RegisteredLibraryDefinitions.Reset();
    bRegistryReady = false;

    BuildRegistryFromLibraries();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bValidateFXRegistryOnInit)
    {
        ValidateLibraryDefinitions();
    }
#endif

    bRegistryReady = true;
}

void USOTS_FXManagerSubsystem::Deinitialize()
{
    RegisteredLibraryDefinitions.Reset();
    bRegistryReady = false;
    CuePools.Reset();
    CueMap.Reset();

    SingletonInstance = nullptr;

    Super::Deinitialize();
}

USOTS_FXManagerSubsystem* USOTS_FXManagerSubsystem::Get()
{
    return SingletonInstance.Get();
}

void USOTS_FXManagerSubsystem::BuildRegistryFromLibraries()
{
    RegisteredLibraryDefinitions.Reset();

    for (const TObjectPtr<USOTS_FXDefinitionLibrary>& Library : Libraries)
    {
        if (!Library)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bValidateFXRegistryOnInit && bWarnOnMissingFXAssets)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Library reference is null; skipping."));
            }
#endif
            continue;
        }

        for (const FSOTS_FXDefinition& Def : Library->Definitions)
        {
            if (!Def.FXTag.IsValid())
            {
                continue;
            }

            if (RegisteredLibraryDefinitions.Contains(Def.FXTag))
            {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
                if (bWarnOnDuplicateFXTags)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Duplicate FX tag %s; first definition kept."), *Def.FXTag.ToString());
                }
#endif
                continue;
            }

            RegisteredLibraryDefinitions.Add(Def.FXTag, Def);
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

        for (UNiagaraComponent* NiagaraComp : Pool.NiagaraComponents)
        {
            if (NiagaraComp && NiagaraComp->IsActive())
            {
                ++OutActiveNiagara;
            }
        }

        for (UAudioComponent* AudioComp : Pool.AudioComponents)
        {
            if (AudioComp && AudioComp->IsPlaying())
            {
                ++OutActiveAudio;
            }
        }
    }
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

void USOTS_FXManagerSubsystem::RequestFXCue(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target)
{
    (void)RequestFXCueWithReport(FXCueTag, Instigator, Target);
}

FSOTS_FXRequestReport USOTS_FXManagerSubsystem::RequestFXCueWithReport(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target)
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
    const ESOTS_FXRequestResult PolicyResult = EvaluatePolicy(Definition, PolicyFail);
    if (PolicyResult != ESOTS_FXRequestResult::Success)
    {
        SetFailure(PolicyResult, ESOTS_FXRequestStatus::MissingContext, PolicyFail);
        if (OutLegacyResult)
        {
            *OutLegacyResult = Legacy;
        }
        return Report;
    }

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

    FVector ResolvedLocation = Request.Location;
    FRotator ResolvedRotation = Request.Rotation;

    if (Request.SpawnSpace != ESOTS_FXSpawnSpace::World && Request.AttachComponent)
    {
        const FTransform Xf = Request.AttachComponent->GetComponentTransform();
        ResolvedLocation = Xf.GetLocation();
        ResolvedRotation = Xf.Rotator();
    }

    float EffectiveScale = Request.Scale;
    if (EffectiveScale <= 0.0f)
    {
        EffectiveScale = 1.0f;
    }
    EffectiveScale *= Definition ? Definition->DefaultScale : 1.0f;

    Report.Result = ESOTS_FXRequestResult::Success;
    Legacy = ConvertReportToLegacy(Report, Request, ResolvedLocation, ResolvedRotation, EffectiveScale, Definition);

    BroadcastResolvedFX(
        Report.ResolvedCueTag,
        Request.Instigator,
        Request.Target,
        Definition,
        ResolvedLocation,
        ResolvedRotation,
        Request.SpawnSpace,
        Request.AttachComponent,
        Request.AttachSocketName,
        EffectiveScale);

    if (OutLegacyResult)
    {
        *OutLegacyResult = Legacy;
    }

    return Report;
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

ESOTS_FXRequestResult USOTS_FXManagerSubsystem::EvaluatePolicy(const FSOTS_FXDefinition* Definition, FString& OutFailReason) const
{
    OutFailReason.Reset();

    if (!Definition)
    {
        OutFailReason = TEXT("Definition missing");
        return ESOTS_FXRequestResult::NotFound;
    }

    // No per-cue policy metadata yet; global toggles are currently pass-through.
    return ESOTS_FXRequestResult::Success;
}

ESOTS_FXRequestResult USOTS_FXManagerSubsystem::TryResolveCue(FGameplayTag FXTag, const FSOTS_FXDefinition*& OutDefinition, FGameplayTag& OutResolvedTag, FString& OutFailReason) const
{
    OutDefinition = nullptr;
    OutResolvedTag = FXTag;
    OutFailReason.Reset();

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

    if (const FSOTS_FXDefinition* Def = RegisteredLibraryDefinitions.Find(FXTag))
    {
        OutDefinition = Def;
        return ESOTS_FXRequestResult::Success;
    }

    OutFailReason = TEXT("Definition not found");
    return ESOTS_FXRequestResult::NotFound;
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

    // Reuse an inactive component if possible
    for (TObjectPtr<UNiagaraComponent>& Comp : Pool.NiagaraComponents)
    {
        if (Comp && !Comp->IsActive())
        {
            return Comp;
        }
    }

    // No free one – spawn a new component configured for pooling
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
        Pool.NiagaraComponents.Add(NewComp);
    }

    return NewComp;
}

UAudioComponent* USOTS_FXManagerSubsystem::AcquireAudioComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition)
{
    if (!World || !CueDefinition || !CueDefinition->Sound)
    {
        return nullptr;
    }

    FSOTS_FXCuePool& Pool = CuePools.FindOrAdd(CueDefinition);

    // Reuse a non-playing component
    for (TObjectPtr<UAudioComponent>& Comp : Pool.AudioComponents)
    {
        if (Comp && !Comp->IsPlaying())
        {
            return Comp;
        }
    }

    // No free one – spawn a pooled audio component.
    UAudioComponent* NewComp = UGameplayStatics::SpawnSoundAtLocation(
        World,
        CueDefinition->Sound,
        FVector::ZeroVector,   // Location
        FRotator::ZeroRotator, // Rotation
        1.0f,                  // VolumeMultiplier
        1.0f,                  // PitchMultiplier
        0.0f,                  // StartTime
        nullptr,               // Attenuation
        nullptr,               // Concurrency
        false                  // bAutoDestroy (we pool)
    );

    if (NewComp)
    {
        NewComp->bAutoDestroy = false;
        Pool.AudioComponents.Add(NewComp);
    }

    return NewComp;
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
    if (CueDefinition->CameraShakeClass)
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

    for (const TPair<FGameplayTag, FSOTS_FXDefinition>& Pair : RegisteredLibraryDefinitions)
    {
        ValidateCueDefinition(Pair.Value, SeenTags);
    }

    for (const TObjectPtr<USOTS_FXDefinitionLibrary>& Library : Libraries)
    {
        if (!Library && bWarnOnMissingFXAssets)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOTS_FX] Validation skipped null library reference."));
        }
    }
#endif
}
