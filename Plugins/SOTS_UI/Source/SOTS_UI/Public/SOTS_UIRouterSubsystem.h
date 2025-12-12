#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "SOTS_WidgetRegistryTypes.h"
#include "SOTS_UIRouterSubsystem.generated.h"

class USOTS_WidgetRegistryDataAsset;
class USOTS_ProHUDAdapter;
class USOTS_InvSPAdapter;
class UUserWidget;

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
	bool ReplaceTopWidgetById(FGameplayTag WidgetId, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	bool PopWidget(ESOTS_UILayer OptionalLayerFilter, bool bUseLayerFilter = false);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void PopAllModals();

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void SetObjectiveText(const FString& InText);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void ShowNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWaypoint_Actor(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	FGuid AddOrUpdateWaypoint_Location(FVector Location, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UI")
	void RemoveWaypoint(FGuid Id);

protected:
	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSoftObjectPtr<USOTS_WidgetRegistryDataAsset> WidgetRegistry;

	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSubclassOf<USOTS_ProHUDAdapter> ProHUDAdapterClass;

	UPROPERTY(EditAnywhere, Config, Category = "SOTS|UI")
	TSubclassOf<USOTS_InvSPAdapter> InvSPAdapterClass;

private:
	bool PushOrReplaceWidget(FGameplayTag WidgetId, FInstancedStruct Payload, bool bReplaceTop);
	bool PushWidgetByEntry(const FSOTS_WidgetRegistryEntry& Entry, FInstancedStruct Payload, bool bReplaceTop);
	bool ResolveEntry(FGameplayTag WidgetId, FSOTS_WidgetRegistryEntry& OutEntry);
	void RefreshInputAndPauseState();
	void ApplyInputPolicy(const FSOTS_ActiveWidgetEntry* TopEntry);
	void RemoveTopFromLayer(ESOTS_UILayer Layer);
	FSOTS_ActiveWidgetEntry* GetTopEntry(ESOTS_UILayer Layer);
	const FSOTS_ActiveWidgetEntry* GetTopEntry(ESOTS_UILayer Layer) const;
	int32 GetLayerBaseZOrder(ESOTS_UILayer Layer) const;
	UUserWidget* ResolveWidgetInstance(const FSOTS_WidgetRegistryEntry& Entry);
	void CacheWidgetIfNeeded(const FSOTS_WidgetRegistryEntry& Entry, UUserWidget* Widget);
	APlayerController* GetPlayerController() const;
	void EnsureAdapters();

private:
	UPROPERTY()
	TObjectPtr<USOTS_WidgetRegistryDataAsset> LoadedRegistry;

	UPROPERTY()
	TObjectPtr<USOTS_ProHUDAdapter> ProHUDAdapter;

	UPROPERTY()
	TObjectPtr<USOTS_InvSPAdapter> InvSPAdapter;

	UPROPERTY()
	TMap<ESOTS_UILayer, FSOTS_LayerStack> ActiveLayerStacks;

	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UUserWidget>> CachedWidgets;

	bool bGamePausedForUI = false;
};
