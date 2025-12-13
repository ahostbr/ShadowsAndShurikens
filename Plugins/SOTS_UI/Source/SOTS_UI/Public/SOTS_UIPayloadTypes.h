#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "SOTS_UIPayloadTypes.generated.h"

class UMediaSource;
class AActor;

USTRUCT(BlueprintType)
struct F_SOTS_UIConfirmDialogPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FText ConfirmText = NSLOCTEXT("SOTS", "UIConfirmYes", "Yes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FText CancelText = NSLOCTEXT("SOTS", "UIConfirmNo", "No");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGameplayTag ConfirmActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGameplayTag CancelActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bCloseOnConfirm = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bCloseOnCancel = true;
};

USTRUCT(BlueprintType)
struct F_SOTS_UIPlayVideoPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FName VideoId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	TSoftObjectPtr<UMediaSource> MediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bPauseGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bSkippable = true;
};

USTRUCT(BlueprintType)
struct F_SOTS_UIOpenMenuPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGameplayTag MenuId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FInstancedStruct MenuData;
};

USTRUCT(BlueprintType)
struct F_SOTS_UINotificationPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	float DurationSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGameplayTag CategoryTag;
};

USTRUCT(BlueprintType)
struct F_SOTS_UIWaypointPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bActorTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	FGameplayTag CategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bClampToEdges = true;
};

UENUM(BlueprintType)
enum class ESOTS_UIInventoryRequestType : uint8
{
	OpenInventory,
	CloseInventory,
	ToggleInventory,
	OpenContainer,
	CloseContainer,
	ToggleContainer
};

USTRUCT(BlueprintType)
struct F_SOTS_UIInventoryRequestPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	ESOTS_UIInventoryRequestType RequestType = ESOTS_UIInventoryRequestType::OpenInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	TWeakObjectPtr<AActor> ContainerActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|UI|Payload")
	bool bPauseGameOverride = false;
};
