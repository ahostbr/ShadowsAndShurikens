// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/Modifiers/UGC_CameraModifier.h"
#include "Camera/Data/UGC_CameraData.h"
#include "DrawDebugHelpers.h"
#include "UGC_CameraDitheringModifier.generated.h"

UENUM()
enum class EDitherType : uint8
{
    None,
    BlockingLOS,
    OverlappingCamera
};

USTRUCT(BlueprintType)
struct FDitheredActorState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    TObjectPtr<AActor> Actor;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    float CurrentOpacity = 1.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    float CollisionTime = 0.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    bool bIsDitheringIn = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    bool bIsDitheringOut = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UGC|Modifiers|Camera Dithering")
    EDitherType DitherType = EDitherType::None;

    bool IsValid() const { return Actor != nullptr && DitherType != EDitherType::None; }

    void StartDithering(AActor* InActor, EDitherType InDitherType);

    void Invalidate();

    friend bool operator==(FDitheredActorState const& lhs, FDitheredActorState const& rhs)
    {
        return lhs.Actor == rhs.Actor;
    }

    void ComputeOpacity(float DeltaTime, float DitherInTime, float DitherOutTime, float DitherMin);
};

/**
 * UGC Camera Modifier used to dither objects colliding with the camera
 */
UCLASS()
class AURORADEVS_UGC_API UUGC_CameraDitheringModifier : public UUGC_CameraModifier
{
    GENERATED_BODY()

public:
    UUGC_CameraDitheringModifier();

protected:
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

    virtual void ApplyDithering(float DeltaTime, FDitheredActorState& DitherState);

private:
#if ENABLE_DRAW_DEBUG
    void UGCDebugDithering(FDitheredActorState& DitherState, float DeltaTime, float DitherMin);
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|Modifiers|Dithering")
    FCameraDitheringSettings DitheringSettings;

    /** Material Parameter Collection for everything dithering-related */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|Modifiers|Dithering")
    TSoftObjectPtr<class UMaterialParameterCollection> DitheringMPC;

    UPROPERTY(Transient)
    TSoftObjectPtr<class UMaterialParameterCollectionInstance> DitheringMPCInstance;

    UPROPERTY(Transient)
    TArray<FDitheredActorState> DitheredActorStates;
};