#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourConfig.generated.h"

/**
 * Data-driven tuning for the simple SOTS_Parkour detect/execute flow.
 *
 * SPINE 4 keeps this focused on the basics:
 *   - How far / high we probe for a ledge.
 *   - Mantle height thresholds.
 *   - Max safe drop height.
 *   - Warp offsets for mantle/drop execution.
 *
 * Later passes can extend this with OmniTrace, anchors, and full TNT parity.
 */
UCLASS(BlueprintType)
class SOTS_PARKOUR_API USOTS_ParkourConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    USOTS_ParkourConfig();

    // --------------------------------------------------------------------
    // Detection / probe tuning
    // --------------------------------------------------------------------

    /** How far forward to trace when searching for a simple ledge/wall. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    float ForwardTraceDistance;

    /** Vertical offset above character origin for the forward trace. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    float TraceVerticalOffset;

    UPROPERTY(EditAnywhere, Category = "Parkour|LOD")
    float MinSpeedForDetection = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Parkour|LOD")
    float MaxCameraDistanceForDetection = 3000.0f;

    /** When true, runs parkour detection every tick (still respects the per-frame trace budget). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    bool bContinuousTraceMode;

    /** Maximum number of parkour traces allowed per frame (<=0 disables budgeting). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    int32 MaxTracesPerFrame;

    /** Randomized cooldown (seconds) applied while in climb to reduce trace load. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    float ClimbTraceCooldownMin = 0.2f;

    /** Randomized cooldown (seconds) applied while in climb to reduce trace load. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection")
    float ClimbTraceCooldownMax = 0.35f;

    /** Height delta between the authored CGF pawn and this character (affects IK/trace start heights). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Character")
    float CharacterHeightDiff = 17.0f;

    /** Additional character height adjustment used by retarget/warp helpers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Character")
    float CharacterHeightAdjustment = -15.0f;

    // --------------------------------------------------------------------
    // Classification thresholds
    // --------------------------------------------------------------------

    /** Minimum height above feet to consider something mantle-able. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Mantle")
    float MantleMinHeight;

    /** Maximum height above feet to consider something mantle-able. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Mantle")
    float MantleMaxHeight;

    /** Minimum height above feet to consider a low obstacle vault instead of mantle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultMinHeight;

    /** Maximum height above feet to still treat as vault before mantle takes over. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultMaxHeight;

    /** Minimum 2D speed required before selecting a vault-style action. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultMinSpeed = 19.99f;

    /** Maximum wall depth (trace distance into the surface) to allow a vault. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultMaxWallDepth = 120.0f;

    /** Minimum wall depth (trace distance into the surface) to allow a vault. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultMinWallDepth = 30.0f;

    /** Maximum wall depth to allow a mantle-style action. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Mantle")
    float MantleMaxWallDepth = 120.0f;

    /** Minimum wall depth to allow a mantle-style action. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Mantle")
    float MantleMinWallDepth = 30.0f;

    /** Height threshold to classify a very low vault as a thin vault. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultThinHeightMax = 60.0f;

    /** Height threshold to classify a tall vault as a high vault. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Vault")
    float VaultHighHeightMin = 120.0f;

    /**
     * Maximum distance BELOW the character's feet we consider a "safe drop"
     * in the current simple flow.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification|Drop")
    float MaxSafeDropHeight;

    /** Forward offset for the secondary/confirm traces that look back into the wall (negative = into wall). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Confirm")
    float SecondTraceForwardOffset = -40.0f;

    /** Vertical offset for secondary/confirm traces (negative drops below hit point). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Confirm")
    float SecondTraceVerticalOffset = -80.0f;

    /** Confirm capsule radius (CGF overlap ~25uu). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Confirm")
    float ConfirmCapsuleRadius = 25.0f;

    /** Confirm capsule half-height (CGF overlap ~82uu). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Confirm")
    float ConfirmCapsuleHalfHeight = 82.0f;

    /** Vault-only forward nudge for confirm overlap (negative pushes into wall). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Confirm")
    float ConfirmVaultForwardOffset = -40.0f;

    // --------------------------------------------------------------------
    // Back-hop tuning
    // --------------------------------------------------------------------

    /** Horizontal back-hop distance (primary). Mirrors CGF back-hop sweep length. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopDistance = 140.0f;

    /** Short inner back-hop distance (legacy CGF inner hop). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopInnerDistance = 55.0f;

    /** Acceptable deviation (degrees) from a vertical wall normal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopAngleToleranceDegrees = 8.0f;

    /** Lift applied to back-hop traces (Z). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopVerticalLift = 25.0f;

    /** Upwards offset from the wall impact used as the trace origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopTraceStartZOffset = 7.0f;

    /** Radius used for back-hop sweeps (smaller than generic ledge-move probes). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopProbeRadius = 2.5f;

    /** Minimum right input magnitude before allowing left/right back-hop windows. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopRightInputThreshold = 0.25f;

    /** Minimum forward input magnitude used for the straight-back back-hop window. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopForwardInputThreshold = 0.25f;

    /** Positive yaw window (degrees) between control and actor rotation to allow right-input back-hop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopYawMaxDegrees = 120.0f;

    /** Negative yaw window (degrees) between control and actor rotation to allow left-input back-hop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopYawMinDegrees = -120.0f;

    /** Deadzone around forward facing to exclude from strafe windows (degrees). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopYawCenterDeadzoneDegrees = 30.0f;

    /** Forward-facing exclusion: require |DeltaYaw| to exceed this for the forward-input back-hop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopForwardYawRejectDegrees = 150.0f;

    /** Capsule radius used to validate space at the back-hop landing/wrap point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopSurfaceCapsuleRadius = 25.0f;

    /** Capsule half-height used to validate space at the back-hop landing/wrap point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopSurfaceCapsuleHalfHeight = 82.0f;

    /** Forward offset from the wall (away from impact normal) for the surface clearance capsule. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    float BackHopSurfaceForwardOffset = 55.0f;

        /** Horizontal wall detect trace half-quantity (NotBusy state). Mirrors CGF HorizontalWallDetectTraceHalfQuantity. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        int32 HorizontalWallTraceHalfQuantity_NotBusy;

        /** Horizontal wall detect trace half-quantity (Climb state). Mirrors CGF HorizontalWallDetectTraceHalfQuantity in climb. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        int32 HorizontalWallTraceHalfQuantity_Climb;

        /** Horizontal wall detect trace range step (units) used per lateral slot. Mirrors CGF HorizontalWallDetectTraceRange. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        float HorizontalWallTraceRange;

        /** Vertical wall detect trace quantity (NotBusy state). Mirrors CGF VerticalWallDetectTraceQuantity. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        int32 VerticalWallTraceQuantity_NotBusy;

        /** Vertical wall detect trace quantity (Climb state). Mirrors CGF VerticalWallDetectTraceQuantity in climb. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        int32 VerticalWallTraceQuantity_Climb;

        /** Vertical wall detect trace range step (units) for the grid. Mirrors CGF VerticalWallDetectTraceRange. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
        float VerticalWallTraceRange;

            /** Whether to run the wall grid probes as overlaps (CGF overlap style) before sweeping forward. */
            UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Detection|Grid")
            bool bUseWallGridOverlap;

        /** Distance-from-ledge XY cutoff used to pick DistanceMantle-like behavior. Mirrors CGF 122uu cutoff. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Classification")
        float DistanceFromLedgeXYCutoff;

    /** Predictive jump forward offset from the initial surface point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|PredictJump")
    float PredictJumpForwardDistance = 120.0f;

    /** Downward probe depth used when searching for a predictive jump landing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|PredictJump")
    float PredictJumpProbeDepth = 120.0f;

    /** Sphere radius used for predictive jump landing probes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|PredictJump")
    float PredictJumpProbeRadius = 20.0f;

    /** Per-action trace profiles mirroring CGF Parkour.Action.* trace settings. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Detection")
    TMap<FGameplayTag, FSOTS_ParkourTraceProfile> ActionTraceProfiles;

    // --------------------------------------------------------------------
    // Ledge move / hop tuning
    // --------------------------------------------------------------------

    /** Horizontal sweep distance used for ledge-move and hop probes (CGF uses ~140 units). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LedgeMove")
    float LedgeMoveHorizontalDistance = 140.0f;

    /** Upwards offset applied when firing lateral ledge-move probes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LedgeMove")
    float LedgeMoveVerticalOffset = 25.0f;

    /** Radius for ledge-move/hop sphere sweeps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LedgeMove")
    float LedgeMoveProbeRadius = 5.0f;

    /** Vertical lift used for hop style traces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LedgeMove")
    float HopVerticalLift = 25.0f;

    /** Whether to require a clearance capsule check before accepting back-hop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|BackHop")
    bool bValidateBackHopSurface = true;

    // --------------------------------------------------------------------
    // Timing / linger tuning
    // --------------------------------------------------------------------

    /** Linger duration after vault-style actions before returning to idle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Timing")
    float LingerVaultSeconds = 0.35f;

    /** Linger duration after mantle-style actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Timing")
    float LingerMantleSeconds = 0.25f;

    /** Linger duration after corner/ledge move/hang actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Timing")
    float LingerHangSeconds = 0.20f;

    /** Linger duration after hops/tictac/predictive jumps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Timing")
    float LingerHopSeconds = 0.15f;

    // --------------------------------------------------------------------
    // Execution offsets
    // --------------------------------------------------------------------

    /**
     * Mantle forward offset multiplier.
     *   Final forward offset ≈ CapsuleRadius * MantleForwardOffsetScale.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Execution|Mantle")
    float MantleForwardOffsetScale;

    /**
     * Mantle vertical offset multiplier.
     *   Final up offset ≈ CapsuleHalfHeight * MantleUpOffsetScale.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Execution|Mantle")
    float MantleUpOffsetScale;

    /**
     * Fixed distance to move the character downward for a simple drop.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Execution|Drop")
    float DropStepDownDistance;

    // --------------------------------------------------------------------
    // IK tuning
    // --------------------------------------------------------------------

    /** Sphere probe radius for per-hand IK ledge checks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKProbeRadius = 5.0f;

    /** Horizontal offset from the ledge point used when searching for hand contacts (fallback when side-specific offsets are unset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKHorizontalOffset = 8.0f;

    /** Left hand horizontal offset from the ledge point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKHorizontalOffsetLeft = 0.0f;

    /** Right hand horizontal offset from the ledge point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKHorizontalOffsetRight = 0.0f;

    /** Vertical lift applied before sweeping down for hand contacts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKVerticalLift = 10.0f;

    /** Downward trace depth for hand contact validation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKTraceDepth = 60.0f;

    /** Minimum normal alignment (dot) to accept a hand surface. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKNormalDotThreshold = 0.3f;

    /** Interp speed used when blending hand IK targets (climb / ledge case). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKInterpSpeed = 0.2f;

    /** Interp speed used when blending hand IK targets while not climbing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Hand")
    float HandIKInterpSpeedNotClimb = 0.2f;

        /** Max consecutive hand IK failures before applying the failure offset (0 disables). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
        int32 HandIKMaxConsecutiveFailures = 0;

        /** Vertical offset applied when hand IK keeps failing. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
        float HandIKFailureOffsetZ = 0.0f;

        /** Max consecutive frames with no hand IK hit before skipping traces. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
        int32 HandIKMaxConsecutiveMissFrames = 2;

        /** Cooldown frames to skip hand IK after exceeding the miss budget. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
        int32 HandIKMissCooldownFrames = 1;

    /** Horizontal spacing used when deriving climb hand targets from the climb ledge hit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Climb")
    float ClimbIKHandSpacing = 8.0f;

    /** Forward/back offset from the ledge when placing climb IK targets (negative pushes into wall). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Climb")
    float ClimbIKForwardOffset = -20.0f;

    /** Vertical offset applied to climb IK targets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Climb")
    float ClimbIKUpOffset = 0.0f;

    /** Sphere probe radius for per-foot IK checks (default use). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKProbeRadius = 6.0f;

    /** Corner move variant of the foot IK probe radius (Parkour.Action.CornerMove). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKCornerMoveProbeRadius = 15.0f;

    /** Forward offset from the foot socket used when sweeping for contact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKForwardOffset = 40.0f;

    /** Right-hand offset from the foot socket (signed by foot side). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKRightOffset = 7.0f;

    /** Upwards lift applied before sweeping down to find the foot surface. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKVerticalLift = 10.0f;

    /** Downward sweep depth when searching for foot placement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK|Foot")
    float FootIKTraceDepth = 50.0f;

    // --------------------------------------------------------------------
    // Debug drawing toggles
    // --------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    bool bDrawForwardProbeDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    bool bDrawVaultMantleDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    bool bDrawLedgeAndBackHopDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    bool bDrawCornerDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    bool bDrawIKDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    float DebugLineLifetime = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Debug")
    float DebugShapeLifetime = 0.1f;
};
