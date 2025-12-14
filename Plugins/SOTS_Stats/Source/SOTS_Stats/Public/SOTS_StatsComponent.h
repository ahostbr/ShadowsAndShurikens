#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSOTS_OnStatChangedSignature,
    FGameplayTag, StatTag,
    float, OldValue,
    float, NewValue
);

UCLASS(ClassGroup = (SOTS), meta = (BlueprintSpawnableComponent))
class SOTS_STATS_API USOTS_StatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_StatsComponent();

    UFUNCTION(BlueprintPure, Category = "SOTS|Stats")
    float GetStatValue(FGameplayTag StatTag) const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats")
    void SetStatValue(FGameplayTag StatTag, float NewValue);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats")
    void AddToStat(FGameplayTag StatTag, float Delta);

    UFUNCTION(BlueprintPure, Category = "SOTS|Stats")
    bool HasStat(FGameplayTag StatTag) const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Stats")
    const TMap<FGameplayTag, float>& GetAllStats() const { return StatValues; }

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats")
    void SetAllStats(const TMap<FGameplayTag, float>& NewStats);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stats|Snapshot")
    void WriteToCharacterState(UPARAM(ref) FSOTS_CharacterStateData& InOutState) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Stats|Snapshot")
    void ReadFromCharacterState(const FSOTS_CharacterStateData& InState);

    void BuildCharacterStateData(struct FSOTS_CharacterStateData& OutState) const;
    void ApplyCharacterStateData(const struct FSOTS_CharacterStateData& InState);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats")
    void SetStatBounds(FGameplayTag StatTag, float MinValue, float MaxValue);

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Stats")
    FSOTS_OnStatChangedSignature OnStatChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Stats")
    TMap<FGameplayTag, float> StatValues;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Stats")
    TMap<FGameplayTag, float> StatMinValues;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Stats")
    TMap<FGameplayTag, float> StatMaxValues;

private:
    float ClampToBounds(FGameplayTag StatTag, float InValue) const;
    void InternalSetStat(FGameplayTag StatTag, float NewValue, bool bBroadcast);
public:
};
