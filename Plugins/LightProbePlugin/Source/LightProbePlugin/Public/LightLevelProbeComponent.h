
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LightLevelProbeComponent.generated.h"

class UTextureRenderTarget2D;
class UStaticMeshComponent;
class USpringArmComponent;
class USceneCaptureComponent2D;
class ULightProbeDebugWidget;
class UMaterialInterface;

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class LIGHTPROBEPLUGIN_API ULightLevelProbeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULightLevelProbeComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category="Light Probe")
    float GetLightLevel() const { return CurrentLightLevel; }

    UFUNCTION(BlueprintPure, Category="Light Probe")
    float GetRawLuminance() const { return RawLuminance; }

    /** How often (seconds) to resample the probe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="0.01", UIMin="0.01"))
    float ProbeUpdateInterval;

    /** Vertical offset above the owner to place the probe cube & capture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    float ProbeHeightOffset;

    /** Size in pixels of the square render target. Higher = less noise, more cost. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="16", UIMin="16"))
    int32 RenderTargetSize;

    /** Number of samples to take from the render target (3 = cube faces, >3 = grid). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="1", ClampMax="16", UIMin="1", UIMax="16"))
    int32 NumSamples;

    /** Distance from probe cube to the capture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="0.0", UIMin="0.0"))
    float ProbeArmLength;

    /** FOV used by the probe SceneCapture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="1.0", ClampMax="120.0", UIMin="1.0", UIMax="120.0"))
    float ProbeFOV;

    /** Smoothing factor for light level changes (0-1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float SmoothingAlpha;

    /** Rotation of the probe cube mesh (to get 3 faces visible). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    FRotator ProbeMeshRotation;

    /** Rotation of the probe spring arm (usually points down at the cube). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    FRotator ProbeArmRotation;

    /** Material used on the probe cube. If null, uses DefaultWhite. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe")
    UMaterialInterface* ProbeMaterial;

    // --- Calibration ---

    /** Raw luminance value we treat as "fully dark". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Calibration", meta=(ClampMin="0.0"))
    float ShadowReference;

    /** Raw luminance value we treat as "fully bright". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Calibration", meta=(ClampMin="0.0"))
    float BrightReference;

    /** Curve applied after normalization (gamma). 1 = linear. <1 brightens shadows. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Calibration", meta=(ClampMin="0.01"))
    float ResponseCurveGamma;

    /** If true, the probe capture uses scene auto exposure. If false, exposure is locked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Calibration")
    bool bUseAutoExposure;

    /** Exposure brightness used when auto exposure is disabled (min/max brightness). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Calibration", meta=(ClampMin="0.01", ClampMax="5.0", EditCondition="!bUseAutoExposure"))
    float LockedExposure;

    // --- Debug ---

    /** Show the debug RT preview widget on screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Debug")
    bool bShowDebugWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Debug")
    FVector2D DebugWidgetSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Probe|Debug")
    FVector2D DebugWidgetOffset;

protected:
    void InitializeProbe();
    void ShutdownProbe();
    void UpdateLightLevel();

    UPROPERTY(Transient)
    UTextureRenderTarget2D* RenderTarget;

    UPROPERTY(Transient)
    UStaticMeshComponent* ProbeMesh;

    UPROPERTY(Transient)
    USpringArmComponent* ProbeSpringArm;

    UPROPERTY(Transient)
    USceneCaptureComponent2D* ProbeCapture;

    /** Raw averaged luminance from the capture. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Light Probe", meta=(AllowPrivateAccess="true"))
    float RawLuminance;

    /** Normalized light level [0..1] after calibration. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Light Probe", meta=(AllowPrivateAccess="true"))
    float CurrentLightLevel;

    FTimerHandle UpdateTimerHandle;

    UPROPERTY(Transient)
    ULightProbeDebugWidget* DebugWidget;
};
