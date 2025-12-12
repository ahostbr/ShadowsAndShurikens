#include "SOTS_UIRouterSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_InvSPAdapter.h"
#include "SOTS_HUDSubsystem.h"
#include "SOTS_NotificationSubsystem.h"
#include "SOTS_ProHUDAdapter.h"
#include "SOTS_WaypointSubsystem.h"
#include "SOTS_WidgetRegistryDataAsset.h"
#include "SOTS_UISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UIRouter, Log, All);

namespace
{
	static constexpr ESOTS_UILayer LayerPriority[] = {
		ESOTS_UILayer::Modal,
		ESOTS_UILayer::Overlay,
		ESOTS_UILayer::Debug,
		ESOTS_UILayer::HUD
	};
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
}

void USOTS_UIRouterSubsystem::Deinitialize()
{
	ActiveLayerStacks.Empty();
	CachedWidgets.Empty();
	ProHUDAdapter = nullptr;
	InvSPAdapter = nullptr;
	LoadedRegistry = nullptr;
	bGamePausedForUI = false;

	Super::Deinitialize();
}

bool USOTS_UIRouterSubsystem::PushWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload)
{
	return PushOrReplaceWidget(WidgetId, Payload, false);
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

bool USOTS_UIRouterSubsystem::PushWidgetByEntry(const FSOTS_WidgetRegistryEntry& Entry, FInstancedStruct Payload, bool bReplaceTop)
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

	UUserWidget* WidgetInstance = ResolveWidgetInstance(Entry);
	if (!WidgetInstance)
	{
		return false;
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

void USOTS_UIRouterSubsystem::RemoveTopFromLayer(ESOTS_UILayer Layer)
{
	if (FSOTS_LayerStack* Stack = ActiveLayerStacks.Find(Layer))
	{
		if (Stack->Entries.Num() > 0)
		{
			if (UUserWidget* Widget = Stack->Entries.Last().Widget.Get())
			{
				Widget->RemoveFromParent();
			}

			Stack->Entries.RemoveAt(Stack->Entries.Num() - 1);
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
	case ESOTS_UILayer::HUD:
		return 0;
	case ESOTS_UILayer::Overlay:
		return 100;
	case ESOTS_UILayer::Debug:
		return 1000;
	case ESOTS_UILayer::Modal:
		return 2000;
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

	if (!InvSPAdapter && InvSPAdapterClass)
	{
		InvSPAdapter = NewObject<USOTS_InvSPAdapter>(GetGameInstance(), InvSPAdapterClass);
	}
}
