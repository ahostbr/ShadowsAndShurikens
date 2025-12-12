#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_WidgetRegistryTypes.h"
#include "SOTS_WidgetRegistryDataAsset.generated.h"

UCLASS(BlueprintType)
class SOTS_UI_API USOTS_WidgetRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	TArray<FSOTS_WidgetRegistryEntry> Entries;

	bool FindEntry(FGameplayTag Id, FSOTS_WidgetRegistryEntry& Out) const;

	const FSOTS_WidgetRegistryEntry* FindEntryPtr(FGameplayTag Id) const;
};
