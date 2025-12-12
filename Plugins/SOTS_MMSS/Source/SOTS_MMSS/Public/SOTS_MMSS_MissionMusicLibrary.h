#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_MMSS_Types.h"
#include "SOTS_MMSS_MissionMusicLibrary.generated.h"

/**
 * Mission-scoped music library that maps mission ids to music sets.
 * Tracks are keyed by gameplay tags so other systems can refer to them
 * without hard dependencies on concrete assets.
 */
UCLASS(BlueprintType)
class SOTS_MMSS_API USOTS_MissionMusicLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    // MissionId -> FSOTS_MissionMusicSet
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    TMap<FName, FSOTS_MissionMusicSet> MissionMusic;

    // Fallback if a mission key is not found.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Music")
    FSOTS_MissionMusicSet DefaultMusicSet;

    bool GetTrack(const FName& MissionId,
                  const FGameplayTag& TrackId,
                  FSOTS_MusicTrackEntry& OutTrack) const
    {
        if (const FSOTS_MissionMusicSet* MissionSet = MissionMusic.Find(MissionId))
        {
            if (const FSOTS_MusicTrackEntry* Track = MissionSet->Tracks.Find(TrackId))
            {
                OutTrack = *Track;
                return true;
            }
        }

        if (const FSOTS_MusicTrackEntry* Track = DefaultMusicSet.Tracks.Find(TrackId))
        {
            OutTrack = *Track;
            return true;
        }

        return false;
    }
};

