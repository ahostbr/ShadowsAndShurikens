#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "SOTS_BodyDragTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTS_BodyDragState : uint8
{
    None,
    Entering,
    Dragging,
    Exiting
};

UENUM(BlueprintType)
enum class ESOTS_BodyDragBodyType : uint8
{
    KnockedOut,
    Dead
};

USTRUCT(BlueprintType)
struct FSOTS_BodyDragMontageSet
{
    GENERATED_BODY()

    /** Montage to start dragging. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    UAnimMontage* StartDrag = nullptr;

    /** Montage to stop dragging. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    UAnimMontage* StopDrag = nullptr;
};

USTRUCT(BlueprintType)
struct FSOTS_BodyDragConfig
{
    GENERATED_BODY()

    /** Knocked-out body montage set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FSOTS_BodyDragMontageSet KO;

    /** Dead body montage set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FSOTS_BodyDragMontageSet Dead;

    /** Socket to attach dragged target to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FName AttachSocketName = TEXT("BodyDragSocket");

    /** Movement speed while dragging. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    float DragWalkSpeed = 180.f;

    /** Attach target when dragging starts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    bool bAttachOnStart = true;

    /** Re-enable physics when dropping a dead target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    bool bReenablePhysicsOnDrop_Dead = true;

    /** Re-enable physics when dropping a KO target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    bool bReenablePhysicsOnDrop_KO = false;

    /** Tag name representing dragging state on the player. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FName DraggingStateTagName = TEXT("SAS.BodyDrag.State.Dragging");

    /** Tag name applied when dragging a dead target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FName DraggingDeadTagName = TEXT("SAS.BodyDrag.Target.Dead");

    /** Tag name applied when dragging a KO target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FName DraggingKOTagName = TEXT("SAS.BodyDrag.Target.KO");
};
