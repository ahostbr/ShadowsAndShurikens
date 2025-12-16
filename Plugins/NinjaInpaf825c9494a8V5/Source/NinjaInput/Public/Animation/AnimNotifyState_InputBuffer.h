// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_InputBuffer.generated.h"

class UAnimMontage;

/**
 * Enables the all Input Buffers available to the owner, during the notify duration.
 */
UCLASS(EditInlineNew , HideCategories = Object, CollapseCategories, meta = (DisplayName = "Input Buffer"))
class NINJAINPUT_API UAnimNotifyState_InputBuffer : public UAnimNotifyState
{
    
	GENERATED_BODY()

public:

	UAnimNotifyState_InputBuffer();

	// -- Begin Notify State implementation
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
	// -- End Notify State implementation

	/**
	 * Checks if the Mesh Component belongs to a local client. 
	 */
	static bool IsLocallyControlled(const USkeletalMeshComponent* MeshComp);
	
protected:

	/** The buffer channel managed by this Notify State. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
	FGameplayTag BufferChannelTag;
	
	float PlayRate;
	float ElapsedTime;
	float NotifyDuration;
	float EffectiveDuration;
	
	/**
	 * Calculates the actual play rate, considering the Ability System Component as well.
	 *
	 * Having the ASC is not required, and this will work fine without it. But if the ASC is present,
	 * we need to determine the Play Rate set to its Animation Instance, and not the one in the Animation.
	 * 
	 * @param BufferOwner		Actor that owns the Input Buffer, and also the ASC.
	 * @param Animation			Animation being played, provides a fallback play rate if there's no ASC.
	 */
	virtual float GetActualPlayRate(AActor* BufferOwner, const UAnimSequenceBase* Animation) const;

	/**
	 * Closes the Input Buffer.
	 *
	 * This Notify State can close the Buffer in two important scenarios: When the Notify
	 * ends (finishes or cancelled), or when the accumulated time is higher than the actual
	 * duration, reduced from a faster play rate.
	 */
	virtual void CloseBuffer(AActor* BufferOwner);
	
#if WITH_EDITOR

	// -- Begin Notify State implementation
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
	// -- End Notify State implementation
	
#endif
};
