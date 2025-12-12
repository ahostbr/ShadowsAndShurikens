#include "SOTS_MMSSSubsystem.h"

#include "SOTS_MMSSLog.h"
#include "SOTS_MMSS_MissionMusicLibrary.h"
#include "SOTS_ProfileTypes.h"

#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogSOTSMusicManager);

void USOTS_MMSSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentMissionId = NAME_None;
    CurrentTrackId = FGameplayTag();
    PersistentAudioComponent = nullptr;
    FadingOutComponent = nullptr;
    LastPlaybackTimeSeconds = 0.0f;
    bWasPlayingBeforeWorldChange = false;
    CachedWorld = nullptr;
    CurrentStreamableHandle.Reset();

    UE_LOG(LogSOTSMusicManager, Log, TEXT("[MusicManager] Initialized."));
}

void USOTS_MMSSSubsystem::Deinitialize()
{
    // Fade out any active music quickly on shutdown and reset state.
    if (PersistentAudioComponent)
    {
        PersistentAudioComponent->Stop();
    }

    Library = nullptr;
    CurrentStreamableHandle.Reset();
    PersistentAudioComponent = nullptr;
    FadingOutComponent = nullptr;
    CachedWorld = nullptr;
    LastPlaybackTimeSeconds = 0.0f;
    bWasPlayingBeforeWorldChange = false;
    LastFadeOutTimeFromTrack = 0.0f;
    bHasLastFadeOutTime = false;

    UE_LOG(LogSOTSMusicManager, Log, TEXT("[MusicManager] Deinitialized."));

    Super::Deinitialize();
}

void USOTS_MMSSSubsystem::SetMusicLibrary(USOTS_MissionMusicLibrary* InLibrary)
{
    Library = InLibrary;

    if (DebugMode != ESOTS_MusicDebugMode::Off)
    {
        UE_LOG(LogSOTSMusicManager, Log,
               TEXT("[MusicManager] Library set to %s"),
               Library ? *Library->GetName() : TEXT("<null>"));
    }
}

void USOTS_MMSSSubsystem::SetCurrentMissionId(FName NewMissionId)
{
    CurrentMissionId = NewMissionId;

    if (DebugMode != ESOTS_MusicDebugMode::Off)
    {
        UE_LOG(LogSOTSMusicManager, Log,
               TEXT("[MusicManager] Current mission set to '%s'"),
               *CurrentMissionId.ToString());
    }
}

void USOTS_MMSSSubsystem::SetDebugMode(ESOTS_MusicDebugMode NewMode)
{
    DebugMode = NewMode;

    UE_LOG(LogSOTSMusicManager, Log,
           TEXT("[MusicManager] Debug mode set to %d"),
           static_cast<int32>(DebugMode));
}

void USOTS_MMSSSubsystem::RequestMusicByTag(
    const UObject* WorldContextObject,
    const FGameplayTag& TrackId,
    bool bForceRestart,
    float OverrideFadeOut,
    float OverrideFadeIn)
{
    CurrentTrackId = TrackId;
    CurrentMusicRoleTag = TrackId;
    CurrentTrackIdName = TrackId.IsValid() ? TrackId.GetTagName() : NAME_None;
    bIsPlaying = TrackId.IsValid();
    InternalRequestTrack(CurrentMissionId, TrackId, bForceRestart, OverrideFadeOut, OverrideFadeIn);
}

void USOTS_MMSSSubsystem::RequestMusicByMissionAndTag(
    FName MissionId,
    const FGameplayTag& TrackId,
    bool bForceRestart,
    float OverrideFadeOut,
    float OverrideFadeIn)
{
    CurrentTrackId = TrackId;
    CurrentMusicRoleTag = TrackId;
    CurrentTrackIdName = TrackId.IsValid() ? TrackId.GetTagName() : NAME_None;
    CurrentMissionId = MissionId;
    bIsPlaying = TrackId.IsValid();
    InternalRequestTrack(MissionId, TrackId, bForceRestart, OverrideFadeOut, OverrideFadeIn);
}

void USOTS_MMSSSubsystem::RequestRoleTrack(FGameplayTag RoleTag, FName TrackIdName, float StartTime)
{
    CurrentMusicRoleTag = RoleTag;
    CurrentTrackIdName = TrackIdName.IsNone() && RoleTag.IsValid() ? RoleTag.GetTagName() : TrackIdName;
    LastPlaybackTimeSeconds = FMath::Max(StartTime, 0.0f);
    bIsPlaying = RoleTag.IsValid();
    RequestMusicByTag(nullptr, RoleTag, true, -1.0f, -1.0f);
}

void USOTS_MMSSSubsystem::BuildProfileData(FSOTS_MMSSProfileData& OutData) const
{
    OutData.CurrentMusicRoleTag = CurrentMusicRoleTag;
    OutData.CurrentTrackId = CurrentTrackIdName;
    OutData.PlaybackPositionSeconds = LastPlaybackTimeSeconds;
}

void USOTS_MMSSSubsystem::ApplyProfileData(const FSOTS_MMSSProfileData& InData)
{
    CurrentMusicRoleTag = InData.CurrentMusicRoleTag;
    CurrentTrackIdName = InData.CurrentTrackId;
    LastPlaybackTimeSeconds = FMath::Max(InData.PlaybackPositionSeconds, 0.0f);
    bIsPlaying = CurrentMusicRoleTag.IsValid();
    if (CurrentMusicRoleTag.IsValid())
    {
        RequestMusicByTag(nullptr, CurrentMusicRoleTag, true, -1.0f, -1.0f);
    }
}

void USOTS_MMSSSubsystem::StopMusic(float FadeOutTime)
{
    if (PersistentAudioComponent)
    {
        PersistentAudioComponent->FadeOut(FMath::Max(0.0f, FadeOutTime), 0.0f);
    }

    LastPlaybackTimeSeconds = 0.0f;
    bWasPlayingBeforeWorldChange = false;
    CurrentTrackId = FGameplayTag();
    CurrentMusicRoleTag = FGameplayTag();
    CurrentTrackIdName = NAME_None;
    bIsPlaying = false;
}

void USOTS_MMSSSubsystem::DebugPrintCurrentState() const
{
    UE_LOG(LogSOTSMusicManager, Log,
           TEXT("[MusicManager] Mission='%s' Track='%s' ActiveComponent=%s"),
           *CurrentMissionId.ToString(),
           CurrentTrackId.IsValid() ? *CurrentTrackId.ToString() : TEXT("<None>"),
           PersistentAudioComponent ? TEXT("Yes") : TEXT("No"));
}

void USOTS_MMSSSubsystem::InternalRequestTrack(
    FName MissionId,
    const FGameplayTag& TrackId,
    bool bForceRestart,
    float OverrideFadeOut,
    float OverrideFadeIn)
{
    if (!Library)
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] InternalRequestTrack called with no MusicLibrary set."));
        return;
    }

    FSOTS_MusicTrackEntry TrackData;
    bool bFoundTrack = false;

    // This reflects the actual gameplay tag used for the track,
    // whether it comes from a mission-specific set or the default set.
    FGameplayTag EffectiveTrackId = TrackId;

    // Mission-specific lookup (existing behavior) when a mission id is set.
    if (!MissionId.IsNone())
    {
        if (!TrackId.IsValid())
        {
            UE_LOG(LogSOTSMusicManager, Warning,
                   TEXT("[MusicManager] InternalRequestTrack called with invalid TrackId for Mission='%s'."),
                   *MissionId.ToString());
            return;
        }

        if (Library->GetTrack(MissionId, TrackId, TrackData))
        {
            bFoundTrack = true;
        }
    }
    else
    {
        // No mission id: use the default music set and always pick the first
        // track entry, ignoring the requested TrackId.
        const TMap<FGameplayTag, FSOTS_MusicTrackEntry>& DefaultTracks = Library->DefaultMusicSet.Tracks;
        if (DefaultTracks.Num() == 0)
        {
            UE_LOG(LogSOTSMusicManager, Warning,
                   TEXT("[MusicManager] No MissionId set and DefaultMusicSet has no tracks."));
            return;
        }

        auto It = DefaultTracks.CreateConstIterator();
        if (It)
        {
            EffectiveTrackId = It->Key;
            TrackData = It->Value;
            bFoundTrack = true;

            if (DebugMode != ESOTS_MusicDebugMode::Off)
            {
                UE_LOG(LogSOTSMusicManager, Log,
                       TEXT("[MusicManager] Using DefaultMusicSet (no MissionId). Track=%s"),
                       *EffectiveTrackId.ToString());
            }
        }
    }

    if (!bFoundTrack)
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] No track found for Mission='%s' Track='%s' (and DefaultMusicSet fallback failed)."),
               *MissionId.ToString(),
               *TrackId.ToString());
        return;
    }

    // De-duplicate against the currently active track using the effective tag.
    if (!bForceRestart &&
        MissionId == CurrentMissionId &&
        EffectiveTrackId == CurrentTrackId)
    {
        if (DebugMode == ESOTS_MusicDebugMode::Verbose)
        {
            UE_LOG(LogSOTSMusicManager, Log,
                   TEXT("[MusicManager] Requested track is already playing (Mission=%s, Track=%s)."),
                   *MissionId.ToString(),
                   *EffectiveTrackId.ToString());
        }
        return;
    }

    if (!TrackData.Sound.IsValid() && TrackData.Sound.ToSoftObjectPath().IsNull())
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] Track '%s' for Mission='%s' has no sound asset set."),
               *EffectiveTrackId.ToString(),
               *MissionId.ToString());
        return;
    }

    // Fade-out time is based on the previous track (if we have one).
    float FadeOutTime = 0.0f;
    if (OverrideFadeOut >= 0.0f)
    {
        FadeOutTime = OverrideFadeOut;
    }
    else if (bHasLastFadeOutTime)
    {
        FadeOutTime = LastFadeOutTimeFromTrack;
    }
    else
    {
        // First track or no previous info: fall back to the new track's fade-out.
        FadeOutTime = TrackData.FadeOutTime;
    }

    // Fade-in time always comes from the new track (unless overridden).
    float FadeInTime = (OverrideFadeIn >= 0.0f)
        ? OverrideFadeIn
        : TrackData.FadeInTime;

    CurrentMissionId = MissionId;
    CurrentTrackId   = EffectiveTrackId;

    TSoftObjectPtr<USoundBase> SoundRef = TrackData.Sound;

    if (SoundRef.IsValid())
    {
        USoundBase* LoadedSound = SoundRef.Get();
        HandleLoadedSound(LoadedSound, TrackData, FadeOutTime, FadeInTime);
        return;
    }

    const FSoftObjectPath AssetPath = SoundRef.ToSoftObjectPath();
    if (!AssetPath.IsValid())
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] Invalid asset path for track '%s' (Mission='%s')."),
               *EffectiveTrackId.ToString(),
               *MissionId.ToString());
        return;
    }

    if (DebugMode == ESOTS_MusicDebugMode::Verbose)
    {
        UE_LOG(LogSOTSMusicManager, Log,
               TEXT("[MusicManager] Async loading track '%s' for Mission='%s' (%s)."),
               *EffectiveTrackId.ToString(),
               *MissionId.ToString(),
               *AssetPath.ToString());
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TWeakObjectPtr<USOTS_MMSSSubsystem> WeakThis(this);

    CurrentStreamableHandle = Streamable.RequestAsyncLoad(
        AssetPath,
        FStreamableDelegate::CreateLambda(
            [WeakThis, SoundRef, TrackData, FadeOutTime, FadeInTime]()
            {
                if (!WeakThis.IsValid())
                {
                    return;
                }

                USOTS_MMSSSubsystem* StrongThis = WeakThis.Get();
                USoundBase* LoadedSound = SoundRef.Get();
                StrongThis->HandleLoadedSound(LoadedSound, TrackData, FadeOutTime, FadeInTime);
            }));
}

void USOTS_MMSSSubsystem::HandleLoadedSound(
    USoundBase* LoadedSound,
    const FSOTS_MusicTrackEntry& TrackData,
    float FadeOutTime,
    float FadeInTime)
{
    if (!LoadedSound)
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] HandleLoadedSound called with null sound for TrackId='%s'."),
               *CurrentTrackId.ToString());
        return;
    }

    UWorld* World = ResolveAudioWorld(GetWorld());
    if (!World)
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] HandleLoadedSound: World is null."));
        return;
    }

    // Fade out previous playback using a short-lived component for crossfades.
    USoundBase* PreviousSound = nullptr;

    if (PersistentAudioComponent && PersistentAudioComponent->IsRegistered() && PersistentAudioComponent->IsPlaying())
    {
        PreviousSound = PersistentAudioComponent->GetSound();

        // At the moment we do not query an exact playback time from the
        // component; crossfades start from the beginning of the previous
        // track and we treat the upcoming track as a fresh start.
        LastPlaybackTimeSeconds = 0.0f;
        bWasPlayingBeforeWorldChange = false;

        if (PreviousSound)
        {
            FadingOutComponent = NewObject<UAudioComponent>(World);
            if (FadingOutComponent)
            {
                FadingOutComponent->bAutoDestroy = true;
                FadingOutComponent->bIsUISound = false;
                FadingOutComponent->bAllowSpatialization = false;

                FadingOutComponent->RegisterComponentWithWorld(World);
                FadingOutComponent->SetSound(PreviousSound);
                FadingOutComponent->SetVolumeMultiplier(1.0f);
                FadingOutComponent->Play(0.0f);
                FadingOutComponent->FadeOut(FMath::Max(0.0f, FadeOutTime), 0.0f);
            }
        }

        PersistentAudioComponent->Stop();
    }
    else
    {
        LastPlaybackTimeSeconds = 0.0f;
        bWasPlayingBeforeWorldChange = false;
    }

    // Ensure we have a persistent, GI-owned audio component for the new track.
    UAudioComponent* NewComponent = EnsurePersistentAudioComponent(World, LoadedSound);
    if (!NewComponent)
    {
        UE_LOG(LogSOTSMusicManager, Warning,
               TEXT("[MusicManager] Failed to get persistent audio component for TrackId='%s'."),
               *CurrentTrackId.ToString());
        return;
    }

    NewComponent->bIsUISound = false;
    NewComponent->bAllowSpatialization = false;
    NewComponent->bAutoDestroy = false;
    NewComponent->SetVolumeMultiplier(TrackData.VolumeMultiplier);

    const float TargetVolume = TrackData.VolumeMultiplier;

    // Resume playback from the last known time only when we are effectively
    // recreating the same sound (e.g., across a world change).
    if (bWasPlayingBeforeWorldChange &&
        LastPlaybackTimeSeconds > 0.0f &&
        PreviousSound == LoadedSound)
    {
        NewComponent->Play(LastPlaybackTimeSeconds);

        if (DebugMode != ESOTS_MusicDebugMode::Off)
        {
            UE_LOG(LogSOTSMusicManager, Log,
                   TEXT("[MusicManager] Resuming TrackId='%s' at %.2f seconds."),
                   *CurrentTrackId.ToString(),
                   LastPlaybackTimeSeconds);
        }
    }
    else
    {
        if (FadeInTime > 0.0f)
        {
            NewComponent->FadeIn(FadeInTime, TargetVolume);
        }
        else
        {
            NewComponent->Play(0.0f);
        }
    }

    // Reset resume flag after we've applied it.
    bWasPlayingBeforeWorldChange = false;

    if (DebugMode != ESOTS_MusicDebugMode::Off)
    {
        UE_LOG(LogSOTSMusicManager, Log,
               TEXT("[MusicManager] Now playing TrackId='%s' (FadeIn=%.2f, FadeOut=%.2f, Volume=%.2f)."),
               *CurrentTrackId.ToString(),
               FadeInTime,
               FadeOutTime,
               TrackData.VolumeMultiplier);
    }

    // We just started this track; remember its fade-out duration for the next
    // transition so the previous track can use its own FadeOutTime.
    LastFadeOutTimeFromTrack = TrackData.FadeOutTime;
    bHasLastFadeOutTime = true;
}

UWorld* USOTS_MMSSSubsystem::ResolveAudioWorld(const UObject* WorldContextObject) const
{
    // Prefer an explicit world context if one is provided and the engine is available.
    if (WorldContextObject && GEngine)
    {
        if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            return World;
        }
    }

    // Fall back to any cached world the subsystem has used previously.
    if (CachedWorld.IsValid())
    {
        return CachedWorld.Get();
    }

    // As a last resort, use the GameInstance's world.
    if (const UGameInstance* GI = GetGameInstance())
    {
        return GI->GetWorld();
    }

    return nullptr;
}

UAudioComponent* USOTS_MMSSSubsystem::EnsurePersistentAudioComponent(UWorld* World, USoundBase* NewSound)
{
    if (!World || !NewSound)
    {
        return nullptr;
    }

    // Create the component on demand, using the subsystem as the outer so it
    // shares the GameInstance lifetime.
    if (!PersistentAudioComponent)
    {
        PersistentAudioComponent = NewObject<UAudioComponent>(this);
        if (!PersistentAudioComponent)
        {
            return nullptr;
        }

        PersistentAudioComponent->bAutoDestroy = false;
        PersistentAudioComponent->bIsUISound = false;
        PersistentAudioComponent->bAllowSpatialization = false;
    }

    // Ensure the component is registered with the requested world. If it is
    // already registered with another world, unregister first.
    if (!PersistentAudioComponent->IsRegistered() || PersistentAudioComponent->GetWorld() != World)
    {
        if (PersistentAudioComponent->IsRegistered())
        {
            PersistentAudioComponent->UnregisterComponent();
        }

        PersistentAudioComponent->RegisterComponentWithWorld(World);
    }

    CachedWorld = World;
    PersistentAudioComponent->SetSound(NewSound);

    return PersistentAudioComponent;
}
