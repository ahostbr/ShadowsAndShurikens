#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "SOTS_WidgetRegistryTypes.h"
#include "SOTS_UIPayloadTypes.h"
#include "SOTS_UIModalResultTypes.h"
#include "SOTS_InteractionViewTypes.h"
#include "SOTS_UIRouterSubsystem.generated.h"

class USOTS_WidgetRegistryDataAsset;
class USOTS_ProHUDAdapter;
class USOTS_InvSPAdapter;
class USOTS_InteractionEssentialsAdapter;
class UUserWidget;
class AActor;
struct F_SOTS_UIConfirmDialogPayload;

USTRUCT()
struct FSOTS_ActiveWidgetEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag WidgetId;

	UPROPERTY()
	TObjectPtr<UUserWidget> Widget = nullptr;

	UPROPERTY()
	ESOTS_UILayer Layer = ESOTS_UILayer::Overlay;

	UPROPERTY()
	ESOTS_UIInputPolicy InputPolicy = ESOTS_UIInputPolicy::GameOnly;

	UPROPERTY()
	ESOTS_UICachePolicy CachePolicy = ESOTS_UICachePolicy::Recreate;

	UPROPERTY()
	FInstancedStruct Payload;

	UPROPERTY()
	bool bPauseGame = false;

	UPROPERTY()
	bool bCloseOnEscape = true;
};

USTRUCT()
struct FSOTS_LayerStack
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSOTS_ActiveWidgetEntry> Entries;
};

/**
 * Central UI router/stack driver. All UI requests flow through here.
 */
UCLASS(Config=Game)
class SOTS_UI_API USOTS_UIRouterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USOTS_UIRouterSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool PushWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	UUserWidget* CreateWidgetById(FGameplayTag WidgetId);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool PushWidgetByIdWithInstance(FGameplayTag WidgetId, UUserWidget* Widget, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool ReplaceTopWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool PopWidget(ESOTS_UILayer OptionalLayerFilter, bool bUseLayerFilter = false);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void PopAllModals();

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void SetObjectiveText(const FString& InText);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void ShowNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Notifications")
	void PushNotification_SOTS(const F_SOTS_UINotificationPayload& Payload);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Notifications")
	void ShowPickupNotification(const FText& ItemName, int32 Quantity, float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Notifications")
	void ShowFirstTimePickupNotification(const FText& ItemName, float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWaypoint_Actor(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWaypoint_Location(FVector Location, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void RemoveWaypoint(FGuid Id);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWorldMarker_Actor(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWorldMarker_Location(FVector Location, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void RemoveWorldMarker(FGuid Id);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Inventory")
	bool OpenInventoryMenu();

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Inventory")
	bool CloseInventoryMenu();

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Inventory")
	bool ToggleInventoryMenu();

	// Expose widget pop by id for external flows (e.g., shader warmup teardown).
	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool PopWidgetById(FGameplayTag WidgetId);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Inventory")
	bool OpenItemContainerMenu(AActor* ContainerActor);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Inventory")
	bool CloseItemContainerMenu();

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void EnsureGameplayHUDReady();

	// Modal result channel
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnModalResult, const F_SOTS_UIModalResult&, Result);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnReturnToMainMenuRequested);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Modal")
	FGuid MakeRequestId() const { return FGuid::NewGuid(); }

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Modal")
	void SubmitModalResult(const F_SOTS_UIModalResult& Result);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Modal")
	bool ShowConfirmDialog(const F_SOTS_UIConfirmDialogPayload& Payload);

	// Interaction intents (payload via FInstancedStruct)
	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Interaction")
	bool HandleInteractionIntent(FGameplayTag IntentTag, FInstancedStruct Payload);

	// System flows
	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|System", meta = (AutoCreateRefTerm = "MessageOverride"))
	bool RequestReturnToMainMenu(const FText& MessageOverride);

protected:
	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSoftObjectPtr<USOTS_WidgetRegistryDataAsset> WidgetRegistry;

	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSubclassOf<USOTS_ProHUDAdapter> ProHUDAdapterClass;

	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSubclassOf<USOTS_InvSPAdapter> InvSPAdapterClass;

	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSubclassOf<USOTS_InteractionEssentialsAdapter> InteractionAdapterClass;

private:
	bool PushOrReplaceWidget(FGameplayTag WidgetId, FInstancedStruct Payload, bool bReplaceTop);
	bool PushWidgetByEntry(const FSOTS_WidgetRegistryEntry& Entry, FInstancedStruct Payload, bool bReplaceTop, UUserWidget* WidgetOverride = nullptr);
	bool ResolveEntry(FGameplayTag WidgetId, FSOTS_WidgetRegistryEntry& OutEntry);
	void RefreshInputAndPauseState();
	void ApplyInputPolicy(const FSOTS_ActiveWidgetEntry* TopEntry);
	void RefreshInputLayers(const FSOTS_ActiveWidgetEntry* TopEntry);
	void UpdateUINavLayerState(bool bShouldEnable);
	bool ShouldEnableUINavLayer(const FSOTS_ActiveWidgetEntry* TopEntry) const;
	void RemoveTopFromLayer(ESOTS_UILayer Layer);
	FSOTS_ActiveWidgetEntry* GetTopEntry(ESOTS_UILayer Layer);
	const FSOTS_ActiveWidgetEntry* GetTopEntry(ESOTS_UILayer Layer) const;
	int32 GetLayerBaseZOrder(ESOTS_UILayer Layer) const;
	UUserWidget* ResolveWidgetInstance(const FSOTS_WidgetRegistryEntry& Entry);
	void CacheWidgetIfNeeded(const FSOTS_WidgetRegistryEntry& Entry, UUserWidget* Widget);
	APlayerController* GetPlayerController() const;
	void EnsureAdapters();
	void EnsureInteractionAdapter();
	bool ExecuteSystemAction(FGameplayTag ActionTag);
	FGameplayTag GetDefaultConfirmDialogTag() const;
	FGameplayTag ResolveFirstValidTag(const TArray<FName>& Names) const;
	bool PushNotificationWidget(FGameplayTag WidgetTag, const F_SOTS_UINotificationPayload& Payload);
	bool PushInventoryWidget(FGameplayTag WidgetTag, ESOTS_UIInventoryRequestType RequestType, AActor* ContainerActor = nullptr, bool bPauseOverride = false);
	bool IsWidgetActive(FGameplayTag WidgetId, ESOTS_UILayer* OutLayer = nullptr) const;
	bool PopFirstMatchingFromLayer(ESOTS_UILayer Layer, FGameplayTag WidgetId);
	bool DispatchInteractionIntent(FGameplayTag IntentTag, const FInstancedStruct& Payload);
	bool DispatchInteractionMarkerIntent(FGameplayTag IntentTag, const FInstancedStruct& Payload);
	bool HandleReturnToMainMenuAction();
	F_SOTS_UIConfirmDialogPayload BuildReturnToMainMenuPayload(const FText& MessageOverride) const;

private:
	UPROPERTY()
	TObjectPtr<USOTS_WidgetRegistryDataAsset> LoadedRegistry;

	UPROPERTY()
	TObjectPtr<USOTS_ProHUDAdapter> ProHUDAdapter;

	UPROPERTY()
	TObjectPtr<USOTS_InvSPAdapter> InvSPAdapter;

	UPROPERTY()
	TObjectPtr<USOTS_InteractionEssentialsAdapter> InteractionAdapter;

	UPROPERTY()
	TMap<ESOTS_UILayer, FSOTS_LayerStack> ActiveLayerStacks;

	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UUserWidget>> CachedWidgets;

public:
	UPROPERTY(BlueprintAssignable, Category = "SOTS|UI|Modal")
	FSOTS_OnModalResult OnModalResult;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|UI|System")
	FSOTS_OnReturnToMainMenuRequested OnReturnToMainMenuRequested;

private:
	bool bGamePausedForUI = false;
	bool bUINavLayerActive = false;
};
