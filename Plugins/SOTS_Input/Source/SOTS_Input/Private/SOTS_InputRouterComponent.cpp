#include "SOTS_InputRouterComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "InputAction.h"
#include "InputActionInstance.h"
#include "SOTS_InputBufferComponent.h"
#include "SOTS_InputDeviceLibrary.h"
#include "SOTS_InputHandler.h"
#include "SOTS_InputLayerDataAsset.h"
#include "SOTS_InputLayerRegistrySubsystem.h"
#include "Subsystems/SubsystemBlueprintLibrary.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

USOTS_InputRouterComponent::USOTS_InputRouterComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USOTS_InputRouterComponent::OnRegister()
{
    Super::OnRegister();
    SetComponentTickEnabled(bDebugLogRouterState);
    StartAutoRefreshTimer();
    ScheduleRefreshNextTick();
}

void USOTS_InputRouterComponent::BeginPlay()
{
    Super::BeginPlay();
    StartAutoRefreshTimer();
    ScheduleRefreshNextTick();
}

void USOTS_InputRouterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        for (USOTS_InputHandler* Handler : Layer.RuntimeHandlers)
        {
            if (Handler)
            {
                Handler->OnDeactivated(this);
            }
        }
    }

    ClearRouterOwnedBindings(BoundInputComponent.Get());

    if (InputSubsystem.IsValid())
    {
        InputSubsystem->ClearAllMappings();
    }

    if (USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent())
    {
        Buffer->ResetAll();
    }

    ActiveLayers.Reset();
    CachedBindingOrder.Reset();
    BoundInputComponent.Reset();

    StopAutoRefreshTimer();

    Super::EndPlay(EndPlayReason);
}

void USOTS_InputRouterComponent::InitializeRouter()
{
    SetComponentTickEnabled(bDebugLogRouterState);
    StartAutoRefreshTimer();
    RefreshRouter();
}

bool USOTS_InputRouterComponent::TryResolveOwnerAndSubsystems()
{
    if (OwningPC.IsValid() && EnhancedInputComp.IsValid() && InputSubsystem.IsValid())
    {
        return true;
    }

    if (AActor* OwnerActor = GetOwner())
    {
        OwningPC = Cast<APlayerController>(OwnerActor);
        if (!OwningPC.IsValid())
        {
            OwningPC = Cast<APlayerController>(OwnerActor->GetInstigatorController());
        }

        EnhancedInputComp = OwnerActor->FindComponentByClass<UEnhancedInputComponent>();
        if (!EnhancedInputComp.IsValid() && OwningPC.IsValid())
        {
            EnhancedInputComp = Cast<UEnhancedInputComponent>(OwningPC->InputComponent);
        }

        if (const ULocalPlayer* LP = OwningPC.IsValid() ? OwningPC->GetLocalPlayer() : nullptr)
        {
            InputSubsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        }
    }

    BufferComp = GetOwner() ? GetOwner()->FindComponentByClass<USOTS_InputBufferComponent>() : nullptr;
    return OwningPC.IsValid() && EnhancedInputComp.IsValid() && InputSubsystem.IsValid();
}

void USOTS_InputRouterComponent::ScheduleRefreshNextTick()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(this, &USOTS_InputRouterComponent::RefreshRouter);
    }
}

void USOTS_InputRouterComponent::ClearRouterOwnedBindings(UEnhancedInputComponent* TargetComponent)
{
    if (!TargetComponent)
    {
        RouterOwnedBindingIndices.Reset();
        return;
    }

    for (int32 Index = RouterOwnedBindingIndices.Num() - 1; Index >= 0; --Index)
    {
        if (RouterOwnedBindingIndices.IsValidIndex(Index))
        {
            TargetComponent->RemoveBinding(RouterOwnedBindingIndices[Index]);
        }
    }

    RouterOwnedBindingIndices.Reset();
}

void USOTS_InputRouterComponent::StartAutoRefreshTimer()
{
    if (!bEnableAutoRefresh)
    {
        StopAutoRefreshTimer();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (!World->GetTimerManager().IsTimerActive(AutoRefreshTimerHandle))
        {
            World->GetTimerManager().SetTimer(AutoRefreshTimerHandle, this, &USOTS_InputRouterComponent::AutoRefreshCheck, AutoRefreshIntervalSeconds, true);
        }
    }
}

void USOTS_InputRouterComponent::StopAutoRefreshTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
    }
}

void USOTS_InputRouterComponent::AutoRefreshCheck()
{
    const TWeakObjectPtr<APlayerController> PreviousPC = LastObservedPC;
    const TWeakObjectPtr<UEnhancedInputComponent> PreviousInputComp = LastObservedInputComponent;

    TryResolveOwnerAndSubsystems();

    const bool bPCChanged = PreviousPC.Get() != OwningPC.Get();
    const bool bInputChanged = PreviousInputComp.Get() != EnhancedInputComp.Get();

    LastObservedPC = OwningPC;
    LastObservedInputComponent = EnhancedInputComp;

    if (bPCChanged || bInputChanged)
    {
        RefreshRouter();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebugLogRouterState)
        {
            UE_LOG(LogTemp, Log, TEXT("SOTS_Input Router auto-refresh: PCChanged=%s InputChanged=%s"), bPCChanged ? TEXT("true") : TEXT("false"), bInputChanged ? TEXT("true") : TEXT("false"));
        }
#endif
    }
}

void USOTS_InputRouterComponent::PushLayer(const USOTS_InputLayerDataAsset* Layer)
{
    if (!Layer || !TryResolveOwnerAndSubsystems())
    {
        return;
    }

    FSOTS_ActiveInputLayer Active;
    Active.LayerAsset = Layer;
    Active.Priority = Layer->Priority;
    Active.bBlocksLowerPriorityLayers = Layer->bBlocksLowerPriorityLayers;
    Active.ConsumePolicy = Layer->ConsumePolicy;
    Active.AppliedContexts = Layer->MappingContexts;

    for (USOTS_InputHandler* TemplateHandler : Layer->HandlerTemplates)
    {
        if (!TemplateHandler)
        {
            continue;
        }

        USOTS_InputHandler* HandlerInstance = DuplicateObject(TemplateHandler, this);
        if (HandlerInstance)
        {
            HandlerInstance->OnActivated(this);
            Active.RuntimeHandlers.Add(HandlerInstance);
        }
    }

    ActiveLayers.Add(Active);
    ActiveLayers.Sort([](const FSOTS_ActiveInputLayer& A, const FSOTS_ActiveInputLayer& B)
    {
        return A.Priority > B.Priority;
    });

    RebuildBindings();
}

void USOTS_InputRouterComponent::PushLayerByTag(FGameplayTag LayerTag)
{
    if (!LayerTag.IsValid())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USOTS_InputLayerRegistrySubsystem* Registry = GI->GetSubsystem<USOTS_InputLayerRegistrySubsystem>())
            {
                USOTS_InputLayerDataAsset* LayerAsset = nullptr;
                if (Registry->TryGetLayerAsset(LayerTag, LayerAsset) && LayerAsset)
                {
                    PushLayer(LayerAsset);
                }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
                else if (Registry->IsAsyncLoadsEnabled())
                {
                    UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input: Layer %s not yet loaded (async mode)."), *LayerTag.ToString());
                }
#endif
            }
        }
    }
}

void USOTS_InputRouterComponent::PopLayerByTag(FGameplayTag LayerTag)
{
    if (!LayerTag.IsValid())
    {
        return;
    }

    for (int32 Index = ActiveLayers.Num() - 1; Index >= 0; --Index)
    {
        if (ActiveLayers[Index].LayerAsset && ActiveLayers[Index].LayerAsset->LayerTag.MatchesTagExact(LayerTag))
        {
            for (USOTS_InputHandler* Handler : ActiveLayers[Index].RuntimeHandlers)
            {
                if (Handler)
                {
                    Handler->OnDeactivated(this);
                }
            }

            ActiveLayers.RemoveAt(Index);
            RebuildBindings();
            return;
        }
    }
}

void USOTS_InputRouterComponent::ClearAllLayers()
{
    for (FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        for (USOTS_InputHandler* Handler : Layer.RuntimeHandlers)
        {
            if (Handler)
            {
                Handler->OnDeactivated(this);
            }
        }
    }

    ActiveLayers.Reset();
    RebuildBindings();
}

bool USOTS_InputRouterComponent::IsLayerActive(FGameplayTag LayerTag) const
{
    for (const FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        if (Layer.LayerAsset && Layer.LayerAsset->LayerTag.MatchesTagExact(LayerTag))
        {
            return true;
        }
    }
    return false;
}

void USOTS_InputRouterComponent::OpenInputBuffer(FGameplayTag Channel)
{
    if (USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent())
    {
        Buffer->OpenChannel(Channel);
    }
}

void USOTS_InputRouterComponent::CloseInputBuffer(FGameplayTag Channel, bool bFlush)
{
    if (USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent())
    {
        Buffer->CloseChannel(Channel, bFlush, this);
    }
}

USOTS_InputBufferComponent* USOTS_InputRouterComponent::GetOrFindBufferComponent() const
{
    if (BufferComp.IsValid())
    {
        return BufferComp.Get();
    }

    if (AActor* OwnerActor = GetOwner())
    {
        return OwnerActor->FindComponentByClass<USOTS_InputBufferComponent>();
    }
    return nullptr;
}

void USOTS_InputRouterComponent::NotifyKeyInput(const FKey& Key)
{
    const ESOTS_InputDevice NewDevice = USOTS_InputDeviceLibrary::GetDeviceFromKey(Key);
    if (NewDevice != LastDevice)
    {
        LastDevice = NewDevice;
        OnInputDeviceChanged.Broadcast(NewDevice);
    }
}

void USOTS_InputRouterComponent::BroadcastIntent(FGameplayTag IntentTag, ETriggerEvent TriggerEvent, FInputActionValue Value)
{
    FSOTS_InputIntentEvent Event;
    Event.IntentTag = IntentTag;
    Event.TriggerEvent = TriggerEvent;
    Event.Value = Value;
    OnInputIntent.Broadcast(Event);
}

bool USOTS_InputRouterComponent::EvaluateGates(bool bForBuffering) const
{
    if (!bEnableTagGates || GateRules.Num() == 0)
    {
        return true;
    }

    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return true;
    }

    const UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    if (!GI)
    {
        return true;
    }

    UClass* TagManagerClass = FindObject<UClass>(ANY_PACKAGE, TEXT("USOTS_GameplayTagManagerSubsystem"));
    if (!TagManagerClass)
    {
        return true;
    }

    UObject* Subsys = USubsystemBlueprintLibrary::GetGameInstanceSubsystem(GI, TagManagerClass);
    if (!Subsys)
    {
        return true;
    }

    static const FName ActorHasTagName(TEXT("ActorHasTag"));
    UFunction* HasTagFunc = Subsys->FindFunction(ActorHasTagName);
    if (!HasTagFunc)
    {
        return true;
    }

    for (const FSOTS_InputGateRule& Rule : GateRules)
    {
        if (!Rule.GateTag.IsValid())
        {
            continue;
        }

        if (bForBuffering && !Rule.bAffectsBuffering)
        {
            continue;
        }

        if (!bForBuffering && !Rule.bAffectsLiveInput)
        {
            continue;
        }

        struct
        {
            const AActor* Actor;
            FGameplayTag Tag;
            bool ReturnValue;
        } Params{OwnerActor, Rule.GateTag, false};

        Subsys->ProcessEvent(HasTagFunc, &Params);

        const bool bHasTag = Params.ReturnValue;
        const bool bPasses = Rule.bRequirePresent ? bHasTag : !bHasTag;
        if (!bPasses)
        {
            return false;
        }
    }

    return true;
}

void USOTS_InputRouterComponent::DispatchBufferedEvent(const FSOTS_BufferedInputEvent& Evt)
{
    const UInputAction* Action = Evt.Action;
    if (!Action)
    {
        return;
    }

    RouteInput(Action, Evt.TriggerEvent, Evt.Value, nullptr, true);
}

void USOTS_InputRouterComponent::RebuildBindings()
{
    if (!TryResolveOwnerAndSubsystems())
    {
        return;
    }

    UEnhancedInputComponent* CurrentInputComp = EnhancedInputComp.Get();
    UEnhancedInputLocalPlayerSubsystem* LocalInputSubsystem = InputSubsystem.Get();

    if (!CurrentInputComp || !LocalInputSubsystem)
    {
        ClearRouterOwnedBindings(BoundInputComponent.Get());
        BoundInputComponent.Reset();
        CachedBindingOrder.Reset();
        return;
    }

    ClearRouterOwnedBindings(BoundInputComponent.Get());
    RouterOwnedBindingIndices.Reset();
    BoundInputComponent = CurrentInputComp;

    LocalInputSubsystem->ClearAllMappings();

    DispatchEntries.Reset();
    DispatchLayers.Reset();

    TArray<const FSOTS_ActiveInputLayer*> LayersToApply;
    for (const FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        LayersToApply.Add(&Layer);
        if (Layer.bBlocksLowerPriorityLayers)
        {
            break;
        }
    }

    for (const FSOTS_ActiveInputLayer* Layer : LayersToApply)
    {
        if (!Layer)
        {
            continue;
        }

        for (const UInputMappingContext* Context : Layer->AppliedContexts)
        {
            if (Context)
            {
                LocalInputSubsystem->AddMappingContext(Context, Layer->Priority);
            }
        }
    }

    int32 LayerCounter = 0;
    for (const FSOTS_ActiveInputLayer* Layer : LayersToApply)
    {
        FSOTS_DispatchLayerInfo LayerInfo;
        LayerInfo.StartIndex = DispatchEntries.Num();
        LayerInfo.ConsumePolicy = Layer ? Layer->ConsumePolicy : ESOTS_InputLayerConsumePolicy::ConsumeHandled;
        LayerInfo.bBlocksLowerPriorityLayers = Layer ? Layer->bBlocksLowerPriorityLayers : false;

        if (Layer)
        {
            for (USOTS_InputHandler* Handler : Layer->RuntimeHandlers)
            {
                if (!Handler)
                {
                    continue;
                }

                FSOTS_DispatchEntry Entry;
                Entry.Handler = Handler;
                Entry.LayerIndex = LayerCounter;
                DispatchEntries.Add(Entry);
            }
        }

        LayerInfo.EndIndex = DispatchEntries.Num();
        DispatchLayers.Add(LayerInfo);
        ++LayerCounter;
    }

    TSet<FSOTS_InputBindingKey> UniqueBindings;
    for (const FSOTS_ActiveInputLayer* Layer : LayersToApply)
    {
        if (Layer && Layer->LayerAsset)
        {
            TMap<const UInputAction*, TSet<ETriggerEvent>> LayerBindings;
            Layer->LayerAsset->GetAllBindings(LayerBindings);
            for (const TPair<const UInputAction*, TSet<ETriggerEvent>>& Pair : LayerBindings)
            {
                for (const ETriggerEvent Event : Pair.Value)
                {
                    if (Event == ETriggerEvent::None)
                    {
                        continue;
                    }

                    FSOTS_InputBindingKey Key;
                    Key.Action = Pair.Key;
                    Key.TriggerEvent = Event;
                    UniqueBindings.Add(Key);
                }
            }
        }
    }

    CachedBindingOrder = UniqueBindings.Array();
    CachedBindingOrder.Sort([](const FSOTS_InputBindingKey& A, const FSOTS_InputBindingKey& B)
    {
        const FName NameA = A.Action ? A.Action->GetFName() : NAME_None;
        const FName NameB = B.Action ? B.Action->GetFName() : NAME_None;
        if (NameA != NameB)
        {
            return NameA.LexicalLess(NameB);
        }

        return static_cast<uint8>(A.TriggerEvent) < static_cast<uint8>(B.TriggerEvent);
    });

    for (const FSOTS_InputBindingKey& BindingKey : CachedBindingOrder)
    {
        BindActionEvent(BindingKey);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogRouterState || bDebugLogBindings)
    {
        TArray<FString> BindingStrings;
        BindingStrings.Reserve(CachedBindingOrder.Num());
        for (const FSOTS_InputBindingKey& Binding : CachedBindingOrder)
        {
            BindingStrings.Add(Binding.ToString());
        }
        UE_LOG(LogTemp, Log, TEXT("SOTS_Input Router Bindings (%d): %s"), CachedBindingOrder.Num(), *FString::Join(BindingStrings, TEXT(",")));
    }
#endif
}

void USOTS_InputRouterComponent::RouteInput(const UInputAction* Action, ETriggerEvent TriggerEvent, const FInputActionValue& Value, const FInputActionInstance* Instance, bool bFromBuffer)
{
    if (!Action)
    {
        return;
    }

    const bool bGatesAllowLive = EvaluateGates(false);
    const bool bGatesAllowBuffer = EvaluateGates(true);

    if (!bFromBuffer && !bGatesAllowLive && !bGatesAllowBuffer)
    {
        return;
    }

    USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent();
    FGameplayTag OpenChannel;
    const bool bHasOpenChannel = Buffer && Buffer->GetTopOpenChannel(OpenChannel);

    for (int32 LayerIdx = 0; LayerIdx < DispatchLayers.Num(); ++LayerIdx)
    {
        const FSOTS_DispatchLayerInfo& LayerInfo = DispatchLayers[LayerIdx];
        bool bLayerHandled = false;

        for (int32 EntryIndex = LayerInfo.StartIndex; EntryIndex < LayerInfo.EndIndex; ++EntryIndex)
        {
            const FSOTS_DispatchEntry& Entry = DispatchEntries.IsValidIndex(EntryIndex) ? DispatchEntries[EntryIndex] : FSOTS_DispatchEntry();
            USOTS_InputHandler* Handler = Entry.Handler.Get();
            if (!Handler || !Handler->CanHandle(Action, TriggerEvent))
            {
                continue;
            }

            if (!bFromBuffer && bHasOpenChannel && Handler->ShouldBuffer(Action, TriggerEvent, OpenChannel))
            {
                if (!bGatesAllowBuffer)
                {
                    return;
                }

                FSOTS_BufferedInputEvent Evt;
                Evt.Action = Action;
                Evt.TriggerEvent = TriggerEvent;
                Evt.Value = Value;
                Evt.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
                Evt.Channel = Handler->BufferChannel;
                Buffer->BufferEvent(Evt);
                continue;
            }

            if (bFromBuffer)
            {
                if (bGatesAllowLive)
                {
                    Handler->HandleBufferedInput(this, Action, TriggerEvent, Value);
                    bLayerHandled = true;
                }
            }
            else
            {
                if (!bGatesAllowLive)
                {
                    return;
                }

                if (Instance)
                {
                    Handler->HandleInput(this, *Instance, TriggerEvent);
                }
                bLayerHandled = true;
            }
        }

        if (LayerInfo.ConsumePolicy == ESOTS_InputLayerConsumePolicy::ConsumeAllMatches)
        {
            break;
        }

        if (bLayerHandled)
        {
            if (LayerInfo.ConsumePolicy == ESOTS_InputLayerConsumePolicy::ConsumeHandled)
            {
                break;
            }

            if (LayerInfo.bBlocksLowerPriorityLayers)
            {
                break;
            }
        }
        else if (LayerInfo.ConsumePolicy == ESOTS_InputLayerConsumePolicy::ConsumeAllMatches)
        {
            break;
        }
    }
}

void USOTS_InputRouterComponent::BindActionEvent(const FSOTS_InputBindingKey& BindingKey)
{
    if (!BindingKey.Action || !BoundInputComponent.IsValid())
    {
        return;
    }

    UEnhancedInputComponent* InputComp = BoundInputComponent.Get();

    using FHandlerSignature = void (USOTS_InputRouterComponent::*)(const FInputActionInstance&);
    FHandlerSignature Handler = nullptr;

    switch (BindingKey.TriggerEvent)
    {
        case ETriggerEvent::Started:
            Handler = &USOTS_InputRouterComponent::OnActionStarted;
            break;
        case ETriggerEvent::Triggered:
            Handler = &USOTS_InputRouterComponent::OnActionTriggered;
            break;
        case ETriggerEvent::Completed:
            Handler = &USOTS_InputRouterComponent::OnActionCompleted;
            break;
        case ETriggerEvent::Canceled:
            Handler = &USOTS_InputRouterComponent::OnActionCanceled;
            break;
        case ETriggerEvent::Ongoing:
            Handler = &USOTS_InputRouterComponent::OnActionOngoing;
            break;
        default:
            break;
    }

    if (!Handler)
    {
        return;
    }

    InputComp->BindAction(BindingKey.Action, BindingKey.TriggerEvent, this, Handler);
    const int32 NewIndex = InputComp->GetNumActionBindings() - 1;
    RouterOwnedBindingIndices.Add(NewIndex);
}

void USOTS_InputRouterComponent::OnActionStarted(const FInputActionInstance& Instance)
{
    RouteInput(Instance.GetSourceAction(), ETriggerEvent::Started, Instance.GetValue(), &Instance, false);
}

void USOTS_InputRouterComponent::OnActionTriggered(const FInputActionInstance& Instance)
{
    RouteInput(Instance.GetSourceAction(), ETriggerEvent::Triggered, Instance.GetValue(), &Instance, false);
}

void USOTS_InputRouterComponent::OnActionCompleted(const FInputActionInstance& Instance)
{
    RouteInput(Instance.GetSourceAction(), ETriggerEvent::Completed, Instance.GetValue(), &Instance, false);
}

void USOTS_InputRouterComponent::OnActionCanceled(const FInputActionInstance& Instance)
{
    RouteInput(Instance.GetSourceAction(), ETriggerEvent::Canceled, Instance.GetValue(), &Instance, false);
}

void USOTS_InputRouterComponent::OnActionOngoing(const FInputActionInstance& Instance)
{
    RouteInput(Instance.GetSourceAction(), ETriggerEvent::Ongoing, Instance.GetValue(), &Instance, false);
}

void USOTS_InputRouterComponent::GetActiveLayerTags(TArray<FGameplayTag>& OutTags) const
{
    OutTags.Reset();
    for (const FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        if (Layer.LayerAsset)
        {
            OutTags.Add(Layer.LayerAsset->LayerTag);
        }
    }
}

void USOTS_InputRouterComponent::GetOpenBufferChannels(TArray<FGameplayTag>& OutChannels) const
{
    OutChannels.Reset();
    if (const USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent())
    {
        Buffer->GetOpenChannels(OutChannels);
    }
}

FGameplayTag USOTS_InputRouterComponent::GetTopBufferChannel() const
{
    FGameplayTag Top;
    if (const USOTS_InputBufferComponent* Buffer = GetOrFindBufferComponent())
    {
        Buffer->GetTopOpenChannel(Top);
    }
    return Top;
}

void USOTS_InputRouterComponent::RefreshRouter()
{
    const TWeakObjectPtr<APlayerController> PreviousPC = OwningPC;
    const TWeakObjectPtr<UEnhancedInputComponent> PreviousInputComponent = EnhancedInputComp;

    SetComponentTickEnabled(bDebugLogRouterState);
    StartAutoRefreshTimer();

    TryResolveOwnerAndSubsystems();

    if (InputSubsystem.IsValid())
    {
        InputSubsystem->ClearAllMappings();
    }

    if (PreviousInputComponent.IsValid() && PreviousInputComponent.Get() != EnhancedInputComp.Get())
    {
        ClearRouterOwnedBindings(PreviousInputComponent.Get());
    }

    RebuildBindings();

    LastObservedPC = OwningPC;
    LastObservedInputComponent = EnhancedInputComp;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogRouterState)
    {
        UE_LOG(LogTemp, Log, TEXT("SOTS_Input Router refreshed (PC changed: %s, InputComp changed: %s)"),
            PreviousPC.Get() != OwningPC.Get() ? TEXT("true") : TEXT("false"),
            PreviousInputComponent.Get() != EnhancedInputComp.Get() ? TEXT("true") : TEXT("false"));
    }
#endif
}

void USOTS_InputRouterComponent::GetActiveLayerSummaries(TArray<FSOTS_ActiveInputLayerSummary>& OutSummaries) const
{
    OutSummaries.Reset();
    for (const FSOTS_ActiveInputLayer& Layer : ActiveLayers)
    {
        FSOTS_ActiveInputLayerSummary Summary;
        Summary.LayerTag = Layer.LayerAsset ? Layer.LayerAsset->LayerTag : FGameplayTag();
        Summary.Priority = Layer.Priority;
        Summary.bBlocksLowerPriorityLayers = Layer.bBlocksLowerPriorityLayers;
        Summary.ConsumePolicy = Layer.ConsumePolicy;
        Summary.MappingContextCount = Layer.AppliedContexts.Num();
        Summary.HandlerCount = Layer.RuntimeHandlers.Num();
        OutSummaries.Add(Summary);
    }
}

void USOTS_InputRouterComponent::GetBindingSnapshot(TArray<FSOTS_InputBindingKey>& OutBindings) const
{
    OutBindings = CachedBindingOrder;
}

void USOTS_InputRouterComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bDebugLogRouterState)
    {
        SetComponentTickEnabled(false);
        return;
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    if ((Now - LastDebugLogTime) >= DebugLogIntervalSeconds)
    {
        LastDebugLogTime = Now;

        TArray<FGameplayTag> LayerTags;
        GetActiveLayerTags(LayerTags);

        TArray<FGameplayTag> OpenChannels;
        GetOpenBufferChannels(OpenChannels);

        const FString LayersStr = FString::JoinBy(LayerTags, TEXT(","), [](const FGameplayTag& Tag){ return Tag.ToString(); });
        const FString ChannelsStr = FString::JoinBy(OpenChannels, TEXT(","), [](const FGameplayTag& Tag){ return Tag.ToString(); });
        const FGameplayTag TopChannel = GetTopBufferChannel();

        UE_LOG(LogTemp, Log, TEXT("SOTS_Input Router Debug: Layers=[%s] TopChannel=%s Open=[%s] Device=%d"), *LayersStr, *TopChannel.ToString(), *ChannelsStr, static_cast<int32>(LastDevice));
    }
#endif
}
