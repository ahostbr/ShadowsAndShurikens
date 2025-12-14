#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_BodyDragTypes.h"
#include "SOTS_InteractableInterface.h"
#include "SOTS_BodyDragPlayerComponent.generated.h"

class USOTS_BodyDragTargetComponent;
class USOTS_BodyDragConfigDA;
class UCharacterMovementComponent;
class UAnimMontage;

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class USOTS_BodyDragPlayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_BodyDragPlayerComponent();

    UPROPERTY(BlueprintReadOnly, Category="SOTS|BodyDrag")
    ESOTS_BodyDragState DragState;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|BodyDrag")
    AActor* CurrentBodyActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|BodyDrag")
    USOTS_BodyDragTargetComponent* CurrentBodyTargetComp;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    USOTS_BodyDragConfigDA* ConfigDA;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|BodyDrag")
    float CachedOriginalMaxWalkSpeed;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    bool IsDragging() const;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    ESOTS_BodyDragState GetDragState() const;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    bool TryBeginDrag(AActor* BodyActor);

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    bool TryDropBody();

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    AActor* GetCurrentBody() const;

private:
    ESOTS_BodyDragBodyType CurrentBodyType;

    void OnStartMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void OnStopMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void ApplyDraggingTags();
    void RemoveDraggingTags();
    void FinalizeDrop();
    void DebugDrawDragging() const;
    UCharacterMovementComponent* ResolveMovementComponent() const;
    const FSOTS_BodyDragConfig* GetConfig() const;
};
