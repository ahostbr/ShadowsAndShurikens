#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_BodyDragTypes.h"
#include "SOTS_BodyDragTargetComponent.generated.h"

class USkeletalMeshComponent;

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class USOTS_BodyDragTargetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_BodyDragTargetComponent();

    /** KO vs Dead body state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    ESOTS_BodyDragBodyType BodyType;

    /** Optional override for the mesh to drive physics/collision changes. Auto-finds first skeletal mesh if unset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    USkeletalMeshComponent* BodyMesh;

    /** True while being dragged (runtime). */
    UPROPERTY(BlueprintReadOnly, Category="SOTS|BodyDrag", Transient)
    bool bIsCurrentlyDragged;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    bool CanBeDragged() const;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    USkeletalMeshComponent* ResolveBodyMesh() const;

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    void BeginDragged();

    UFUNCTION(BlueprintCallable, Category="SOTS|BodyDrag")
    void EndDragged(bool bDropToPhysics);

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS|BodyDrag")
    void BP_OnDragStarted(AActor* DraggingActor);

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS|BodyDrag")
    void BP_OnDragEnded(AActor* DraggingActor);

private:
    bool bCachedSimulatePhysics;
    FName CachedCollisionProfileName;
    bool bCachedCollisionEnabled;
};
