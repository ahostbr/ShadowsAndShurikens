#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_ProfileSubsystem.generated.h"

UCLASS()
class SHADOWSANDSHURIKENS_API USOTS_ProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    bool LoadProfile(const FSOTS_ProfileId& ProfileId, FSOTS_ProfileSnapshot& OutSnapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    bool SaveProfile(const FSOTS_ProfileId& ProfileId, const FSOTS_ProfileSnapshot& InSnapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void BuildSnapshotFromWorld(FSOTS_ProfileSnapshot& OutSnapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void ApplySnapshotToWorld(const FSOTS_ProfileSnapshot& InSnapshot);

    void BuildCharacterStatsStateFromWorld(AActor* Character, FSOTS_CharacterStateData& OutState) const;
    void ApplyCharacterStatsStateToWorld(AActor* Character, const FSOTS_CharacterStateData& InState) const;

    UFUNCTION(exec)
    void SOTS_DumpCurrentPlayerStatsSnapshot();

protected:
    FString GetSlotNameForProfile(const FSOTS_ProfileId& ProfileId) const;
};
