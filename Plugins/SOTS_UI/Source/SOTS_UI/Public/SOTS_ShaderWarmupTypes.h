#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_ShaderWarmupTypes.generated.h"

class USOTS_ShaderWarmupLoadListDataAsset;

UENUM(BlueprintType)
enum class ESOTS_ShaderWarmupSourceMode : uint8
{
	UseLoadListDA,
	LevelReferencedFallback
};

USTRUCT(BlueprintType)
struct F_SOTS_ShaderWarmupRequest
{
	GENERATED_BODY()

	F_SOTS_ShaderWarmupRequest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	FName TargetLevelPackageName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	FGameplayTag ScreenWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	bool bUseMoviePlayerDuringMapLoad = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	bool bFreezeWithGlobalTimeDilation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	float FrozenTimeDilation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	ESOTS_ShaderWarmupSourceMode SourceMode = ESOTS_ShaderWarmupSourceMode::UseLoadListDA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Warmup")
	TObjectPtr<USOTS_ShaderWarmupLoadListDataAsset> LoadListOverride = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnShaderWarmupProgress, float, Percent, FText, StatusText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnShaderWarmupFinished);

inline F_SOTS_ShaderWarmupRequest::F_SOTS_ShaderWarmupRequest()
{
	ScreenWidgetId = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Screen.Loading.ShaderWarmup")), false);
}
