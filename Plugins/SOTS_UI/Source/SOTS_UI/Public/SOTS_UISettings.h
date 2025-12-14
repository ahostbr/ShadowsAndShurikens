#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_WidgetRegistryDataAsset.h"
#include "SOTS_ProHUDAdapter.h"
#include "SOTS_InvSPAdapter.h"
#include "Adapters/SOTS_InteractionEssentialsAdapter.h"
#include "UObject/SoftObjectPtr.h"
#include "SOTS_UISettings.generated.h"

/**
 * Project settings for SOTS_UI. Exposes widget registry and adapter classes under SOTS -> UI in Project Settings.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS UI Settings"))
class SOTS_UI_API USOTS_UISettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_UISettings(const FObjectInitializer& ObjectInitializer);

    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const override;

    static const USOTS_UISettings* Get();

public:
    UPROPERTY(EditAnywhere, Config, Category="Registry", meta=(AllowedClasses="/Script/SOTS_UI.SOTS_WidgetRegistryDataAsset", ToolTip="Default widget registry consumed by the SOTS_UI router."))
    TSoftObjectPtr<USOTS_WidgetRegistryDataAsset> DefaultWidgetRegistry;

    UPROPERTY(EditAnywhere, Config, Category="Adapters", meta=(ToolTip="Adapter used to bridge ProHUD into the router."))
    TSubclassOf<USOTS_ProHUDAdapter> ProHUDAdapterClass;

    UPROPERTY(EditAnywhere, Config, Category="Adapters", meta=(ToolTip="Adapter used to bridge InventorySP into the router."))
    TSubclassOf<USOTS_InvSPAdapter> InvSPAdapterClass;

    UPROPERTY(EditAnywhere, Config, Category="Adapters", meta=(ToolTip="Adapter used to bridge InteractionEssentials prompts/markers into the router."))
    TSubclassOf<USOTS_InteractionEssentialsAdapter> InteractionAdapterClass;
};
