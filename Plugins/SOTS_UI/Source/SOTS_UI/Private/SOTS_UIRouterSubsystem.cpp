#include "SOTS_UIRouterSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Set.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SOTS_InvSPAdapter.h"
#include "SOTS_HUDSubsystem.h"
#include "SOTS_NotificationSubsystem.h"
#include "SOTS_ProHUDAdapter.h"
#include "Adapters/SOTS_InteractionEssentialsAdapter.h"
#include "SOTS_UIIntentTags.h"
#include "SOTS_UIModalResultTypes.h"
#include "SOTS_UIPayloadTypes.h"
#include "SOTS_WaypointSubsystem.h"
#include "SOTS_WidgetRegistryDataAsset.h"
#include "SOTS_UISettings.h"
#include "SOTS_InputAPI.h"
#include "SOTS_InteractionSubsystem.h"
#include "SOTS_ProfileSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UIRouter, Log, All);

namespace
{
	static constexpr ESOTS_UILayer LayerPriority[] = {
		ESOTS_UILayer::Modal,
		ESOTS_UILayer::Overlay,
		ESOTS_UILayer::Debug,
		ESOTS_UILayer::HUD
	};

	static const FGameplayTag UINavLayerTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Input.Layer.UI.Nav")), false);
	static TAutoConsoleVariable<int32> CVarLogUnhandledInteractionVerbs(
		TEXT("sots.ui.LogUnhandledInteractionVerbs"),
		0,
		TEXT("When enabled, logs each unhandled interaction verb once per session."),
		ECVF_Default);

	static TSet<FName> LoggedUnhandledInteractionVerbTags;

	void MaybeLogUnhandledInteractionVerb(const FGameplayTag& VerbTag)
	{
		if (CVarLogUnhandledInteractionVerbs.GetValueOnGameThread() == 0 || !VerbTag.IsValid())
		{
			return;
		}

		const FName VerbName = VerbTag.GetTagName();
		if (VerbName.IsNone() || LoggedUnhandledInteractionVerbTags.Contains(VerbName))
		{
			return;
		}

		LoggedUnhandledInteractionVerbTags.Add(VerbName);
		UE_LOG(LogSOTS_UIRouter, Verbose, TEXT(
			"UIRouter: Interaction verb %s is not owned by UI. SOTS_Interaction, SOTS_BodyDrag, and SOTS_KillExecutionManager handle execute/drag semantics. Enable sots.ui.LogUnhandledInteractionVerbs to repeat this message once per verb per session."),
			*VerbTag.ToString());
	}
}

USOTS_UIRouterSubsystem* USOTS_UIRouterSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<USOTS_UIRouterSubsystem>();
		}
	}

	return nullptr;
}

void USOTS_UIRouterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!WidgetRegistry.IsValid() && WidgetRegistry.IsNull())
	{
		if (const USOTS_UISettings* Settings = USOTS_UISettings::Get())
		{
			WidgetRegistry = Settings->DefaultWidgetRegistry;

			if (!ProHUDAdapterClass)
			{
				ProHUDAdapterClass = Settings->ProHUDAdapterClass;
			}

			if (!InvSPAdapterClass)
			{
				InvSPAdapterClass = Settings->InvSPAdapterClass;
			}

			if (!InteractionAdapterClass)
			{
				InteractionAdapterClass = Settings->InteractionAdapterClass;
			}
		}
	}

	if (WidgetRegistry.IsValid())
	{
		LoadedRegistry = WidgetRegistry.Get();
	}
	else if (WidgetRegistry.IsNull() == false)
	{
		LoadedRegistry = WidgetRegistry.LoadSynchronous();
	}

	if (!LoadedRegistry)
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: WidgetRegistry not set (expected /Game/... DA)"));
	}

	EnsureAdapters();
	EnsureInteractionActionBinding();

	static bool bLoggedBaseZOrders = false;
	if (!bLoggedBaseZOrders)
	{
		bLoggedBaseZOrders = true;
		UE_LOG(LogSOTS_UIRouter, Log, TEXT("UIRouter base ZOrders HUD=%d Overlay=%d Modal=%d Debug=%d"),
			GetLayerBaseZOrder(ESOTS_UILayer::HUD),
			GetLayerBaseZOrder(ESOTS_UILayer::Overlay),
			GetLayerBaseZOrder(ESOTS_UILayer::Modal),
			GetLayerBaseZOrder(ESOTS_UILayer::Debug));
	}
}

void USOTS_UIRouterSubsystem::Deinitialize()
{
	ActiveLayerStacks.Empty();
	CachedWidgets.Empty();
	ActiveExternalMenus.Empty();
	ProHUDAdapter = nullptr;
	InvSPAdapterInstance = nullptr;
	InteractionAdapter = nullptr;
	LoadedRegistry = nullptr;
	bGamePausedForUI = false;
	UpdateUINavLayerState(false);

	if (bInteractionActionBindingInitialized)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (USOTS_InteractionSubsystem* InteractionSubsystem = GameInstance->GetSubsystem<USOTS_InteractionSubsystem>())
				{
					InteractionSubsystem->OnInteractionActionRequested.RemoveDynamic(this, &USOTS_UIRouterSubsystem::HandleInteractionActionRequested);
				}
			}
		}
		bInteractionActionBindingInitialized = false;
	}

	Super::Deinitialize();
}

bool USOTS_UIRouterSubsystem::PushWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload)
{
	return PushOrReplaceWidget(WidgetId, Payload, false);
}

UUserWidget* USOTS_UIRouterSubsystem::CreateWidgetById(FGameplayTag WidgetId)
{
	FSOTS_WidgetRegistryEntry Entry;
	if (!ResolveEntry(WidgetId, Entry))
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: WidgetId %s not found in registry"), *WidgetId.ToString());
		return nullptr;
	}

	return ResolveWidgetInstance(Entry);
}

bool USOTS_UIRouterSubsystem::PushWidgetByIdWithInstance(FGameplayTag WidgetId, UUserWidget* Widget, FInstancedStruct Payload)
{
	if (!Widget)
	{
		return false;
	}

	FSOTS_WidgetRegistryEntry Entry;
	if (!ResolveEntry(WidgetId, Entry))
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: WidgetId %s not found in registry"), *WidgetId.ToString());
		return false;
	}

	return PushWidgetByEntry(Entry, Payload, false, Widget);
}

bool USOTS_UIRouterSubsystem::ReplaceTopWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload)
{
	return PushOrReplaceWidget(WidgetId, Payload, true);
}

bool USOTS_UIRouterSubsystem::PopWidget(ESOTS_UILayer OptionalLayerFilter, bool bUseLayerFilter)
{
	if (bUseLayerFilter)
	{
		if (FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(OptionalLayerFilter))
		{
			const int32 BeforeCount = Stack->Entries.Num();
			RemoveTopFromLayer(OptionalLayerFilter);
			RefreshInputAndPauseState();
			return Stack->Entries.Num() < BeforeCount;
		}

		return false;
	}

	for (ESOTS_UILayer Layer : LayerPriority)
	{
		if (const FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
		{
			if (Stack->Entries.Num() > 0)
			{
				if (Stack->Entries.Last().bExternalMenu)
				{
					return false;
				}

				RemoveTopFromLayer(Layer);
				RefreshInputAndPauseState();
				return true;
			}
		}
	}

	return false;
}

void USOTS_UIRouterSubsystem::PopAllModals()
{
	for (;;)
	{
		const FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(ESOTS_UILayer::Modal);
		if (!Stack || Stack->Entries.Num() == 0)
		{
			break;
		}

		RemoveTopFromLayer(ESOTS_UILayer::Modal);
	}

	RefreshInputAndPauseState();
}

void USOTS_UIRouterSubsystem::SetObjectiveText(const FString& InText)
{
	if (USOTS_HUDSubsystem* HUD = USOTS_HUDSubsystem::Get(this))
	{
		HUD->SetObjectiveText(InText);
	}
}

void USOTS_UIRouterSubsystem::ShowNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag)
{
	if (USOTS_NotificationSubsystem* Notifications = USOTS_NotificationSubsystem::Get(this))
	{
		Notifications->PushNotification(Message, DurationSeconds, CategoryTag);
	}

	EnsureAdapters();
	if (ProHUDAdapter)
	{
		ProHUDAdapter->PushNotification(Message, DurationSeconds, CategoryTag);
	}
}

void USOTS_UIRouterSubsystem::PushNotification_SOTS(const F_SOTS_UINotificationPayload& Payload)
{
	ShowNotification(Payload.Message.ToString(), Payload.DurationSeconds, Payload.CategoryTag);
}

void USOTS_UIRouterSubsystem::ShowPickupNotification(const FText& ItemName, int32 Quantity, float DurationSeconds)
{
	const FText Message = Quantity > 1
		? FText::Format(NSLOCTEXT("SOTS", "PickupNotification", "{0} x{1}"), ItemName, FText::AsNumber(Quantity))
		: ItemName;

	F_SOTS_UINotificationPayload Payload;
	Payload.Message = Message;
	Payload.DurationSeconds = DurationSeconds;
	Payload.CategoryTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Notification.Pickup")), false);

	const FGameplayTag WidgetTag = ResolveFirstValidTag({
		FName(TEXT("UI.HUD.PickupNotification")),
		FName(TEXT("SAS.UI.InvSP.PickupNotification"))
		});

	if (!PushNotificationWidget(WidgetTag, Payload))
	{
		PushNotification_SOTS(Payload);
	}
}

void USOTS_UIRouterSubsystem::ShowFirstTimePickupNotification(const FText& ItemName, float DurationSeconds)
{
	const FText Message = FText::Format(NSLOCTEXT("SOTS", "FirstTimePickupNotification", "New item: {0}"), ItemName);

	F_SOTS_UINotificationPayload Payload;
	Payload.Message = Message;
	Payload.DurationSeconds = DurationSeconds;
	Payload.CategoryTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Notification.Pickup.FirstTime")), false);

	const FGameplayTag WidgetTag = ResolveFirstValidTag({
		FName(TEXT("UI.HUD.PickupNotification.FirstTime")),
		FName(TEXT("SAS.UI.InvSP.PickupNotification.FirstTime"))
		});

	if (!PushNotificationWidget(WidgetTag, Payload))
	{
		PushNotification_SOTS(Payload);
	}
}

FGuid USOTS_UIRouterSubsystem::AddOrUpdateWaypoint_Actor(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges)
{
	FGuid Result;

	if (USOTS_WaypointSubsystem* Waypoints = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTS_WaypointSubsystem>() : nullptr)
	{
		Result = Waypoints->AddActorWaypoint(Target, CategoryTag, bClampToEdges).Id;
	}

	EnsureAdapters();
	if (ProHUDAdapter && Result.IsValid())
	{
		const FVector Location = Target ? Target->GetActorLocation() : FVector::ZeroVector;
		ProHUDAdapter->AddOrUpdateWorldMarker(Result, Target, Location, true, CategoryTag, bClampToEdges);
	}

	return Result;
}

FGuid USOTS_UIRouterSubsystem::AddOrUpdateWaypoint_Location(FVector Location, FGameplayTag CategoryTag, bool bClampToEdges)
{
	FGuid Result;

	if (USOTS_WaypointSubsystem* Waypoints = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTS_WaypointSubsystem>() : nullptr)
	{
		Result = Waypoints->AddLocationWaypoint(Location, CategoryTag, bClampToEdges).Id;
	}

	EnsureAdapters();
	if (ProHUDAdapter && Result.IsValid())
	{
		ProHUDAdapter->AddOrUpdateWorldMarker(Result, nullptr, Location, false, CategoryTag, bClampToEdges);
	}

	return Result;
}

void USOTS_UIRouterSubsystem::RemoveWaypoint(FGuid Id)
{
	if (USOTS_WaypointSubsystem* Waypoints = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTS_WaypointSubsystem>() : nullptr)
	{
		Waypoints->RemoveWaypoint(Id);
	}

	if (ProHUDAdapter)
	{
		ProHUDAdapter->RemoveWorldMarker(Id);
	}
}

FGuid USOTS_UIRouterSubsystem::AddOrUpdateWorldMarker_Actor(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges)
{
	return AddOrUpdateWaypoint_Actor(Target, CategoryTag, bClampToEdges);
}

FGuid USOTS_UIRouterSubsystem::AddOrUpdateWorldMarker_Location(FVector Location, FGameplayTag CategoryTag, bool bClampToEdges)
{
	return AddOrUpdateWaypoint_Location(Location, CategoryTag, bClampToEdges);
}

void USOTS_UIRouterSubsystem::RemoveWorldMarker(FGuid Id)
{
	RemoveWaypoint(Id);
}

bool USOTS_UIRouterSubsystem::OpenInventoryMenu()
{
	const FGameplayTag InventoryTag = ResolveFirstValidTag({
		FName(TEXT("UI.Menu.Inventory")),
		FName(TEXT("SAS.UI.InvSP.InventoryMenu"))
		});

	if (!InventoryTag.IsValid())
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: Inventory menu tag not configured."));
		return false;
	}

	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->OpenInventory();
	}

	NotifyExternalMenuOpened(InventoryTag);
	return PushInventoryWidget(InventoryTag, ESOTS_UIInventoryRequestType::OpenInventory);
}

bool USOTS_UIRouterSubsystem::CloseInventoryMenu()
{
	const FGameplayTag InventoryTag = ResolveFirstValidTag({
		FName(TEXT("UI.Menu.Inventory")),
		FName(TEXT("SAS.UI.InvSP.InventoryMenu"))
		});

	if (!InventoryTag.IsValid())
	{
		return false;
	}

	if (USOTS_InvSPAdapter* Adapter = GetInvSPAdapter())
	{
		Adapter->CloseInventory();
	}

	const bool bWasActive = IsWidgetActive(InventoryTag);
	NotifyExternalMenuClosed(InventoryTag);
	return bWasActive;
}

bool USOTS_UIRouterSubsystem::ToggleInventoryMenu()
{
	const FGameplayTag InventoryTag = ResolveFirstValidTag({
		FName(TEXT("UI.Menu.Inventory")),
		FName(TEXT("SAS.UI.InvSP.InventoryMenu"))
		});

	if (!InventoryTag.IsValid())
	{
		return false;
	}

	if (IsWidgetActive(InventoryTag))
	{
		if (USOTS_InvSPAdapter* Adapter = GetInvSPAdapter())
		{
			Adapter->CloseInventory();
		}

		NotifyExternalMenuClosed(InventoryTag);
		return true;
	}

	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->OpenInventory();
	}

	NotifyExternalMenuOpened(InventoryTag);
	return PushInventoryWidget(InventoryTag, ESOTS_UIInventoryRequestType::ToggleInventory);
}

bool USOTS_UIRouterSubsystem::OpenItemContainerMenu(AActor* ContainerActor)
{
	const FGameplayTag ContainerTag = ResolveFirstValidTag({
		FName(TEXT("UI.Menu.Container")),
		FName(TEXT("SAS.UI.InvSP.ContainerMenu"))
		});

	if (!ContainerTag.IsValid())
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: Container menu tag not configured."));
		return false;
	}

	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->OpenContainer(ContainerActor);
	}

	NotifyExternalMenuOpened(ContainerTag);
	return PushInventoryWidget(ContainerTag, ESOTS_UIInventoryRequestType::OpenContainer, ContainerActor);
}

bool USOTS_UIRouterSubsystem::CloseItemContainerMenu()
{
	const FGameplayTag ContainerTag = ResolveFirstValidTag({
		FName(TEXT("UI.Menu.Container")),
		FName(TEXT("SAS.UI.InvSP.ContainerMenu"))
		});

	if (!ContainerTag.IsValid())
	{
		return false;
	}

	if (USOTS_InvSPAdapter* Adapter = GetInvSPAdapter())
	{
		Adapter->CloseContainer();
	}

	const bool bWasActive = IsWidgetActive(ContainerTag);
	NotifyExternalMenuClosed(ContainerTag);
	return bWasActive;
}

void USOTS_UIRouterSubsystem::EnsureGameplayHUDReady()
{
	EnsureAdapters();
	if (ProHUDAdapter)
	{
		ProHUDAdapter->EnsureHUDCreated();
	}
}

void USOTS_UIRouterSubsystem::RegisterHUDHost(AHUD* HUD)
{
	if (!HUD)
	{
		return;
	}

	HUDHost = HUD;

	if (const USOTS_UISettings* Settings = USOTS_UISettings::Get())
	{
		if (Settings->bEnableSOTSCoreHUDHostBridge && Settings->bEnableSOTSCoreBridgeVerboseLogs)
		{
			UE_LOG(LogSOTS_UIRouter, Verbose, TEXT("Router: Registered HUD host via SOTS_Core bridge HUD=%s"), *GetNameSafe(HUD));
		}
	}
}

bool USOTS_UIRouterSubsystem::HasHUDHost() const
{
	return HUDHost.IsValid();
}

FString USOTS_UIRouterSubsystem::GetHUDHostDebugString() const
{
	AHUD* Host = HUDHost.Get();
	return FString::Printf(TEXT("HUDHost=%s"), *GetNameSafe(Host));
}

USOTS_InvSPAdapter* USOTS_UIRouterSubsystem::EnsureInvSPAdapter()
{
	if (InvSPAdapterInstance)
	{
		return InvSPAdapterInstance;
	}

	UClass* ClassToUse = nullptr;
	if (InvSPAdapterClassOverride)
	{
		ClassToUse = InvSPAdapterClassOverride;
	}
	else if (InvSPAdapterClass)
	{
		ClassToUse = InvSPAdapterClass;
	}
	else
	{
		ClassToUse = USOTS_InvSPAdapter::StaticClass();
	}

	InvSPAdapterInstance = NewObject<USOTS_InvSPAdapter>(GetGameInstance(), ClassToUse);
	return InvSPAdapterInstance;
}

USOTS_InvSPAdapter* USOTS_UIRouterSubsystem::GetInvSPAdapter() const
{
	return InvSPAdapterInstance;
}

void USOTS_UIRouterSubsystem::RequestInvSP_ToggleInventory()
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->ToggleInventory();
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_OpenInventory()
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->OpenInventory();
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_CloseInventory()
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->CloseInventory();
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_RefreshInventory()
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->RefreshInventoryView();
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_SetShortcutMenuVisible(bool bVisible)
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->SetShortcutMenuVisible(bVisible);
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_NotifyPickupItem(FGameplayTag ItemTag, int32 Quantity)
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->NotifyPickupItem(ItemTag, Quantity);
	}
}

void USOTS_UIRouterSubsystem::RequestInvSP_NotifyFirstTimePickup(FGameplayTag ItemTag)
{
	if (USOTS_InvSPAdapter* Adapter = EnsureInvSPAdapter())
	{
		Adapter->NotifyFirstTimePickup(ItemTag);
	}
}

void USOTS_UIRouterSubsystem::SubmitModalResult(const F_SOTS_UIModalResult& Result)
{
	OnModalResult.Broadcast(Result);

	if (Result.ActionTag.IsValid())
	{
		ExecuteSystemAction(Result.ActionTag);
	}

	// Close top modal after handling result.
	PopWidget(ESOTS_UILayer::Modal, true);
}

bool USOTS_UIRouterSubsystem::ShowConfirmDialog(const F_SOTS_UIConfirmDialogPayload& Payload)
{
	F_SOTS_UIConfirmDialogPayload LocalPayload = Payload;
	if (!LocalPayload.RequestId.IsValid())
	{
		LocalPayload.RequestId = FGuid::NewGuid();
	}

	const FGameplayTag ModalTag = GetDefaultConfirmDialogTag();
	if (!ModalTag.IsValid())
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: Confirm dialog tag not configured; cannot show dialog."));
		return false;
	}

	FInstancedStruct PayloadStruct;
	PayloadStruct.InitializeAs<F_SOTS_UIConfirmDialogPayload>(LocalPayload);
	return PushWidgetById(ModalTag, PayloadStruct);
}

void USOTS_UIRouterSubsystem::NotifyExternalMenuOpened(FGameplayTag MenuIdTag)
{
	if (!MenuIdTag.IsValid() || !IsExternalMenuTag(MenuIdTag))
	{
		return;
	}

	if (ActiveExternalMenus.Contains(MenuIdTag))
	{
		return;
	}

	ActiveExternalMenus.Add(MenuIdTag);
	OnExternalMenuOpened.Broadcast(MenuIdTag);
}

void USOTS_UIRouterSubsystem::NotifyExternalMenuClosed(FGameplayTag MenuIdTag)
{
	if (!MenuIdTag.IsValid())
	{
		return;
	}

	if (IsExternalMenuTag(MenuIdTag))
	{
		if (ActiveExternalMenus.Remove(MenuIdTag) > 0)
		{
			OnExternalMenuClosed.Broadcast(MenuIdTag);
		}
	}

	if (IsWidgetActive(MenuIdTag))
	{
		PopWidgetById(MenuIdTag);
	}
}

F_SOTS_UIConfirmDialogPayload USOTS_UIRouterSubsystem::BuildReturnToMainMenuPayload(const FText& MessageOverride) const
{
	F_SOTS_UIConfirmDialogPayload Payload;
	Payload.Title = NSLOCTEXT("SOTS", "UI_ReturnToMainMenu_Title", "Return to Main Menu");
	Payload.Message = MessageOverride.IsEmpty()
		? NSLOCTEXT("SOTS", "UI_ReturnToMainMenu_Body", "Return to the main menu? Unsaved progress may be lost.")
		: MessageOverride;
	Payload.ConfirmText = NSLOCTEXT("SOTS", "UI_ReturnToMainMenu_Confirm", "Yes");
	Payload.CancelText = NSLOCTEXT("SOTS", "UI_ReturnToMainMenu_Cancel", "No");
	Payload.ConfirmActionTag = SOTS_UIIntentTags::GetReturnToMainMenuTag();
	Payload.CancelActionTag = SOTS_UIIntentTags::GetCloseTopModalTag();
	Payload.bCloseOnConfirm = true;
	Payload.bCloseOnCancel = true;
	return Payload;
}

bool USOTS_UIRouterSubsystem::RequestReturnToMainMenu(const FText& MessageOverride)
{
	const F_SOTS_UIConfirmDialogPayload Payload = BuildReturnToMainMenuPayload(MessageOverride);
	return ShowConfirmDialog(Payload);
}

bool USOTS_UIRouterSubsystem::HandleInteractionIntent(FGameplayTag IntentTag, FInstancedStruct Payload)
{
	if (!IntentTag.IsValid())
	{
		return false;
	}

	if (DispatchInteractionIntent(IntentTag, Payload))
	{
		return true;
	}

	if (DispatchInteractionMarkerIntent(IntentTag, Payload))
	{
		return true;
	}

	UE_LOG(LogSOTS_UIRouter, Verbose, TEXT("Router: Interaction intent %s not handled."), *IntentTag.ToString());
	return false;
}

bool USOTS_UIRouterSubsystem::PushOrReplaceWidget(FGameplayTag WidgetId, FInstancedStruct Payload, bool bReplaceTop)
{
	FSOTS_WidgetRegistryEntry Entry;
	if (!ResolveEntry(WidgetId, Entry))
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: WidgetId %s not found in registry"), *WidgetId.ToString());
		return false;
	}

	return PushWidgetByEntry(Entry, Payload, bReplaceTop);
}

bool USOTS_UIRouterSubsystem::PushWidgetByEntry(const FSOTS_WidgetRegistryEntry& Entry, FInstancedStruct Payload, bool bReplaceTop, UUserWidget* WidgetOverride)
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return false;
	}

	FSOTS_LayerStack& Stack = ActiveLayerStacks.FindOrAdd(Entry.Layer);

	// Remove duplicates when multiple instances are not allowed.
	if (!Entry.bAllowMultiple)
	{
		for (int32 Index = Stack.Entries.Num() - 1; Index >= 0; --Index)
		{
			if (Stack.Entries[Index].WidgetId == Entry.WidgetId)
			{
				if (UUserWidget* ExistingWidget = Stack.Entries[Index].Widget.Get())
				{
					ExistingWidget->RemoveFromParent();
				}
				Stack.Entries.RemoveAt(Index);
			}
		}
	}

	if (bReplaceTop)
	{
		RemoveTopFromLayer(Entry.Layer);
	}

	UUserWidget* WidgetInstance = WidgetOverride ? WidgetOverride : ResolveWidgetInstance(Entry);
	if (!WidgetInstance)
	{
		return false;
	}

	if (WidgetOverride)
	{
		CacheWidgetIfNeeded(Entry, WidgetOverride);
	}

	const int32 ZOrder = GetLayerBaseZOrder(Entry.Layer) + Entry.ZOrder;
	WidgetInstance->AddToViewport(ZOrder);

	FSOTS_ActiveWidgetEntry ActiveEntry;
	ActiveEntry.WidgetId = Entry.WidgetId;
	ActiveEntry.Widget = WidgetInstance;
	ActiveEntry.Layer = Entry.Layer;
	ActiveEntry.InputPolicy = Entry.InputPolicy;
	ActiveEntry.CachePolicy = Entry.CachePolicy;
	ActiveEntry.bPauseGame = Entry.bPauseGame;
	ActiveEntry.bCloseOnEscape = Entry.bCloseOnEscape;
	ActiveEntry.bExternalMenu = IsExternalMenuTag(Entry.WidgetId);
	ActiveEntry.Payload = Payload;

	Stack.Entries.Add(ActiveEntry);

	RefreshInputAndPauseState();
	return true;
}

bool USOTS_UIRouterSubsystem::ResolveEntry(FGameplayTag WidgetId, FSOTS_WidgetRegistryEntry& OutEntry)
{
	if (!WidgetId.IsValid())
	{
		return false;
	}

	if (!LoadedRegistry && WidgetRegistry.IsValid())
	{
		LoadedRegistry = WidgetRegistry.Get();
	}

	if (!LoadedRegistry && WidgetRegistry.IsNull() == false)
	{
		LoadedRegistry = WidgetRegistry.LoadSynchronous();
	}

	if (!LoadedRegistry)
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: Widget registry not configured"));
		return false;
	}

	return LoadedRegistry->FindEntry(WidgetId, OutEntry);
}

void USOTS_UIRouterSubsystem::RefreshInputAndPauseState()
{
	const FSOTS_ActiveWidgetEntry* TopEntry = nullptr;
	for (ESOTS_UILayer Layer : LayerPriority)
	{
		if (const FSOTS_ActiveWidgetEntry* Candidate = GetTopEntry(Layer))
		{
			TopEntry = Candidate;
			break;
		}
	}

	ApplyInputPolicy(TopEntry);
	RefreshInputLayers(TopEntry);

	bool bShouldPause = false;
	for (const auto& Pair : ActiveLayerStacks)
	{
		for (const FSOTS_ActiveWidgetEntry& Entry : Pair.Value.Entries)
		{
			if (Entry.bPauseGame)
			{
				bShouldPause = true;
				break;
			}
		}

		if (bShouldPause)
		{
			break;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (bShouldPause && !bGamePausedForUI)
		{
			UGameplayStatics::SetGamePaused(World, true);
			bGamePausedForUI = true;
		}
		else if (!bShouldPause && bGamePausedForUI)
		{
			UGameplayStatics::SetGamePaused(World, false);
			bGamePausedForUI = false;
		}
	}
}

void USOTS_UIRouterSubsystem::ApplyInputPolicy(const FSOTS_ActiveWidgetEntry* TopEntry)
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return;
	}

	if (!TopEntry || !TopEntry->Widget)
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		return;
	}

	switch (TopEntry->InputPolicy)
	{
	case ESOTS_UIInputPolicy::UIOnly:
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TopEntry->Widget->TakeWidget());
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
		break;
	}
	case ESOTS_UIInputPolicy::GameAndUI:
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TopEntry->Widget->TakeWidget());
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
		break;
	}
	default:
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		break;
	}
}

void USOTS_UIRouterSubsystem::RefreshInputLayers(const FSOTS_ActiveWidgetEntry* TopEntry)
{
	const bool bShouldEnableUINav = ShouldEnableUINavLayer(TopEntry);
	UpdateUINavLayerState(bShouldEnableUINav);
}

void USOTS_UIRouterSubsystem::UpdateUINavLayerState(bool bShouldEnable)
{
	if (!UINavLayerTag.IsValid() || bUINavLayerActive == bShouldEnable)
	{
		return;
	}

	if (APlayerController* PC = GetPlayerController())
	{
		if (bShouldEnable)
		{
			USOTS_InputBlueprintLibrary::PushLayerTag(PC, UINavLayerTag);
			bUINavLayerActive = true;
		}
		else
		{
			USOTS_InputBlueprintLibrary::PopLayerTag(PC, UINavLayerTag);
			bUINavLayerActive = false;
		}
	}
}

bool USOTS_UIRouterSubsystem::ShouldEnableUINavLayer(const FSOTS_ActiveWidgetEntry* TopEntry) const
{
	return TopEntry && TopEntry->InputPolicy != ESOTS_UIInputPolicy::GameOnly;
}

void USOTS_UIRouterSubsystem::RemoveTopFromLayer(ESOTS_UILayer Layer)
{
	if (FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
	{
		if (Stack->Entries.Num() > 0)
		{
			FSOTS_ActiveWidgetEntry& RemovedEntry = Stack->Entries.Last();
			if (UUserWidget* Widget = RemovedEntry.Widget.Get())
			{
				Widget->RemoveFromParent();
			}

			const FGameplayTag RemovedTag = RemovedEntry.WidgetId;
			const bool bWasExternal = RemovedEntry.bExternalMenu;
			Stack->Entries.RemoveAt(Stack->Entries.Num() - 1);

			if (bWasExternal && ActiveExternalMenus.Remove(RemovedTag) > 0)
			{
				OnExternalMenuClosed.Broadcast(RemovedTag);
			}
		}
	}
}

FSOTS_ActiveWidgetEntry* USOTS_UIRouterSubsystem::GetTopEntry(ESOTS_UILayer Layer)
{
	if (FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
	{
		return Stack->Entries.Num() > 0 ? &Stack->Entries.Last() : nullptr;
	}

	return nullptr;
}

const FSOTS_ActiveWidgetEntry* USOTS_UIRouterSubsystem::GetTopEntry(ESOTS_UILayer Layer) const
{
	if (const FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
	{
		return Stack->Entries.Num() > 0 ? &Stack->Entries.Last() : nullptr;
	}

	return nullptr;
}

int32 USOTS_UIRouterSubsystem::GetLayerBaseZOrder(ESOTS_UILayer Layer) const
{
	switch (Layer)
	{
	// Visual ordering only: Debug overlays sit above everything else; input priority is controlled by LayerPriority separately.
	case ESOTS_UILayer::HUD:
		return 0;
	case ESOTS_UILayer::Overlay:
		return 100;
	case ESOTS_UILayer::Modal:
		return 1000;
	case ESOTS_UILayer::Debug:
		return 10000;
	default:
		return 0;
	}
}

UUserWidget* USOTS_UIRouterSubsystem::ResolveWidgetInstance(const FSOTS_WidgetRegistryEntry& Entry)
{
	const bool bCanUseCache = Entry.CachePolicy == ESOTS_UICachePolicy::KeepAlive && !Entry.bAllowMultiple;

	if (bCanUseCache)
	{
		if (TObjectPtr<UUserWidget>* Cached = CachedWidgets.Find(Entry.WidgetId))
		{
			if (UUserWidget* CachedWidget = Cached->Get())
			{
				return CachedWidget;
			}
		}
	}

	UClass* WidgetClass = Entry.WidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: WidgetClass missing for %s"), *Entry.WidgetId.ToString());
		return nullptr;
	}

	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (Widget && bCanUseCache)
	{
		CachedWidgets.Add(Entry.WidgetId, Widget);
	}

	return Widget;
}

void USOTS_UIRouterSubsystem::CacheWidgetIfNeeded(const FSOTS_WidgetRegistryEntry& Entry, UUserWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	const bool bCanUseCache = Entry.CachePolicy == ESOTS_UICachePolicy::KeepAlive && !Entry.bAllowMultiple;
	if (!bCanUseCache)
	{
		return;
	}

	if (!CachedWidgets.Contains(Entry.WidgetId))
	{
		CachedWidgets.Add(Entry.WidgetId, Widget);
	}
}

APlayerController* USOTS_UIRouterSubsystem::GetPlayerController() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetFirstPlayerController();
	}

	return nullptr;
}

void USOTS_UIRouterSubsystem::EnsureAdapters()
{
	if (!ProHUDAdapter && ProHUDAdapterClass)
	{
		ProHUDAdapter = NewObject<USOTS_ProHUDAdapter>(GetGameInstance(), ProHUDAdapterClass);
		if (ProHUDAdapter)
		{
			ProHUDAdapter->EnsureHUDCreated();
		}
	}

	EnsureInvSPAdapter();

	EnsureInteractionAdapter();

	EnsureInteractionActionBinding();
}

void USOTS_UIRouterSubsystem::EnsureInteractionAdapter()
{
	if (!InteractionAdapter && InteractionAdapterClass)
	{
		InteractionAdapter = NewObject<USOTS_InteractionEssentialsAdapter>(GetGameInstance(), InteractionAdapterClass);
	}
}

void USOTS_UIRouterSubsystem::EnsureInteractionActionBinding()
{
	if (bInteractionActionBindingInitialized)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USOTS_InteractionSubsystem* InteractionSubsystem = GameInstance->GetSubsystem<USOTS_InteractionSubsystem>())
			{
				InteractionSubsystem->OnInteractionActionRequested.AddDynamic(this, &USOTS_UIRouterSubsystem::HandleInteractionActionRequested);
				bInteractionActionBindingInitialized = true;
			}
		}
	}
}

void USOTS_UIRouterSubsystem::HandleInteractionActionRequested(const FSOTS_InteractionActionRequest& Request)
{
	if (!Request.VerbTag.IsValid())
	{
		return;
	}

	// Interaction action requests are owned by gameplay subsystems; UI stays intent-only.
	MaybeLogUnhandledInteractionVerb(Request.VerbTag);
}

bool USOTS_UIRouterSubsystem::DispatchInteractionIntent(FGameplayTag IntentTag, const FInstancedStruct& Payload)
{
	EnsureAdapters();

	if (!InteractionAdapter)
	{
		return false;
	}

	if (IntentTag == SOTS_UIIntentTags::GetInteractionHideTag())
	{
		InteractionAdapter->HidePrompt();
		return true;
	}

	const F_SOTS_InteractionPromptView* View = Payload.GetPtr<F_SOTS_InteractionPromptView>();
	if (!View)
	{
		return false;
	}

	InteractionAdapter->EnsureViewportCreated();

	if (IntentTag == SOTS_UIIntentTags::GetInteractionShowTag())
	{
		InteractionAdapter->ShowPrompt(*View);
		return true;
	}

	if (IntentTag == SOTS_UIIntentTags::GetInteractionUpdateTag())
	{
		InteractionAdapter->UpdatePrompt(*View);
		return true;
	}

	return false;
}

bool USOTS_UIRouterSubsystem::DispatchInteractionMarkerIntent(FGameplayTag IntentTag, const FInstancedStruct& Payload)
{
	EnsureAdapters();

	if (!InteractionAdapter)
	{
		return false;
	}

	const F_SOTS_InteractionMarkerView* MarkerView = Payload.GetPtr<F_SOTS_InteractionMarkerView>();

	if (IntentTag == SOTS_UIIntentTags::GetInteractionMarkerAddTag())
	{
		if (!MarkerView)
		{
			return false;
		}

		InteractionAdapter->AddOrUpdateMarker(*MarkerView);
		return true;
	}

	if (IntentTag == SOTS_UIIntentTags::GetInteractionMarkerRemoveTag())
	{
		FGuid MarkerId;
		if (MarkerView)
		{
			MarkerId = MarkerView->MarkerId;
		}
		else if (const FGuid* GuidPtr = Payload.GetPtr<FGuid>())
		{
			MarkerId = *GuidPtr;
		}

		if (MarkerId.IsValid())
		{
			InteractionAdapter->RemoveMarker(MarkerId);
			return true;
		}

		return false;
	}

	return false;
}

bool USOTS_UIRouterSubsystem::HandleReturnToMainMenuAction()
{
	const bool bHadListener = OnReturnToMainMenuRequested.IsBound();
	OnReturnToMainMenuRequested.Broadcast();

	if (!bHadListener)
	{
		UE_LOG(LogSOTS_UIRouter, Warning, TEXT("Router: ReturnToMainMenu requested but no listeners are bound to handle game flow."));
	}

	return true;
}

bool USOTS_UIRouterSubsystem::ExecuteSystemAction(FGameplayTag ActionTag)
{
	if (!ActionTag.IsValid())
	{
		return false;
	}

	if (ActionTag == SOTS_UIIntentTags::GetQuitGameTag())
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
				return true;
			}
		}
		return false;
	}

	if (ActionTag == SOTS_UIIntentTags::GetCloseTopModalTag())
	{
		return PopWidget(ESOTS_UILayer::Modal, true);
	}

	if (ActionTag == SOTS_UIIntentTags::GetOpenSettingsTag())
	{
		const FGameplayTag SettingsTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Menu.Settings")), false);
		if (SettingsTag.IsValid())
		{
			return PushWidgetById(SettingsTag, FInstancedStruct());
		}
	}

	if (ActionTag == SOTS_UIIntentTags::GetOpenProfilesTag())
	{
		const FGameplayTag ProfilesTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Menu.Profiles")), false);
		if (ProfilesTag.IsValid())
		{
			return PushWidgetById(ProfilesTag, FInstancedStruct());
		}
	}

	if (ActionTag == SOTS_UIIntentTags::GetReturnToMainMenuTag())
	{
		return HandleReturnToMainMenuAction();
	}

	return false;
}

FGameplayTag USOTS_UIRouterSubsystem::GetDefaultConfirmDialogTag() const
{
	const FGameplayTag PrimaryTag = SOTS_UIIntentTags::GetConfirmDialogTag();
	if (PrimaryTag.IsValid())
	{
		return PrimaryTag;
	}

	const FGameplayTag FallbackTag = SOTS_UIIntentTags::GetConfirmDialogFallbackTag();
	if (FallbackTag.IsValid())
	{
		return FallbackTag;
	}

	return FGameplayTag();
}

FGameplayTag USOTS_UIRouterSubsystem::ResolveFirstValidTag(const TArray<FName>& Names) const
{
	for (const FName& Name : Names)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Name, false);
		if (Tag.IsValid())
		{
			return Tag;
		}
	}

	return FGameplayTag();
}

bool USOTS_UIRouterSubsystem::PushNotificationWidget(FGameplayTag WidgetTag, const F_SOTS_UINotificationPayload& Payload)
{
	if (!WidgetTag.IsValid())
	{
		return false;
	}

	FInstancedStruct PayloadStruct;
	PayloadStruct.InitializeAs<F_SOTS_UINotificationPayload>(Payload);
	return PushWidgetById(WidgetTag, PayloadStruct);
}

bool USOTS_UIRouterSubsystem::PushInventoryWidget(FGameplayTag WidgetTag, ESOTS_UIInventoryRequestType RequestType, AActor* ContainerActor, bool bPauseOverride)
{
	if (!WidgetTag.IsValid())
	{
		return false;
	}

	F_SOTS_UIInventoryRequestPayload Payload;
	Payload.RequestType = RequestType;
	Payload.ContainerActor = ContainerActor;
	Payload.bPauseGameOverride = bPauseOverride;

	FInstancedStruct PayloadStruct;
	PayloadStruct.InitializeAs<F_SOTS_UIInventoryRequestPayload>(Payload);
	return PushWidgetById(WidgetTag, PayloadStruct);
}

bool USOTS_UIRouterSubsystem::PopWidgetById(FGameplayTag WidgetId)
{
	ESOTS_UILayer Layer;
	if (!IsWidgetActive(WidgetId, &Layer))
	{
		return false;
	}

	return PopFirstMatchingFromLayer(Layer, WidgetId);
}

bool USOTS_UIRouterSubsystem::IsWidgetActive(FGameplayTag WidgetId, ESOTS_UILayer* OutLayer) const
{
	for (const TPair<ESOTS_UILayer, FSOTS_LayerStack>& Pair : ActiveLayerStacks)
	{
		for (int32 Index = Pair.Value.Entries.Num() - 1; Index >= 0; --Index)
		{
			if (Pair.Value.Entries[Index].WidgetId == WidgetId)
			{
				if (OutLayer)
				{
					*OutLayer = Pair.Key;
				}
				return true;
			}
		}
	}

	return false;
}

bool USOTS_UIRouterSubsystem::PopFirstMatchingFromLayer(ESOTS_UILayer Layer, FGameplayTag WidgetId)
{
	if (FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
	{
		for (int32 Index = Stack->Entries.Num() - 1; Index >= 0; --Index)
		{
			if (Stack->Entries[Index].WidgetId == WidgetId)
			{
				const bool bWasExternal = Stack->Entries[Index].bExternalMenu;
				const FGameplayTag RemovedTag = Stack->Entries[Index].WidgetId;
				if (UUserWidget* Widget = Stack->Entries[Index].Widget.Get())
				{
					Widget->RemoveFromParent();
				}

				Stack->Entries.RemoveAt(Index);
				if (bWasExternal && ActiveExternalMenus.Remove(RemovedTag) > 0)
				{
					OnExternalMenuClosed.Broadcast(RemovedTag);
				}
				RefreshInputAndPauseState();
				return true;
			}
		}
	}

	return false;
}

bool USOTS_UIRouterSubsystem::IsExternalMenuTag(const FGameplayTag& MenuIdTag) const
{
	if (!MenuIdTag.IsValid())
	{
		return false;
	}

	static const FGameplayTag InvSPRootTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.InvSP")), false);
	if (InvSPRootTag.IsValid() && MenuIdTag.MatchesTag(InvSPRootTag))
	{
		return true;
	}

	static const FGameplayTag InventoryTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Menu.Inventory")), false);
	if (InventoryTag.IsValid() && MenuIdTag == InventoryTag)
	{
		return true;
	}

	static const FGameplayTag ContainerTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Menu.Container")), false);
	if (ContainerTag.IsValid() && MenuIdTag == ContainerTag)
	{
		return true;
	}

	if (!InvSPRootTag.IsValid())
	{
		return MenuIdTag.ToString().StartsWith(TEXT("SAS.UI.InvSP"));
	}

	return false;
}

bool USOTS_UIRouterSubsystem::RequestSaveGame(const FSOTS_ProfileId& ProfileId)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USOTS_ProfileSubsystem* ProfileSubsystem = GameInstance->GetSubsystem<USOTS_ProfileSubsystem>())
			{
				FText FailureReason;
				ProfileSubsystem->SetActiveProfile(ProfileId);
				const bool bSaved = ProfileSubsystem->RequestSaveCurrentProfile(FailureReason);
				if (!bSaved && !FailureReason.IsEmpty())
				{
					ShowNotification(FailureReason.ToString(), 2.0f, FGameplayTag());
				}
				else if (bSaved)
				{
					ShowNotification(TEXT("Game saved."), 2.0f, FGameplayTag());
				}
				return bSaved;
			}
		}
	}

	ShowNotification(TEXT("Save unavailable."), 2.0f, FGameplayTag());
	return false;
}

bool USOTS_UIRouterSubsystem::RequestCheckpointSave(const FSOTS_ProfileId& ProfileId)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USOTS_ProfileSubsystem* ProfileSubsystem = GameInstance->GetSubsystem<USOTS_ProfileSubsystem>())
			{
				FText FailureReason;
				const bool bSaved = ProfileSubsystem->RequestCheckpointSave(ProfileId, FailureReason);
				if (!bSaved && !FailureReason.IsEmpty())
				{
					ShowNotification(FailureReason.ToString(), 2.0f, FGameplayTag());
				}
				else if (bSaved)
				{
					ShowNotification(TEXT("Checkpoint saved."), 2.0f, FGameplayTag());
				}
				return bSaved;
			}
		}
	}

	ShowNotification(TEXT("Checkpoint save unavailable."), 2.0f, FGameplayTag());
	return false;
}
