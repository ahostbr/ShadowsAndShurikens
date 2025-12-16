#include "SOTS_InputLayerRegistrySubsystem.h"

#include "SOTS_InputLayerDataAsset.h"
#include "SOTS_InputLayerRegistrySettings.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void USOTS_InputLayerRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const USOTS_InputLayerRegistrySettings* Settings = GetDefault<USOTS_InputLayerRegistrySettings>();
    if (!Settings)
    {
        return;
    }

    bUseAsyncLoads = Settings->bUseAsyncLoads;

    LayerMap.Reset();
    LoadedLayers.Reset();
    PendingLoads.Reset();

    for (const FSOTS_InputLayerRegistryEntry& Entry : Settings->Layers)
    {
        if (!Entry.LayerTag.IsValid())
        {
            continue;
        }

        LayerMap.Add(Entry.LayerTag, Entry.LayerAsset);
    }

    if (Settings->bLogRegistryLoads)
    {
        UE_LOG(LogTemp, Log, TEXT("SOTS_Input LayerRegistry loaded %d entries (Async=%s)."), LayerMap.Num(), bUseAsyncLoads ? TEXT("true") : TEXT("false"));
    }
}

bool USOTS_InputLayerRegistrySubsystem::TryGetLayerAsset(FGameplayTag Tag, USOTS_InputLayerDataAsset*& OutLayer) const
{
    OutLayer = nullptr;

    if (!Tag.IsValid())
    {
        return false;
    }

    if (USOTS_InputLayerDataAsset* const* LoadedPtr = LoadedLayers.Find(Tag))
    {
        OutLayer = *LoadedPtr;
        return OutLayer != nullptr;
    }

    const TSoftObjectPtr<USOTS_InputLayerDataAsset>* FoundPtr = LayerMap.Find(Tag);
    if (!FoundPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: Layer tag %s not found in registry."), *Tag.ToString());
        return false;
    }

    if (USOTS_InputLayerDataAsset* LoadedAsset = FoundPtr->Get())
    {
        LoadedLayers.Add(Tag, LoadedAsset);
        OutLayer = LoadedAsset;
        return true;
    }

    if (!bUseAsyncLoads)
    {
        USOTS_InputLayerDataAsset* Loaded = FoundPtr->LoadSynchronous();
        if (!Loaded)
        {
            UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: Layer tag %s failed to load asset."), *Tag.ToString());
            return false;
        }

        LoadedLayers.Add(Tag, Loaded);
        OutLayer = Loaded;
        return true;
    }

    // Async mode: kick request and wait for a future call.
    const_cast<USOTS_InputLayerRegistrySubsystem*>(this)->RequestLayerAsync(Tag);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input: Layer tag %s requested async load."), *Tag.ToString());
#endif
    return false;
}

void USOTS_InputLayerRegistrySubsystem::RequestLayerAsync(FGameplayTag Tag)
{
    if (!bUseAsyncLoads)
    {
        USOTS_InputLayerDataAsset* Loaded = nullptr;
        TryGetLayerAsset(Tag, Loaded);
        return;
    }

    if (!Tag.IsValid())
    {
        return;
    }

    if (LoadedLayers.Contains(Tag))
    {
        return;
    }

    if (const TSharedPtr<FStreamableHandle>* Existing = PendingLoads.Find(Tag))
    {
        if (Existing->IsValid())
        {
            return;
        }
    }

    const TSoftObjectPtr<USOTS_InputLayerDataAsset>* FoundPtr = LayerMap.Find(Tag);
    if (!FoundPtr || FoundPtr->IsNull())
    {
        return;
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(
        FoundPtr->ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &USOTS_InputLayerRegistrySubsystem::OnLayerLoadedInternal, Tag, *FoundPtr),
        FStreamableManager::AsyncLoadHighPriority);

    if (Handle.IsValid())
    {
        PendingLoads.Add(Tag, Handle);
    }
}

void USOTS_InputLayerRegistrySubsystem::OnLayerLoadedInternal(FGameplayTag Tag, TSoftObjectPtr<USOTS_InputLayerDataAsset> SoftPtr)
{
    PendingLoads.Remove(Tag);

    USOTS_InputLayerDataAsset* Loaded = SoftPtr.Get();
    if (!Loaded)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: Async load failed for layer tag %s"), *Tag.ToString());
#endif
        return;
    }

    LoadedLayers.Add(Tag, Loaded);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogTemp, Log, TEXT("SOTS_Input: Async load completed for layer tag %s"), *Tag.ToString());
#endif
}
