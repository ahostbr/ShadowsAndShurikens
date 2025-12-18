#include "SOTS_InvSPAdapter.h"

void USOTS_InvSPAdapter::OpenInventory_Implementation()
{
}

void USOTS_InvSPAdapter::CloseInventory_Implementation()
{
}

void USOTS_InvSPAdapter::RefreshInventoryView_Implementation()
{
}

void USOTS_InvSPAdapter::ToggleInventory_Implementation()
{
	OpenInventory();
}

void USOTS_InvSPAdapter::SetShortcutMenuVisible_Implementation(bool bVisible)
{
	(void)bVisible;
}

void USOTS_InvSPAdapter::NotifyPickupItem_Implementation(FGameplayTag ItemTag, int32 Quantity)
{
	(void)ItemTag;
	(void)Quantity;
}

void USOTS_InvSPAdapter::NotifyFirstTimePickup_Implementation(FGameplayTag ItemTag)
{
	(void)ItemTag;
}

void USOTS_InvSPAdapter::OpenContainer_Implementation(AActor* ContainerActor)
{
	(void)ContainerActor;
}

void USOTS_InvSPAdapter::CloseContainer_Implementation()
{
}
