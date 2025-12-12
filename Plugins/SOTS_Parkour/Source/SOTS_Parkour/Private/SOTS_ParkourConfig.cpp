#include "SOTS_ParkourConfig.h"
#include "GameplayTagContainer.h"

USOTS_ParkourConfig::USOTS_ParkourConfig()
{
    // Match the hard-coded SPINE 3 values so behavior remains identical
    // when you first assign this config.

    ForwardTraceDistance   = 150.0f;
    TraceVerticalOffset    = 40.0f;
    MinSpeedForDetection   = 5.0f;
    MaxCameraDistanceForDetection = 3000.0f;
    bContinuousTraceMode   = false;
    MaxTracesPerFrame      = 3;
    ClimbTraceCooldownMin  = 0.2f;
    ClimbTraceCooldownMax  = 0.35f;
    CharacterHeightDiff    = 17.0f;
    CharacterHeightAdjustment = -15.0f;

    MantleMinHeight        = 30.0f;
    MantleMaxHeight        = 200.0f; // CGF covers up to ~200uu

    VaultMinHeight         = 0.0f;   // CGF thin vault can start near ground
    VaultMaxHeight         = 200.0f; // CGF bands up to ~200uu
    VaultMinSpeed          = 19.99f;
    VaultMaxWallDepth      = 120.0f;
    VaultMinWallDepth      = 30.0f;
    MantleMaxWallDepth     = 120.0f;
    MantleMinWallDepth     = 30.0f;
    VaultThinHeightMax     = 60.0f;
    VaultHighHeightMin     = 120.0f;

    SecondTraceForwardOffset = -40.0f;
    SecondTraceVerticalOffset = -80.0f;
    ConfirmCapsuleRadius = 25.0f;
    ConfirmCapsuleHalfHeight = 82.0f;
    ConfirmVaultForwardOffset = -40.0f;

    MaxSafeDropHeight      = 240.0f;

    BackHopDistance                 = 140.0f;
    BackHopInnerDistance            = 55.0f;
    BackHopAngleToleranceDegrees    = 8.0f;
    BackHopVerticalLift             = 25.0f;
    BackHopTraceStartZOffset        = 7.0f;
    BackHopProbeRadius              = 2.5f;
    BackHopRightInputThreshold      = 0.25f;
    BackHopForwardInputThreshold    = 0.25f;
    BackHopYawMaxDegrees            = 120.0f;
    BackHopYawMinDegrees            = -120.0f;
    BackHopYawCenterDeadzoneDegrees = 30.0f;
    BackHopForwardYawRejectDegrees  = 150.0f;
    BackHopSurfaceCapsuleRadius     = 25.0f;
    BackHopSurfaceCapsuleHalfHeight = 82.0f;
    BackHopSurfaceForwardOffset     = 55.0f;

    HorizontalWallTraceHalfQuantity_NotBusy = 2;
    HorizontalWallTraceHalfQuantity_Climb   = 1;
    HorizontalWallTraceRange                = 20.0f;
    VerticalWallTraceQuantity_NotBusy       = 30;
    VerticalWallTraceQuantity_Climb         = 4;
    VerticalWallTraceRange                  = 20.0f;
    bUseWallGridOverlap                     = false;
    DistanceFromLedgeXYCutoff               = 122.0f;
    PredictJumpForwardDistance              = 120.0f;
    PredictJumpProbeDepth                   = 120.0f;
    PredictJumpProbeRadius                  = 20.0f;

    LedgeMoveHorizontalDistance = 140.0f;
    LedgeMoveVerticalOffset     = 25.0f;
    LedgeMoveProbeRadius        = 5.0f;
    HopVerticalLift             = 25.0f;
    bValidateBackHopSurface     = true;

    // BP ParkourCameraMove / settle curves observed values
    LingerVaultSeconds = 0.40f;  // longer settle after vault
    LingerMantleSeconds = 0.30f; // mantle settle slightly longer
    LingerHangSeconds   = 0.22f; // ledge/corner/hang settle
    LingerHopSeconds    = 0.18f; // hop/tictac/predictive settle

    MantleForwardOffsetScale = 0.8f;
    MantleUpOffsetScale      = 0.6f;

    DropStepDownDistance     = 150.0f;

    HandIKProbeRadius            = 5.0f;
    HandIKHorizontalOffset       = 8.0f;
    HandIKHorizontalOffsetLeft   = 0.0f;
    HandIKHorizontalOffsetRight  = 0.0f;
    HandIKVerticalLift           = 10.0f;
    HandIKTraceDepth             = 60.0f;
    HandIKNormalDotThreshold     = 0.3f;
    HandIKMaxConsecutiveFailures = 0;
    HandIKFailureOffsetZ         = 0.0f;
    HandIKMaxConsecutiveMissFrames = 2;
    HandIKMissCooldownFrames     = 1;

    ClimbIKHandSpacing           = 8.0f;
    ClimbIKForwardOffset         = -20.0f;
    ClimbIKUpOffset              = 0.0f;

    FootIKProbeRadius            = 6.0f;
    FootIKCornerMoveProbeRadius  = 15.0f;
    FootIKForwardOffset          = 40.0f;
    FootIKRightOffset            = 7.0f;
    FootIKVerticalLift           = 10.0f;
    FootIKTraceDepth             = 50.0f;

    bDrawForwardProbeDebug   = true;
    bDrawVaultMantleDebug    = true;
    bDrawLedgeAndBackHopDebug = true;
    bDrawCornerDebug         = true;
    bDrawIKDebug             = false;
    DebugLineLifetime        = 0.1f;
    DebugShapeLifetime       = 0.1f;

    // Base trace profiles (family defaults mirroring CGF numbers).
    {
        FSOTS_ParkourTraceProfile GroundProfile;
        GroundProfile.SourceId = TEXT("GroundForward");
        GroundProfile.CapsuleRadius = 40.0f;
        GroundProfile.CapsuleHalfHeight = 120.0f; // Not falling
        GroundProfile.ForwardDistanceMin = 50.0f;  // Not falling
        GroundProfile.ForwardDistanceMax = 200.0f; // Not falling
        GroundProfile.StartZOffset = 75.0f;        // Not falling
        GroundProfile.VerticalOffsetDown = 15.0f;  // Falling start Z
        GroundProfile.VerticalOffsetUp = 70.0f;    // Falling capsule half-height
        GroundProfile.MinLedgeHeight = 25.0f;      // Falling MinDistance
        GroundProfile.MaxLedgeHeight = 100.0f;     // Falling MaxDistance

        ActionTraceProfiles.Add(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.GroundForward"), false), GroundProfile);
    }

    {
        FSOTS_ParkourTraceProfile ClimbProfile;
        ClimbProfile.SourceId = TEXT("ClimbForward");
        ClimbProfile.CapsuleRadius = 20.0f;
        ClimbProfile.CapsuleHalfHeight = 20.0f;
        ClimbProfile.ForwardDistanceMax = 60.0f;
        ClimbProfile.StartZOffset = 0.0f;
        ClimbProfile.VerticalOffsetDown = 0.0f;
        ActionTraceProfiles.Add(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.ClimbForward"), false), ClimbProfile);
    }

    {
        FSOTS_ParkourTraceProfile MantleProfile;
        MantleProfile.SourceId = TEXT("Mantle");
        MantleProfile.CapsuleRadius = 25.0f;
        MantleProfile.StartZOffset = 8.0f;   // Upper sample: CapsuleHalfHeight + 8
        MantleProfile.VerticalOffsetDown = -8.0f; // Lower sample: CapsuleHalfHeight - 8
        ActionTraceProfiles.Add(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.Mantle"), false), MantleProfile);
    }

    {
        FSOTS_ParkourTraceProfile VaultProfile;
        VaultProfile.SourceId = TEXT("Vault");
        VaultProfile.CapsuleRadius = 25.0f;
        VaultProfile.StartZOffset = 18.0f; // Upper sample: CapsuleHalfHeight + 18
        VaultProfile.VerticalOffsetDown = 5.0f; // Lower sample: CapsuleHalfHeight + 5
        ActionTraceProfiles.Add(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.Vault"), false), VaultProfile);
    }

    {
        FSOTS_ParkourTraceProfile ConfirmProfile;
        ConfirmProfile.SourceId = TEXT("ConfirmForward");
        ConfirmProfile.CapsuleRadius = 25.0f;          // Mirrors BP follow-up capsule radius (25–27)
        ConfirmProfile.CapsuleHalfHeight = 82.0f;      // Mirrors BP follow-up half-height (~82–90)
        ConfirmProfile.ForwardDistanceMax = -40.0f;    // Negative to push into wall along -ImpactNormal
        ConfirmProfile.StartZOffset = -80.0f;          // Downward nudge below hit point
        ActionTraceProfiles.Add(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.ConfirmForward"), false), ConfirmProfile);
    }
}
