#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputTriggers.h"
#include "EnhancedInputComponent.h"
#include "SOTS_InputBindingTypes.h"
#include "SOTS_InputBufferTypes.h"
#include "SOTS_InputDeviceTypes.h"
#include "SOTS_InputGateTypes.h"
#include "SOTS_InputLayerDataAsset.h"
#include "SOTS_InputRouterComponent.generated.h"

class UInputMappingContext;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class APlayerController;
class USOTS_InputHandler;
class USOTS_InputLayerDataAsset;
class USOTS_InputBufferComponent;
struct FInputActionInstance;
struct FInputBindingHandle;

USTRUCT()
struct FSOTS_ActiveInputLayer
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<const USOTS_InputLayerDataAsset> LayerAsset = nullptr;

    UPROPERTY()
    int32 Priority = 0;

    UPROPERTY()
    bool bBlocksLowerPriorityLayers = false;

    UPROPERTY()
    ESOTS_InputLayerConsumePolicy ConsumePolicy = ESOTS_InputLayerConsumePolicy::ConsumeHandled;

    UPROPERTY()
    TArray<TObjectPtr<const UInputMappingContext>> AppliedContexts;

    UPROPERTY()
    TArray<TObjectPtr<USOTS_InputHandler>> RuntimeHandlers;
};

USTRUCT()
struct FSOTS_ActiveInputLayerSummary
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag LayerTag;

    UPROPERTY()
    int32 Priority = 0;

    UPROPERTY()
    bool bBlocksLowerPriorityLayers = false;

    UPROPERTY()
    ESOTS_InputLayerConsumePolicy ConsumePolicy = ESOTS_InputLayerConsumePolicy::ConsumeHandled;

    UPROPERTY()
    int32 MappingContextCount = 0;

    UPROPERTY()
    int32 HandlerCount = 0;
};

USTRUCT()
struct FSOTS_DispatchLayerInfo
{
    GENERATED_BODY()

    int32 StartIndex = 0;
    int32 EndIndex = 0;
    ESOTS_InputLayerConsumePolicy ConsumePolicy = ESOTS_InputLayerConsumePolicy::ConsumeHandled;
    bool bBlocksLowerPriorityLayers = false;
};

USTRUCT()
struct FSOTS_DispatchEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<USOTS_InputHandler> Handler = nullptr;

    UPROPERTY()
    int32 LayerIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FSOTS_InputIntentEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Intent")
    FGameplayTag IntentTag;

    UPROPERTY(BlueprintReadOnly, Category = "Intent")
    ETriggerEvent TriggerEvent = ETriggerEvent::None;

    UPROPERTY(BlueprintReadOnly, Category = "Intent")
    FInputActionValue Value;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnInputIntent, const FSOTS_InputIntentEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnInputDeviceChanged, ESOTS_InputDevice, NewDevice);

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_INPUT_API USOTS_InputRouterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_InputRouterComponent(const FObjectInitializer& ObjectInitializer);

    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void PushLayerByTag(FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void PushLayer(const USOTS_InputLayerDataAsset* Layer);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void PopLayerByTag(FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void ClearAllLayers();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    bool IsLayerActive(FGameplayTag LayerTag) const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void OpenInputBuffer(FGameplayTag Channel);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void CloseInputBuffer(FGameplayTag Channel, bool bFlush);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    USOTS_InputBufferComponent* GetOrFindBufferComponent() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void NotifyKeyInput(const FKey& Key);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void ReportInputDeviceFromKey(const FKey& Key);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void BroadcastIntent(FGameplayTag IntentTag, ETriggerEvent TriggerEvent, FInputActionValue Value);

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    ESOTS_InputDevice GetLastDevice() const { return LastDevice; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    bool IsGamepadActive() const { return LastDevice == ESOTS_InputDevice::Gamepad; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    void GetActiveLayerTags(TArray<FGameplayTag>& OutTags) const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    void GetOpenBufferChannels(TArray<FGameplayTag>& OutChannels) const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    FGameplayTag GetTopBufferChannel() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void RefreshRouter();

    void GetActiveLayerSummaries(TArray<FSOTS_ActiveInputLayerSummary>& OutSummaries) const;
    void GetBindingSnapshot(TArray<FSOTS_InputBindingKey>& OutBindings) const;

    void DispatchBufferedEvent(const FSOTS_BufferedInputEvent& Evt);

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Input")
    FSOTS_OnInputIntent OnInputIntent;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Input")
    FSOTS_OnInputDeviceChanged OnInputDeviceChanged;

private:
    void InitializeRouter();
    void RebuildBindings();
    void ScheduleRefreshNextTick();
    void ClearRouterOwnedBindings(UEnhancedInputComponent* TargetComponent);
    void StartAutoRefreshTimer();
    void StopAutoRefreshTimer();
    void AutoRefreshCheck();

    void RouteInput(const UInputAction* Action, ETriggerEvent TriggerEvent, const FInputActionValue& Value, const FInputActionInstance* Instance, bool bFromBuffer);
    void BindActionEvent(const FSOTS_InputBindingKey& BindingKey);

    bool EvaluateGates(bool bForBuffering) const;

    void OnActionStarted(const FInputActionInstance& Instance);
    void OnActionTriggered(const FInputActionInstance& Instance);
    void OnActionCompleted(const FInputActionInstance& Instance);
    void OnActionCanceled(const FInputActionInstance& Instance);
    void OnActionOngoing(const FInputActionInstance& Instance);

    bool TryResolveOwnerAndSubsystems();

private:
    UPROPERTY()
    TArray<FSOTS_ActiveInputLayer> ActiveLayers;

    UPROPERTY()
    TWeakObjectPtr<APlayerController> OwningPC;

    UPROPERTY()
    TWeakObjectPtr<UEnhancedInputComponent> EnhancedInputComp;

    UPROPERTY()
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;

    UPROPERTY()
    TWeakObjectPtr<USOTS_InputBufferComponent> BufferComp;

    TArray<FInputBindingHandle> RouterOwnedBindings;

    UPROPERTY()
    TArray<FSOTS_InputBindingKey> CachedBindingOrder;

    UPROPERTY()
    TArray<FSOTS_DispatchEntry> DispatchEntries;

    UPROPERTY()
    TArray<FSOTS_DispatchLayerInfo> DispatchLayers;

    UPROPERTY(VisibleAnywhere, Category = "SOTS|Input")
    ESOTS_InputDevice LastDevice = ESOTS_InputDevice::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input", meta = (AllowPrivateAccess = "true"))
    bool bEnableTagGates = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input", meta = (AllowPrivateAccess = "true"))
    TArray<FSOTS_InputGateRule> GateRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Debug", meta = (AllowPrivateAccess = "true"))
    bool bDebugLogRouterState = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Debug", meta = (AllowPrivateAccess = "true"))
    bool bDebugLogBindings = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input", meta = (AllowPrivateAccess = "true"))
    bool bEnableAutoRefresh = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input", meta = (AllowPrivateAccess = "true"))
    float AutoRefreshIntervalSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Debug", meta = (AllowPrivateAccess = "true"))
    float DebugLogIntervalSeconds = 2.0f;

    float LastDebugLogTime = 0.f;

    TWeakObjectPtr<UEnhancedInputComponent> BoundInputComponent;
    TWeakObjectPtr<APlayerController> LastObservedPC;
    TWeakObjectPtr<UEnhancedInputComponent> LastObservedInputComponent;

    FTimerHandle AutoRefreshTimerHandle;
};
