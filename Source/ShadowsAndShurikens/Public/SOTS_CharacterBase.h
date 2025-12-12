#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsComponent.h"
#include "SOTS_ParkourComponent.h"
#include "SOTS_CharacterBase.generated.h"

UCLASS()
class SHADOWSANDSHURIKENS_API ASOTS_CharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    ASOTS_CharacterBase(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintPure, Category = "SOTS|Components")
    USOTS_StatsComponent* GetStatsComponent() const { return StatsComponent; }

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void BuildCharacterStateSnapshot(FSOTS_CharacterStateData& OutData) const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void ApplyCharacterStateSnapshot(const FSOTS_CharacterStateData& InData);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Components", meta = (AllowPrivateAccess = "true"))
    USOTS_StatsComponent* StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USOTS_ParkourComponent> ParkourComponent;
};
