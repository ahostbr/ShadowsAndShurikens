// Ninja Bear Studio Inc., all rights reserved.
#include "Animation/AnimNotifyState_InputBuffer.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "NinjaInputFunctionLibrary.h"
#include "NinjaInputLogs.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/NinjaInputManagerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UAnimNotifyState_InputBuffer::UAnimNotifyState_InputBuffer()
{
	PlayRate = 1.f;
    ElapsedTime = 0.f;
    NotifyDuration = 0.f;
	EffectiveDuration = 0.f;
	BufferChannelTag = Tag_Input_Buffer_Channel_Default;

    #if WITH_EDITORONLY_DATA
    NotifyColor = FColor(211, 221, 197);
    #endif
}

void UAnimNotifyState_InputBuffer::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsLocallyControlled(MeshComp))
	{
		return;
	}

	const AActor* Owner = MeshComp->GetOwner();
	UActorComponent* InputBuffer = UNinjaInputFunctionLibrary::GetInputBufferComponent(Owner);
	if (IsValid(InputBuffer) && BufferChannelTag.IsValid())
	{
		ElapsedTime = 0.f;
		NotifyDuration = TotalDuration;
		PlayRate = GetActualPlayRate(MeshComp->GetOwner(), Animation);
		EffectiveDuration = NotifyDuration / PlayRate;

		UE_LOG(LogNinjaInputBufferComponent, Verbose,
			TEXT("Animation %s on %s will attempt to open the %s buffer for %f seconds."),
			*GetNameSafe(Animation), *GetNameSafe(Owner), *BufferChannelTag.ToString(), EffectiveDuration);
		
		IInputBufferInterface::Execute_OpenInputBuffer(InputBuffer, BufferChannelTag);
	}
}

void UAnimNotifyState_InputBuffer::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (IsLocallyControlled(MeshComp))
	{
		ElapsedTime += FrameDeltaTime;

		// Our elapsed time is greater or equal than the effective duration. This usually happens
		// when the Play Rate in the animation is faster than the default, so the duration is shorter.
		//
		if (ElapsedTime >= EffectiveDuration)
		{
			const AActor* Owner = MeshComp->GetOwner();
			UE_LOG(LogNinjaInputBufferComponent, Verbose,
				TEXT("Animation %s on %s will attempt to close the %s buffer after %f seconds."),
				*GetNameSafe(Animation), *GetNameSafe(Owner), *BufferChannelTag.ToString(), ElapsedTime);
			
			CloseBuffer(MeshComp->GetOwner());
		}	
	}
}

void UAnimNotifyState_InputBuffer::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (IsLocallyControlled(MeshComp))
	{
		const AActor* Owner = MeshComp->GetOwner();
		UE_LOG(LogNinjaInputBufferComponent, Verbose,
			TEXT("Animation %s on %s will attempt to close the %s buffer from notify end."),
			*GetNameSafe(Animation), *GetNameSafe(Owner), *BufferChannelTag.ToString());
		
		CloseBuffer(MeshComp->GetOwner());	
	}
}

float UAnimNotifyState_InputBuffer::GetActualPlayRate(AActor* BufferOwner, const UAnimSequenceBase* Animation) const
{
	if (!IsValid(BufferOwner) || !IsValid(Animation))
	{
		return 1.f;
	}

	const float AnimationPlayRate = Animation->RateScale;
	
	const UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BufferOwner);
	if (!IsValid(AbilitySystemComponent) || AbilitySystemComponent->GetCurrentMontage() != Animation)
	{
		return AnimationPlayRate;
	}
	
	const TWeakObjectPtr<UAnimInstance> AnimInstancePtr = AbilitySystemComponent->AbilityActorInfo->AnimInstance;
	if (!AnimInstancePtr.IsValid() || !AnimInstancePtr->IsValidLowLevelFast())
	{
		return AnimationPlayRate;
	}
	
	const UAnimMontage* CurrentMontage = AbilitySystemComponent->GetCurrentMontage();
	return AnimInstancePtr->Montage_GetEffectivePlayRate(CurrentMontage);
}

void UAnimNotifyState_InputBuffer::CloseBuffer(AActor* BufferOwner)
{
	if (!IsValid(BufferOwner))
	{
		return;
	}

	UActorComponent* InputBuffer = UNinjaInputFunctionLibrary::GetInputBufferComponent(BufferOwner);
	if (!IsValid(InputBuffer) || !IInputBufferInterface::Execute_IsInputBufferOpen(InputBuffer, BufferChannelTag))
	{
		return;
	}

	const bool bCancelled = ElapsedTime <= EffectiveDuration;
	IInputBufferInterface::Execute_CloseInputBuffer(InputBuffer, BufferChannelTag, bCancelled);
}

FString UAnimNotifyState_InputBuffer::GetNotifyName_Implementation() const
{
    return "Input Buffer";
}

bool UAnimNotifyState_InputBuffer::IsLocallyControlled(const USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(MeshComp->GetOwner());
	if (!IsValid(Pawn))
	{
		return false;
	}

	return Pawn->IsLocallyControlled();
}

#if WITH_EDITOR
EDataValidationResult UAnimNotifyState_InputBuffer::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Overall = Super::IsDataValid(Context);
	
	// We need a valid buffer channel.
	if (!BufferChannelTag.IsValid())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("AssetName"), FText::FromString(GetName()));
		const FText Message = FText::Format(NSLOCTEXT("InputBufferNotifyNamespace", "InvalidBufferChannel", "Animation {AssetName} has an invalid buffer channel."), Args);
		Context.AddError(Message);
	}

	EDataValidationResult Assessment = EDataValidationResult::Valid; 
	if (Context.GetNumErrors() > 0 || Context.GetNumWarnings() > 0)
	{
		Assessment = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(Assessment, Overall);
}

bool UAnimNotifyState_InputBuffer::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return Animation && Animation->IsA(UAnimMontage::StaticClass());
}
#endif