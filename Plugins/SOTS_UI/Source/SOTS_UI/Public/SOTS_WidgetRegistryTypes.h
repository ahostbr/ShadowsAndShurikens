#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SOTS_WidgetRegistryTypes.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class ESOTS_UILayer : uint8
{
	HUD,
	Overlay,
	Modal,
	Debug
};

UENUM(BlueprintType)
enum class ESOTS_UIInputPolicy : uint8
{
	GameOnly,
	UIOnly,
	GameAndUI
};

UENUM(BlueprintType)
enum class ESOTS_UICachePolicy : uint8
{
	Recreate,
	KeepAlive
};

USTRUCT(BlueprintType)
struct FSOTS_WidgetRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	FGameplayTag WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	TSoftClassPtr<UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	ESOTS_UILayer Layer = ESOTS_UILayer::Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	ESOTS_UIInputPolicy InputPolicy = ESOTS_UIInputPolicy::GameOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	bool bPauseGame = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	ESOTS_UICachePolicy CachePolicy = ESOTS_UICachePolicy::Recreate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	int32 ZOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	bool bAllowMultiple = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UI")
	bool bCloseOnEscape = true;
};
