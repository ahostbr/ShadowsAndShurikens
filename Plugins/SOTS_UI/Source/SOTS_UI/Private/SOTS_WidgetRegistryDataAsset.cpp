#include "SOTS_WidgetRegistryDataAsset.h"

bool USOTS_WidgetRegistryDataAsset::FindEntry(FGameplayTag Id, FSOTS_WidgetRegistryEntry& Out) const
{
	if (const FSOTS_WidgetRegistryEntry* Found = FindEntryPtr(Id))
	{
		Out = *Found;
		return true;
	}

	return false;
}

const FSOTS_WidgetRegistryEntry* USOTS_WidgetRegistryDataAsset::FindEntryPtr(FGameplayTag Id) const
{
	if (!Id.IsValid())
	{
		return nullptr;
	}

	return Entries.FindByPredicate([Id](const FSOTS_WidgetRegistryEntry& Entry)
	{
		return Entry.WidgetId == Id;
	});
}
