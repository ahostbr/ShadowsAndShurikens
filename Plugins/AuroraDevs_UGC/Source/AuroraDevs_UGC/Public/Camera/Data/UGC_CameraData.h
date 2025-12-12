// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "UGC_CameraData.generated.h"

/*
 * The settings of the camera yaw follow. This is useful for player who don't like to control the camera too much.
 * The YawFollowModifier in blueprint will adjust the yaw to face the movement direction (Used by AAA games like Hogwarts Legacy, Witcher, etc).
 */
USTRUCT(BlueprintType)
struct FCameraYawFollowSettings
{
    GENERATED_BODY()

    // Whether the yaw should follow the character, when they are moving.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Yaw Follow")
    bool bEnableYawMovementFollow = true;

    // Whether the yaw threshold timer should be reset when the character stop moving.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Yaw Follow")
    bool bResetThresholdTimerWhenNoMovement = false;

    // The speed at which the camera rotates its yaw in the movement direction of the character.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Yaw Follow")
    float YawFollowSpeed = 50.f;

    /* The minimum time the player shouldn't rotate the camera manually before the yaw follow kicks in.
     * The timer can be reset when the character stops moving by enabling `bResetThresholdTimerWhenNoMovement`.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Yaw Follow")
    float YawFollowTimeThreshold = 2.f;

    // Threshold yaw angle above which we trigger the yaw follow modifier.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Yaw Follow")
    float YawFollowAngleThreshold = 10.f;
};

/*
 * The settings of the camera pitch follow. This is useful for player who don't like to control the camera too much.
 * The PitchFollowModifier in blueprint will adjust the pitch to face slopes, falling and reset the pitch if it's left untouched for long enough.
 * (Used by AAA games like Red Dead Redemption, Hogwarts Legacy, Witcher, etc)
 */
USTRUCT(BlueprintType)
struct FCameraPitchFollowSettings
{
    GENERATED_BODY()

    /** Whether the pitch should reset to the resting pitch when the character is moving. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pitch Follow")
    bool bEnablePitchMovementFollow = true;

    /** The speed at which the camera rotates its pitch to follow when falling/moving. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pitch Follow")
    float PitchFollowSpeed = 10.f;

    /** The minimum time the player shouldn't rotate the camera manually before the pitch follow kicks in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pitch Follow")
    float PitchFollowTimeThreshold = 2.f;

    /** Whether the pitch threshold timer should be reset when the character stop moving. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pitch Follow")
    bool bResetThresholdTimerWhenNoMovement = false;

    /** Threshold pitch angle above which we trigger the pitch follow modifier to reset the pitch to RestingPitch. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Follow")
    float PitchFollowAngleThreshold = 5.f;

    /** The pitch to go to when the the character is moving without controlling the camera's pitch is messed up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Follow")
    float RestingPitch = -10.f;

    /** Should the camera look down when the character is falling for at least TimeThresholdWhenFalling in seconds and the floor is at least MinDistanceFromGround centimeters away ? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Falling")
    bool bTriggerWhenFalling = true;

    /** The minimun duration of time the character should fall before the modifier is triggered. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Falling", meta = (EditCondition = "bTriggerWhenFalling"))
    float PitchFollowTimeThresholdWhenFalling = 0.5f;

    /** The minimum distance we should be above the ground to trigger the pitch follow when falling. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Falling", meta = (EditCondition = "bTriggerWhenFalling"))
    float MinDistanceFromGroundToTriggerWhenFalling = 500.f;

    /** A multiplier to apply to the follow speed of the camera when we're feeling. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Falling", meta = (EditCondition = "bTriggerWhenFalling"))
    float SpeedMultiplierWhenFalling = 6.f;

    /** Should the camera pitch look toward the inclination of the slope the character is walking on? */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Slopes")
    bool bTriggerOnSlopes = true;

    /** The minimum slope pitch inclination angle so that the pitch follow modifier is triggered (so that it's not triggered for small bumps). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slopes", meta = (EditCondition = "bTriggerOnSlopes"))
    float SlopeMinIncline = 25.f;
};

/*
 * The settings of the camera Arm Offset.
 */
USTRUCT(BlueprintType)
struct FCameraArmOffsetSettings
{
    GENERATED_BODY()

    /** Offset at the end of the spring arm. Use this instead of the relative-space *rotation* so that the UE camera system works as expected. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true"))
    FVector ArmSocketOffset = FVector(0.f, 40.f, 0.f);

    /** How long does it take to blend to the current ArmSocketOffset. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float ArmSocketOffsetBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> ArmSocketOffsetBlendCurve;

    /** Offset at start of spring, applied in world space. Use this if you want a world-space offset from the parent component instead of the usual relative-space location. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true"))
    FVector ArmTargetOffset = FVector(0.f, 0.f, 58.f);

    /** How long does it take to blend to the current ArmTargetOffset. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float ArmTargetOffsetBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Offset Modifier", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> ArmTargetOffsetBlendCurve;
};

/*
 * This is used by the PitchToArmLengthAndFOV Camera Modifier in blueprint.
 * This makes the Arm Length and FOV change when the character is looking up/down (Used by many AAA games like all GTA games, Red Dead Redemption Assassin's Creed, Hogwarts Legacy, etc.).
 */
USTRUCT(BlueprintType)
struct FCameraPitchToArmAndFOVCurveSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Pitch to Arm and FOV Modifier", meta = (MultiLine = "true"))
    bool Enabled = true;

    /** Curve with X and Y between -1.0 and 1.0. This maps the LocalMinPitch (X=-1.0) and LocalMaxPitch (X=1.0) to the MinArmLength and MaxArmLength. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Pitch to Arm and FOV Modifier", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> PitchToArmLengthCurve;

    /** Curve with X and Y between -1.0 and 1.0. This maps the LocalMinPitch (X=-1.0) and LocalMaxPitch (X=1.0) to the MinFOV and MaxFOV. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Pitch to Arm and FOV Modifier", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> PitchToFOVCurve;
};

 /*
 * The settings of the camera Arm Length. Defines the range of the Arm Length and how to blend from one range to this one. This is used by the PitchToArmLengthAndFOV Camera Modifier in blueprint.
 * This makes the Arm Length change when the character is looking up/down (Used by many AAA games like all GTA games, Red Dead Redemption Assassin's Creed, Hogwarts Legacy, etc.).
 */
USTRUCT(BlueprintType)
struct FCameraArmLengthSettings
{
    GENERATED_BODY()

    /** The minimum arm length value. The length of the arm will change depending on the current pitch of the camera and the PitchToArmLengthCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Length", meta = (MultiLine = "true"))
    float MinArmLength = 100.f;

    /** The maximum arm length value. The length of the arm will change depending on the current pitch of the camera and the PitchToArmLengthCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Length", meta = (MultiLine = "true"))
    float MaxArmLength = 250.f;

    /** How long does it take to blend to the current Arm Length range. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Length", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float ArmRangeBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm Length", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> ArmRangeBlendCurve;
};

/*
 * The settings of the camera FOV. Defines the range of the FOV and how to blend from one range to this one. This is used by the PitchToArmLengthAndFOV Camera Modifier in blueprint.
 * This makes the FOV change when the character is looking up/down (Used by many AAA games like GTA IV).
 */
USTRUCT(BlueprintType)
struct FCameraFOVSettings
{
    GENERATED_BODY()

    /** The minimum FOV value. The FOV of the camera will change depending on the current pitch of the camera and the PitchToFOVCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera FOV", meta = (MultiLine = "true"))
    float MinFOV = 90.f;

    /** The maximum FOV value. The FOV of the camera will change depending on the current pitch of the camera and the PitchToFOVCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera FOV", meta = (MultiLine = "true"))
    float MaxFOV = 125.f;

    /** A tolerance in degrees above which the Angle Constraints modifier will start decelerating the camera. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera FOV", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float FOVRangeBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera FOV", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> FOVRangeBlendCurve;
};

/*
 * The settings of the camera yaw contraints. This is useful to limit how much the character can look left/right.
 * The looking input action needs to have a UGC_CameraTurnRateModifier for the camera to decelerate when close to the constraints.
 */
USTRUCT(BlueprintType)
struct FCameraYawConstraintSettings
{
    GENERATED_BODY()

    /** Whether the yaw should be constrained to LocalMinYaw and LocalMaxYaw. If the yaw was already out of the new range, it will blend into range using the BlendTime and BlendCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    bool bConstrainYaw = false;

    /** How close in degrees to the YawMin or YawMax should the camera start decelerating? */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float YawConstraintTolerance = 10.f;

    /** How much can the character look right (angle in degrees). */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    float LocalMinYaw = 0.f;

    /** How much can the character look left (angle in degrees). */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    float LocalMaxYaw = 359.998993f;

    /** How fast should we blend from one Yaw constraint range to this one. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float YawConstraintsBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> YawConstraintsBlendCurve;
};

/*
 * The settings of the camera pitch contraints. This is useful to limit how much the character can look up/down. AAA games usually do not allow the entire 180 degrees range.
 * The looking input action needs to have a UGC_CameraTurnRateModifier for the camera to decelerate when close to the constraints.
 */
USTRUCT(BlueprintType)
struct FCameraPitchConstraintSettings
{
    GENERATED_BODY()

    /** Whether the pitch should be constrained to LocalMinPitch and LocalMaxPitch. If the yaw was already out of the new range, it will blend into range using the BlendTime and BlendCurve. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    bool bConstrainPitch = false;

    /** How close in degrees to the PitchMin or PitchMax should the camera start decelerating? */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float PitchConstraintTolerance = 10.f;

    // How much can the character look down (angle in degrees).
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    float LocalMinPitch = -89.99f;

    // How much can the character look up (angle in degrees).
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    float LocalMaxPitch = 89.99f;

    /** How fast should we blend from one Pitch constraint range to this one. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true", UIMin = "0", ClampMin = "0"))
    float PitchConstraintsBlendTime = 0.5f;

    /** Controls the acceleration/deceleration of the blend. The curve has to be normalized (going from 0 to 1). Leave empty or use Hermite if unsure. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints", meta = (MultiLine = "true"))
    TObjectPtr<UCurveFloat> PitchConstraintsBlendCurve;
};

/*
 * The settings of the focus camera. This is used for hard-lock in games.
 * Has a function which retrieves the target we want the camera to look at. (Uses the Strategy Design Pattern)
 */
USTRUCT(BlueprintType)
struct FUGCCameraFocusSettings
{
    GENERATED_BODY()

    /** Whether the focus camera is enabled. If it is, this will use the Focus Target Method to get the target's location.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    bool bEnabled = false;

    /** How fast the camera will focus the target.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    float InterpSpeed = 10.f;

    /** An offset in degrees applied on the pitch and yaw. This is useful if you want the focused location to always be on the left/right or looked down/up.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    FRotator RotationOffset = FRotator::ZeroRotator;

    /** Whether the camera input should be ignored during focus.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    bool bIgnoreCameraInput = false;


    /** Whether we should rotatet only the yaw angle of the camera or the pitch as well.
     * You can set this to true and combine it with the Rotation Offset if you want the cam to stay at a specific pitch. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    bool bRotateYawOnly = false;

    /** Whether we should stop focusing if the Line of Sight is occluded.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    bool bStopIfBlockedLOS = false;

    /** How long should the line of sight be occluded before we stop focusing trying to focus target.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true", EditCondition="bStopIfBlockedLOS"))
    float BlockedLOSTimeThreshold = 3.f;

    /** The distance from the target below which we will stop focusing the target. If this is too low, the camera might start rotating crazily around the target.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    float MinDistanceFromTarget = 100.f;

    /** The distance from the target above which we will stop focusing the target.*/
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", meta = (MultiLine = "true"))
    float MaxDistanceFromTarget = 1000.f;

    /** Function which retrieves the target we want the camera to look at. (Uses the Strategy Design Pattern) */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", Instanced, meta = (MultiLine = "true"))
    TObjectPtr<class UUGC_IFocusTargetMethod> FocusTargetMethod;
};

/*
 * The settings of a camera modifier which needs to retrieve a list of actors.
 * Has a function which retrieves the all actors relevant for some camera modifier with settings inheriting from this class. (Uses the Strategy Design Pattern)
 */
USTRUCT(BlueprintType)
struct FUGCCameraSettingsWithGetActorsMethod
{
    GENERATED_BODY()

    /** Function which retrieves the all actors relevant for some camera modifier with settings inheriting from this class. (Uses the Strategy Design Pattern) */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Focus", Instanced, meta = (MultiLine = "true"))
    TObjectPtr<class UUGC_IGetActorsMethod> GetActorsMethod;
};

/*
 * Dithering settings used to dither/hide/fade objects colliding with the camera either occluding the line of sight to the player or overlapping the camera directly.
 */
USTRUCT(BlueprintType)
struct FCameraDitheringSettings
{
    GENERATED_BODY()

    // The name of the scalar material parameter to blend in/out.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering")
    FName MaterialOpacityParameterName = "Opacity";

    // Whether the material has a vector parameter which should be updated to the player location.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering")
    bool bUpdateMaterialPlayerPosition = false;

    // The name of the vector parameter to set to the player position inside the Material Parameter Collection.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering", meta = (EditCondition = "bUpdateMaterialPlayerPosition"))
    FName MaterialPlayerPositionParameterName = "PlayerLocation";

    // The minimum value of the material's Opacity when dithered.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering", meta = (UIMin = 0.f, UIMax = 1.f, ClampMin = 0.f, ClampMax = 1.f))
    float MaterialDitherMinimum = 0.1f;

    // Controls the speed of the blend when starting to dither an object.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering", meta = (UIMin = 0.f, ClampMin = 0.f))
    float DitherInSpeed = 10.f;

    // Controls the speed of the blend when finishing to dither an object.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering", meta = (UIMin = 0.f, ClampMin = 0.f))
    float DitherOutSpeed = 10.f;

    // Actors with this tag will not be dithered even if they overlap DitherLOSChannel and DitherOverlapChannel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering")
    FName IgnoreDitheringTag = "UGC_IgnoreDithering";

    // Whether we should dither objects that block the LINE OF SIGHT of the camera.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Line Of Sight")
    bool bDitherLineOfSight = false;

    // The collision channel to use when checking for what the LINE OF SIGHT of the camera is colliding with. The other objects have to Overlap this channel in order to be dithered!
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Line Of Sight", meta = (EditCondition = "bDitherLineOfSight"))
    TEnumAsByte<ECollisionChannel> DitherLOSChannel = ECC_Camera;

    // The width of the probe when testing if any object is blocking the LINE OF SIGHT from the player to the camera. The other objects have to Overlap this channel in order to be dithered!
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Line Of Sight", meta = (UIMin = 0.f, ClampMin = 0.f, EditCondition = "bDitherLineOfSight"))
    float LOSProbeSize = 5.f;

    // The minimum amount of time the camera LINE OF SIGHT has to collide with an object for it to be dithered.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Line Of Sight", meta = (UIMin = 0.f, ClampMin = 0.f, EditCondition = "bDitherLineOfSight"))
    float CollisionTimeThreshold = 0.f;

    // Whether we should dither objects that OVERLAP the actual camera component.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Overlaps")
    bool bDitherOverlaps = true;

    // Whether we should dither the owner character when the actual camera overlaps with them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Overlaps", meta = (EditCondition = "bDitherOverlaps"))
    bool bDitherOwner = true;

    // The collision channel to use when checking for what the actual camera is overlapping with. The other objects have to Overlap this channel in order to be dithered!
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Overlaps", meta = (EditCondition = "bDitherOverlaps"))
    TEnumAsByte<ECollisionChannel> DitherOverlapChannel = ECC_Camera;

    // The radius around the camera where we check for collisions.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC Camera Dithering|Collisions|Overlaps", meta = (UIMin = 0.f, ClampMin = 0.f, EditCondition = "bDitherOverlaps"))
    float SphereCollisionRadius = 15.f;
};

/**
 * Struct defining a feeler ray used for camera penetration avoidance. The feeler uses sphere sweeps.
 */
USTRUCT(BlueprintType)
struct FPenetrationAvoidanceFeeler
{
    GENERATED_BODY()
public:
    FPenetrationAvoidanceFeeler();

    FPenetrationAvoidanceFeeler(const FRotator& InAdjustmentRot, const float& InWorldWeight, const float& InPawnWeight, const float& InExtent);

    // FRotator describing deviance from main ray.
    UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceFeeler)
    FRotator AdjustmentRot;

    // How much this feeler affects the final position if it hits the world.
    UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceFeeler, meta = (UIMin = 0.f, UIMax = 1.f, ClampMin = 0.f, ClampMax = 1.f))
    float WorldWeight = 0.f;

    // How much this feeler affects the final position if it hits a APawn (setting to 0 will not attempt to collide with pawns at all).
    UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceFeeler, meta = (UIMin = 0.f, UIMax = 1.f, ClampMin = 0.f, ClampMax = 1.f))
    float PawnWeight = 0.f;

    // The radius of this feeler probe.
    UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceFeeler)
    float ProbeRadius = 5.f;
};

/**
 * Camera collision settings which define how the camera avoidance uses collision feelers or whiskers to avoid camera clipping through obstacles.
 */
USTRUCT(BlueprintType)
struct FCameraCollisionSettings
{
    GENERATED_BODY()
public:
    FCameraCollisionSettings();

    /* The time the camera takes to go to the safe location after a collision has been detected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float PenetrationBlendInTime = 0.1f;

    /* The time the camera takes to go back to its normal position after the collision has finished. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float PenetrationBlendOutTime = 0.15f;

    // If true, does collision checks to keep the camera out of the world.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    bool bPreventPenetration = true;

    // If true, try to detect nearby walls and move the camera in anticipation.  Helps prevent popping.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision", meta = (EditCondition = "bPreventPenetration"))
    bool bDoPredictiveAvoidance = true;

    /**
     * These are the feeler rays that are used to find where to place the camera.
     * Index: 0  : This is the normal feeler we use to prevent collisions.
     * Index: 1+ : These feelers are used if you bDoPredictiveAvoidance=true, to scan for potential impacts if the player
     *             were to rotate towards that direction and primitively collide the camera so that it pulls in before
     *             impacting the occluder.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    TArray<FPenetrationAvoidanceFeeler> PenetrationAvoidanceFeelers;

    // Actors with this tag will ignore camera collisions.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    FName IgnoreCameraCollisionTag = "UGC_IgnoreCameraCollision";
};


/**
 * Generic data class which holds settings for an associated Camera Add-On Modifier.
 */
UCLASS(abstract, Category = "UGC|Add On|Settings", EditInlineNew, Blueprintable)
class AURORADEVS_UGC_API UUGC_CameraAddOnModifierSettings : public UObject
{
    GENERATED_BODY()
public:

    // Modifier class associated to these settings.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TSubclassOf<class UUGC_CameraAddOnModifier> ModifierClass;
};

/** Camera Data Asset holding all of the camera settings. */
UCLASS(Blueprintable, BlueprintType)
class UUGC_CameraDataAssetBase : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    /**
     * The settings of the camera Arm Length. Defines the range of the Arm Length and how to blend from one range to this one. This is used by the PitchToArmLengthAndFOV Camera Modifier in blueprint.
     * This makes the Arm Length change when the character is looking up/down (Used by many AAA games like all GTA games, Red Dead Redemption Assassin's Creed, Hogwarts Legacy, etc.).
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm")
    FCameraArmLengthSettings ArmLengthSettings;

    /**
     * The settings of the camera FOV. Defines the range of the FOV and how to blend from one range to this one. This is used by the PitchToArmLengthAndFOV Camera Modifier in blueprint.
     * This makes the FOV change when the character is looking up/down (Used by many AAA games like GTA IV).
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera FOV")
    FCameraFOVSettings FOVSettings;

    /** Camera collision settings which define how the camera avoidance uses collision feelers or whiskers to avoid camera clipping through obstacles. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CameraCollisions")
    FCameraCollisionSettings CollisionSettings;

    /** Dithering settings used to dither/hide/fade objects colliding with the camera either occluding the line of sight to the player or overlapping the camera directly. */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Dithering")
    FCameraDitheringSettings DitheringSettings;

    /**
     * The settings of the camera pitch follow. This is useful for player who don't like to control the camera too much.
     * The PitchFollowModifier in blueprint will adjust the pitch to face slopes, falling and reset the pitch if it's left untouched for long enough.
     * (Used by AAA games like Red Dead Redemption, Hogwarts Legacy, Witcher, etc)
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Movement Follow|Pitch")
    FCameraPitchFollowSettings PitchFollowSettings;

    /**
     * The settings of the camera yaw follow. This is useful for player who don't like to control the camera too much.
     * The YawFollowModifier in blueprint will adjust the yaw to face the movement direction (Used by AAA games like Hogwarts Legacy, Witcher, etc).
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Movement Follow|Yaw")
    FCameraYawFollowSettings YawFollowSettings;

    /** The settings of the camera Spring Arm Offset. Offset at the end of the spring arm. Use this instead of the relative world location so that the UE camera system works as exptected */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Arm")
    FCameraArmOffsetSettings ArmOffsetSettings;

    /**
     * The settings of the camera pitch contraints. This is useful to limit how much the character can look up/down. AAA games usually do not allow the entire 180 degrees range.
     * The looking input action needs to have a UGC_CameraSlowDownInputModifier for the camera to decelerate when close to the constraints.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints")
    FCameraPitchConstraintSettings PitchConstraints;

    /**
     * The settings of the camera yaw contraints. This is useful to limit how much the character can look left/right.
     *  The looking input action needs to have a UGC_CameraSlowDownInputModifier for the camera to decelerate when close to the constraints.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Camera Angle Constraints")
    FCameraYawConstraintSettings YawConstraints;

    /**
     * The settings of the focus camera. This is used for hard-lock in games.
     * Has a function which retrieves the target we want the camera to look at. (Uses the Strategy Design Pattern)
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CameraFocus")
    FUGCCameraFocusSettings FocusSettings;

    /** This makes the Arm Length and FOV change when the character is looking up/down (Used by many AAA games like all GTA games, Red Dead Redemption Assassin's Creed, Hogwarts Legacy, etc.). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PitchToArmLengthAndFOVCurv")
    FCameraPitchToArmAndFOVCurveSettings PitchToArmAndFOVCurveSettings;

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Add Ons", Instanced, meta = (MultiLine = "true"))
    TArray<TObjectPtr<UUGC_CameraAddOnModifierSettings>> AddOnsSettings;
};