// Copyright 2020 Ace. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DynamicSocketEditLib.generated.h"


/**
 *
 */
UCLASS()
class SOCKETNOTIFYCOPY_API UDynamicSocketEditLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UE_ENABLE_OPTIMIZATION
		UFUNCTION(BlueprintCallable, Category = "SocketEdit")
		static void CpoySocket(UObject* source, UObject* target) {

		class USkeleton* ss = (class USkeleton*)source;
		class USkeleton* tt = (class USkeleton*)target;
		if (nullptr == ss || nullptr == tt)
		{
			return;
		}

		for (auto t : ss->Sockets)
		{
			auto temp = tt->FindSocket(t->SocketName);
			if (temp &&temp->BoneName == t->BoneName) {
				continue;
			}
			USkeletalMeshSocket* ns = NewObject<USkeletalMeshSocket>(tt);
			ns->SocketName = t->SocketName;
			ns->BoneName = t->BoneName;
			ns->RelativeLocation = t->RelativeLocation;
			ns->RelativeRotation = t->RelativeRotation;
			ns->RelativeScale = t->RelativeScale;
			ns->bForceAlwaysAnimated = t->bForceAlwaysAnimated;
			tt->Sockets.AddUnique(ns);
		}
	}


	UFUNCTION(BlueprintCallable, Category = "SocketEdit")
		static void SkeletonNotifiesCopy(UObject* source, UObject* target) {

		class USkeleton* ss = (class USkeleton*)source;
		class USkeleton* tt = (class USkeleton*)target;
		if (nullptr == ss || nullptr == tt)
		{
			return;
		}

#if WITH_EDITORONLY_DATA
		for (auto t : ss->AnimationNotifies)
		{
			tt->AddNewAnimationNotify(t);
		}
#endif
	}

};
