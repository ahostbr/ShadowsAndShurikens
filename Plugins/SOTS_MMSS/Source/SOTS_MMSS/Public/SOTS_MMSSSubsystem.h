#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/StreamableManager.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_MMSS_Types.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_MMSSSubsystem.generated.h"

class USOTS_MissionMusicLibrary;
class UAudioComponent;
class UWorld;

/**
 * GameInstance-scoped music manager for SOTS.
 *
 * - Tracks the current mission id and active music track.
 * - Resolves music requests via a mission-scoped music library DataAsset.
 * - Async-loads USoundBase assets (MetaSounds, SoundCues) and performs
 *   cross-fades between tracks using UAudioComponent.
 * - Intended as a central backend; other systems (GSM, MissionDirector, etc.)
 *   call into this subsystem via simple tag-based APIs.
 */
UCLASS()
class SOTS_MMSS_API USOTS_MMSSSubsystem : public UGameInstanceSubsystem, public ISOTS_ProfileSnapshotProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ----- Config / setup -----

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music")
    void SetMusicLibrary(USOTS_MissionMusicLibrary* InLibrary);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music")
    void SetCurrentMissionId(FName NewMissionId);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music|Debug")
    void SetDebugMode(ESOTS_MusicDebugMode NewMode);

    UFUNCTION(BlueprintPure, Category = "SOTS|Music")
    FName GetCurrentMissionId() const { return CurrentMissionId; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Music")
    FGameplayTag GetCurrentTrackId() const { return CurrentTrackId; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Music")
    FGameplayTag GetCurrentMusicRoleTag() const { return CurrentMusicRoleTag; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Music")
    float GetCurrentPlaybackTime() const { return LastPlaybackTimeSeconds; }

    // ----- Core API: switch music -----

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music", meta = (WorldContext = "WorldContextObject"))
    void RequestMusicByTag(
        const UObject* WorldContextObject,
        const FGameplayTag& TrackId,
        bool bForceRestart = false,
        float OverrideFadeOut = -1.0f,
        float OverrideFadeIn  = -1.0f);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music")
    void RequestMusicByMissionAndTag(
        FName MissionId,
        const FGameplayTag& TrackId,
        bool bForceRestart = false,
        float OverrideFadeOut = -1.0f,
        float OverrideFadeIn  = -1.0f);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music")
    void RequestRoleTrack(FGameplayTag RoleTag, FName TrackIdName = NAME_None, float StartTime = 0.f);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music")
    void StopMusic(float FadeOutTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Music|Debug")
    void DebugPrintCurrentState() const;

    void BuildProfileData(FSOTS_MMSSProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_MMSSProfileData& InData);
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) override;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) override;

protected:
    UPROPERTY()
    USOTS_MissionMusicLibrary* Library = nullptr;

    UPROPERTY()
    FName CurrentMissionId;

    UPROPERTY()
    FGameplayTag CurrentTrackId;

    UPROPERTY()
    FGameplayTag CurrentMusicRoleTag;

    UPROPERTY()
    FName CurrentTrackIdName = NAME_None;

    UPROPERTY()
    bool bIsPlaying = false;

    UPROPERTY()
    ESOTS_MusicDebugMode DebugMode = ESOTS_MusicDebugMode::Basic;

    TSharedPtr<FStreamableHandle> CurrentStreamableHandle;

    // The main music audio component, owned by this subsystem / GI.
    UPROPERTY(Transient)
    UAudioComponent* PersistentAudioComponent = nullptr;

    // Optional fading-out component for crossfades.
    UPROPERTY(Transient)
    UAudioComponent* FadingOutComponent = nullptr;

    // Last known playback time in seconds for the currently tracked music.
    UPROPERTY(Transient)
    float LastPlaybackTimeSeconds = 0.0f;

    // Flag to indicate a stored resume time (LastPlaybackTimeSeconds) should be honored on the next load.
    UPROPERTY(Transient)
    bool bWasPlayingBeforeWorldChange = false;

    // Cached pointer to the last world we registered the audio component with.
    UPROPERTY(Transient)
    TWeakObjectPtr<UWorld> CachedWorld;

    // Stores the fade-out time of the last track that started playing.
    UPROPERTY(Transient)
    float LastFadeOutTimeFromTrack = 0.0f;

    UPROPERTY(Transient)
    bool bHasLastFadeOutTime = false;

    void InternalRequestTrack(FName MissionId,
                              const FGameplayTag& TrackId,
                              bool bForceRestart,
                              float OverrideFadeOut,
                              float OverrideFadeIn);

    void HandleLoadedSound(USoundBase* LoadedSound,
                           const FSOTS_MusicTrackEntry& TrackData,
                           float FadeOutTime,
                           float FadeInTime);

    // Resolve a reasonable world for audio registration.
    UWorld* ResolveAudioWorld(const UObject* WorldContextObject) const;

    // Ensures the persistent audio component exists and is registered for the given world.
    UAudioComponent* EnsurePersistentAudioComponent(UWorld* World, USoundBase* NewSound);
};
