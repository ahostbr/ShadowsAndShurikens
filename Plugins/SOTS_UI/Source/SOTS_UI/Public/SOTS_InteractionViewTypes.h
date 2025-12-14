#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "Engine/Texture2D.h"
#include "SOTS_InteractionViewTypes.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct F_SOTS_InteractionPromptView
{
	GENERATED_BODY()

	// Main interaction label (e.g., "Open", "Pick up"). Optional.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FText PromptText;

	// Input key or tag used to drive icon lookup (supports gamepad/KBM).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FKey InputKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FGameplayTag InputTag;

	// Progress value in [0..1]; ignored when bShowProgress is false.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	float HoldProgress01 = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bShowProgress = false;

	// Draw attention (pulsing/highlight) if supported by the widget.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bAttention = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bRepeat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bIsQTE = false;

	// Icon lookup; either explicit soft texture, icon id, or tag depending on widget support.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FName IconId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	TSoftObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FGameplayTag IconTag;
};

USTRUCT(BlueprintType)
struct F_SOTS_InteractionMarkerView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FGuid MarkerId;

	// World-space position for the marker; adapters may also bind to an actor if needed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bShowArrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	FGameplayTag CategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Interaction")
	bool bClampToEdges = true;
};
