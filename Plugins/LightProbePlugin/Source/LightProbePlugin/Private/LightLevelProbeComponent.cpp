#include "LightLevelProbeComponent.h"
#include "LightProbeDebugWidget.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "SOTS_PlayerStealthComponent.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"   // EMaterialDomain, MD_Surface

#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#include "Runtime/Launch/Resources/Version.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarLightProbeDebugWidget(
    TEXT("lightprobe.DebugWidget"),
    0,
    TEXT("Enable the LightProbe debug viewport widget (dev only)."),
    ECVF_Default
);

static bool AreDebugWidgetsAllowed()
{
    return CVarLightProbeDebugWidget.GetValueOnGameThread() != 0;
}
#endif

static TAutoConsoleVariable<int32> CVarLightProbeShowProbeMesh(
    TEXT("sots.lightprobe.ShowProbeMesh"),
    0,
    TEXT("Show the LightProbe proxy mesh for debugging (mesh remains shadowless)."),
    ECVF_Default
);


ULightLevelProbeComponent::ULightLevelProbeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    ProbeUpdateInterval = 0.1f;
    ProbeHeightOffset = 100.0f;
    RenderTargetSize = 512;
    NumSamples = 3;
    ProbeArmLength = 40.0f;
    ProbeFOV = 25.0f;
    SmoothingAlpha = 0.35f;

    ProbeMeshRotation = FRotator(45.f, 45.f, 0.f);
    ProbeArmRotation = FRotator(90.f, 0.f, 0.f);

    ProbeMaterial = nullptr;

    ShadowReference = 0.05f;
    BrightReference = 0.6f;
    ResponseCurveGamma = 1.0f;

    bUseAutoExposure = true;   // default: follow project settings
    LockedExposure = 1.0f;   // used when bUseAutoExposure == false

    bShowDebugWidget = false;
    DebugWidgetSize = FVector2D(256.f, 256.f);
    DebugWidgetOffset = FVector2D(16.f, 16.f);

    RenderTarget = nullptr;
    ProbeMesh = nullptr;
    ProbeSpringArm = nullptr;
    ProbeCapture = nullptr;
    DebugWidget = nullptr;

    RawLuminance = 0.0f;
    CurrentLightLevel = 0.0f;
}

void ULightLevelProbeComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeProbe();

    if (UWorld* World = GetWorld())
    {
        if (ProbeUpdateInterval > 0.0f && ProbeCapture && RenderTarget)
        {
            World->GetTimerManager().SetTimer(
                UpdateTimerHandle,
                this,
                &ULightLevelProbeComponent::UpdateLightLevel,
                ProbeUpdateInterval,
                true
            );
        }
    }
}

void ULightLevelProbeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ShutdownProbe();
    Super::EndPlay(EndPlayReason);
}

void ULightLevelProbeComponent::InitializeProbe()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    USceneComponent* OwnerRoot = Owner->GetRootComponent();
    if (!OwnerRoot) return;

    // --- Probe mesh ---
    if (!ProbeMesh)
    {
        ProbeMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("LightProbeMesh"));
        if (ProbeMesh)
        {
            ProbeMesh->SetupAttachment(OwnerRoot);
            ProbeMesh->SetRelativeLocation(FVector(0.f, 0.f, ProbeHeightOffset));
            ProbeMesh->SetWorldScale3D(FVector(0.3f));
            ProbeMesh->SetRelativeRotation(ProbeMeshRotation);
            ProbeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            ProbeMesh->SetGenerateOverlapEvents(false);
            ProbeMesh->CastShadow = false;
            ProbeMesh->bCastHiddenShadow = false;
            ProbeMesh->bOwnerNoSee = true;
            const bool bProbeMeshVisible = CVarLightProbeShowProbeMesh.GetValueOnGameThread() != 0;
            ProbeMesh->SetHiddenInGame(!bProbeMeshVisible);
            ProbeMesh->SetVisibility(bProbeMeshVisible);

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
            if (CubeMesh)
            {
                ProbeMesh->SetStaticMesh(CubeMesh);
            }

            UMaterialInterface* MatToUse = ProbeMaterial;

            // 1) First try user-assigned material
            // 2) Then try Engine DefaultWhite (works on all versions)
            if (!MatToUse)
            {
                MatToUse = LoadObject<UMaterialInterface>(
                    nullptr,
                    TEXT("/Engine/EngineMaterials/DefaultWhite.DefaultWhite")
                );
            }

            // 3) Optional extra safety net on newer engine versions only
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
            if (!MatToUse)
            {
                MatToUse = UMaterial::GetDefaultMaterial(MD_Surface);
            }
#endif

            if (MatToUse)
            {
                ProbeMesh->SetMaterial(0, MatToUse);
            }

            ProbeMesh->RegisterComponent();
        }
    }

    // --- Spring arm ---
    if (!ProbeSpringArm)
    {
        ProbeSpringArm = NewObject<USpringArmComponent>(Owner, TEXT("LightProbeSpringArm"));
        if (ProbeSpringArm)
        {
            ProbeSpringArm->SetupAttachment(OwnerRoot);
            ProbeSpringArm->TargetArmLength = ProbeArmLength;
            ProbeSpringArm->SetRelativeLocation(FVector(0.f, 0.f, ProbeHeightOffset));
            ProbeSpringArm->SetRelativeRotation(ProbeArmRotation);
            ProbeSpringArm->bDoCollisionTest = false;
            ProbeSpringArm->RegisterComponent();
        }
    }

    // --- Scene capture ---
    if (!ProbeCapture)
    {
        ProbeCapture = NewObject<USceneCaptureComponent2D>(Owner, TEXT("LightProbeCapture"));
        if (ProbeCapture)
        {
            ProbeCapture->SetupAttachment(ProbeSpringArm ? ProbeSpringArm : OwnerRoot);
            ProbeCapture->SetRelativeLocation(FVector::ZeroVector);
            ProbeCapture->SetRelativeRotation(FRotator::ZeroRotator);
            ProbeCapture->FOVAngle = ProbeFOV;
            ProbeCapture->bCaptureEveryFrame = false;
            ProbeCapture->bCaptureOnMovement = false;
            ProbeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
            ProbeCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;

            // Per-probe exposure control
            ProbeCapture->PostProcessBlendWeight = 1.0f;
            auto& PPS = ProbeCapture->PostProcessSettings;

            if (!bUseAutoExposure)
            {
                PPS.bOverride_AutoExposureMinBrightness = true;
                PPS.bOverride_AutoExposureMaxBrightness = true;

                PPS.AutoExposureMinBrightness = LockedExposure;
                PPS.AutoExposureMaxBrightness = LockedExposure;
            }

            ProbeCapture->RegisterComponent();
        }
    }

    // --- Render target ---
    if (!RenderTarget)
    {
        RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("LightProbeRT"));
        if (RenderTarget)
        {
            const int32 ClampedSize = FMath::Max(RenderTargetSize, 16);
            RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
            RenderTarget->InitAutoFormat(ClampedSize, ClampedSize);
            RenderTarget->bAutoGenerateMips = false;
            RenderTarget->UpdateResourceImmediate(true);
        }
    }

    if (ProbeCapture && RenderTarget)
    {
        ProbeCapture->TextureTarget = RenderTarget;
    }

    // --- Debug widget ---
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bShowDebugWidget && RenderTarget && !DebugWidget && Owner->GetNetMode() != NM_DedicatedServer && AreDebugWidgetsAllowed())
    {
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                DebugWidget = CreateWidget<ULightProbeDebugWidget>(PC, ULightProbeDebugWidget::StaticClass());
                if (DebugWidget)
                {
                    DebugWidget->RenderTarget = RenderTarget;
                    DebugWidget->ImageSize = DebugWidgetSize;
                    DebugWidget->ScreenOffset = DebugWidgetOffset;
                    DebugWidget->AddToViewport(1000);
                }
            }
        }
    }
#endif
}

void ULightLevelProbeComponent::ShutdownProbe()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }

    if (DebugWidget)
    {
        DebugWidget->RemoveFromParent();
        DebugWidget = nullptr;
    }

    if (ProbeCapture)
    {
        ProbeCapture->DestroyComponent();
        ProbeCapture = nullptr;
    }

    if (ProbeSpringArm)
    {
        ProbeSpringArm->DestroyComponent();
        ProbeSpringArm = nullptr;
    }

    if (ProbeMesh)
    {
        ProbeMesh->DestroyComponent();
        ProbeMesh = nullptr;
    }

    RenderTarget = nullptr;
}

void ULightLevelProbeComponent::UpdateLightLevel()
{
    if (!RenderTarget || !ProbeCapture) return;

    ProbeCapture->CaptureScene();

    const int32 SizeX = RenderTarget->SizeX;
    const int32 SizeY = RenderTarget->SizeY;
    if (SizeX <= 0 || SizeY <= 0) return;

    float NewLum = 0.0f;
    const int32 EffectiveSamples = FMath::Clamp(NumSamples, 1, 16);

    if (EffectiveSamples <= 3)
    {
        // Sample three faces of the cube: top, left, right.
        const float U_Top = 0.5f;
        const float V_Top = 0.20f;
        const float U_Left = 0.25f;
        const float V_Left = 0.80f;
        const float U_Right = 0.75f;
        const float V_Right = 0.80f;

        auto ToPixel = [SizeX, SizeY](float U, float V)
            {
                const int32 X = FMath::Clamp(FMath::RoundToInt(U * (SizeX - 1)), 0, SizeX - 1);
                const int32 Y = FMath::Clamp(FMath::RoundToInt(V * (SizeY - 1)), 0, SizeY - 1);
                return TPair<int32, int32>(X, Y);
            };

        const TPair<int32, int32> Top = ToPixel(U_Top, V_Top);
        const TPair<int32, int32> Left = ToPixel(U_Left, V_Left);
        const TPair<int32, int32> Right = ToPixel(U_Right, V_Right);

        const FColor PixelTop = UKismetRenderingLibrary::ReadRenderTargetPixel(this, RenderTarget, Top.Key, Top.Value);
        const FColor PixelLeft = UKismetRenderingLibrary::ReadRenderTargetPixel(this, RenderTarget, Left.Key, Left.Value);
        const FColor PixelRight = UKismetRenderingLibrary::ReadRenderTargetPixel(this, RenderTarget, Right.Key, Right.Value);

        const float LumTop = FLinearColor::FromSRGBColor(PixelTop).GetLuminance();
        const float LumLeft = FLinearColor::FromSRGBColor(PixelLeft).GetLuminance();
        const float LumRight = FLinearColor::FromSRGBColor(PixelRight).GetLuminance();

        NewLum = (LumTop + LumLeft + LumRight) / 3.0f;
    }
    else
    {
        const int32 GridSide = FMath::CeilToInt(FMath::Sqrt((float)EffectiveSamples));
        const float StepX = (float)SizeX / (GridSide + 1);
        const float StepY = (float)SizeY / (GridSide + 1);

        int32 SamplesTaken = 0;
        float TotalLuminance = 0.0f;

        for (int32 Row = 0; Row < GridSide && SamplesTaken < EffectiveSamples; ++Row)
        {
            for (int32 Col = 0; Col < GridSide && SamplesTaken < EffectiveSamples; ++Col)
            {
                const int32 X = FMath::Clamp(FMath::RoundToInt((Col + 1) * StepX), 0, SizeX - 1);
                const int32 Y = FMath::Clamp(FMath::RoundToInt((Row + 1) * StepY), 0, SizeY - 1);

                const FColor Pixel = UKismetRenderingLibrary::ReadRenderTargetPixel(this, RenderTarget, X, Y);
                const float Lum = FLinearColor::FromSRGBColor(Pixel).GetLuminance();
                TotalLuminance += Lum;
                ++SamplesTaken;
            }
        }

        if (SamplesTaken <= 0) return;
        NewLum = TotalLuminance / (float)SamplesTaken;
    }

    RawLuminance = NewLum;

    float Normalized = 0.0f;
    if (BrightReference > ShadowReference + KINDA_SMALL_NUMBER)
    {
        Normalized = FMath::GetRangePct(ShadowReference, BrightReference, RawLuminance);
    }
    Normalized = FMath::Clamp(Normalized, 0.0f, 1.0f);

    if (!FMath::IsNearlyEqual(ResponseCurveGamma, 1.0f) && ResponseCurveGamma > KINDA_SMALL_NUMBER)
    {
        Normalized = FMath::Pow(Normalized, ResponseCurveGamma);
        Normalized = FMath::Clamp(Normalized, 0.0f, 1.0f);
    }

    const float TargetValue = Normalized;

    if (SmoothingAlpha > 0.0f && SmoothingAlpha < 1.0f)
    {
        CurrentLightLevel = FMath::Lerp(CurrentLightLevel, TargetValue, SmoothingAlpha);
    }
    else
    {
        CurrentLightLevel = TargetValue;
    }

    // Feed the normalized light level into the SOTS player stealth component
    // if the owner has one attached. This keeps the LightProbe plugin decoupled
    // from the global stealth manager while still powering the stealth pipeline.
    if (AActor* Owner = GetOwner())
    {
        if (UActorComponent* RawComp = Owner->GetComponentByClass(USOTS_PlayerStealthComponent::StaticClass()))
        {
            if (USOTS_PlayerStealthComponent* StealthComp = Cast<USOTS_PlayerStealthComponent>(RawComp))
            {
                StealthComp->SetLightLevel(CurrentLightLevel);
            }
        }
    }
}
