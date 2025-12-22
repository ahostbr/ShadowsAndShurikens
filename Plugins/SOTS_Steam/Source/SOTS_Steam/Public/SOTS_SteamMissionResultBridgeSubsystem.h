#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_MissionDirectorTypes.h"
#include "SOTS_SteamSubsystemBase.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamMissionResultBridgeSubsystem.generated.h"

class USOTS_MissionDirectorSubsystem;
class USOTS_SteamAchievementsSubsystem;
class USOTS_SteamLeaderboardsSubsystem;

UCLASS()
class SOTS_STEAM_API USOTS_SteamMissionResultBridgeSubsystem : public USOTS_SteamSubsystemBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void HandleMissionEnded(const FSOTS_MissionRunSummary& Summary);
    FSOTS_SteamMissionResult BuildSteamMissionResult(const FSOTS_MissionRunSummary& Summary) const;
    FGameplayTag BuildMissionTag(const FName& MissionId) const;
    void AppendMissionEventTags(FGameplayTagContainer& OutTags, const FSOTS_MissionRunSummary& Summary) const;
    void ExtractModifierFlags(const FGameplayTagContainer& Tags, bool& bOutNoKills, bool& bOutNoAlerts, bool& bOutPerfectStealth) const;
    FString GetCurrentPlayerName() const;
    void SubmitToLeaderboards(const FSOTS_SteamMissionResult& Result);
    void DispatchAchievements(const FSOTS_SteamMissionResult& Result);

    TWeakObjectPtr<USOTS_MissionDirectorSubsystem> MissionDirector;
    bool bMissionEndedBound = false;

    double LastSubmissionTimeSeconds = 0.0;
    float SubmissionRateLimitSeconds = 1.5f;
};
