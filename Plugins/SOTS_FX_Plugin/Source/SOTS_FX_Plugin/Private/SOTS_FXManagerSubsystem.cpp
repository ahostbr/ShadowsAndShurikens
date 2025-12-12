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
}

void USOTS_FXManagerSubsystem::Deinitialize()
{
    CuePools.Reset();
    CueMap.Reset();

    SingletonInstance = nullptr;

    Super::Deinitialize();
}

USOTS_FXManagerSubsystem* USOTS_FXManagerSubsystem::Get()
{
    return SingletonInstance.Get();
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
    if (!World)
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
    if (!World || !CueDefinition)
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
    if (!FXCueTag.IsValid())
    {
        return;
    }

    UWorld* WorldContext = nullptr;
    if (Instigator)
    {
        WorldContext = Instigator->GetWorld();
    }
    else if (Target)
    {
        WorldContext = Target->GetWorld();
    }
    else
    {
        WorldContext = GetWorld();
    }

    FVector Location = FVector::ZeroVector;
    if (Target)
    {
        Location = Target->GetActorLocation();
    }
    else if (Instigator)
    {
        Location = Instigator->GetActorLocation();
    }

    TriggerFXByTag(WorldContext, FXCueTag, Instigator, Target, Location, FRotator::ZeroRotator);
}

// -------------------------
// Tag-driven FX job router
// -------------------------

const FSOTS_FXDefinition* USOTS_FXManagerSubsystem::FindDefinition(FGameplayTag FXTag) const
{
    if (!FXTag.IsValid())
    {
        return nullptr;
    }

    for (const TObjectPtr<USOTS_FXDefinitionLibrary>& Library : Libraries)
    {
        if (!Library)
        {
            continue;
        }

        for (const FSOTS_FXDefinition& Def : Library->Definitions)
        {
            if (Def.FXTag == FXTag)
            {
                return &Def;
            }
        }
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
    FName AttachSocketName)
{
    if (!FXTag.IsValid())
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
        Resolved.Scale         = Definition->DefaultScale;
    }

    OnFXTriggered.Broadcast(Resolved);
}

void USOTS_FXManagerSubsystem::TriggerFXByTag(
    UObject* WorldContextObject,
    FGameplayTag FXTag,
    AActor* Instigator,
    AActor* Target,
    FVector Location,
    FRotator Rotation)
{
    // WorldContextObject is accepted for consistency with other APIs, but
    // the current implementation only uses tag resolution and delegates.
    (void)WorldContextObject;

    const FSOTS_FXDefinition* Definition = FindDefinition(FXTag);
    BroadcastResolvedFX(
        FXTag,
        Instigator,
        Target,
        Definition,
        Location,
        Rotation,
        ESOTS_FXSpawnSpace::World,
        nullptr,
        NAME_None);
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

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    if (AttachComponent)
    {
        const FTransform Xf = AttachComponent->GetComponentTransform();
        Location = Xf.GetLocation();
        Rotation = Xf.Rotator();
    }

    const FSOTS_FXDefinition* Definition = FindDefinition(FXTag);
    BroadcastResolvedFX(
        FXTag,
        Instigator,
        Target,
        Definition,
        Location,
        Rotation,
        ESOTS_FXSpawnSpace::AttachToComponent,
        AttachComponent,
        AttachSocketName);
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
