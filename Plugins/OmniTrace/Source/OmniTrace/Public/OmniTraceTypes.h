#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

#include "OmniTraceTypes.generated.h"

class AActor;
class UPrimitiveComponent;

/**
 * Public, Blueprint-facing hit result used by OmniTrace.
 * This intentionally does not expose FHitResult directly.
 */
USTRUCT(BlueprintType)
struct FOmniTraceHitResult
{
    GENERATED_BODY()

    /** Whether this represents a blocking hit. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    bool bBlockingHit = false;

    /** End location of the trace (equivalent to FHitResult.Location). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    FVector Location = FVector::ZeroVector;

    /** Impact point if applicable (equivalent to FHitResult.ImpactPoint). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    FVector ImpactPoint = FVector::ZeroVector;

    /** Impact normal. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    FVector Normal = FVector::UpVector;

    /** Actor hit (can be null). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    TWeakObjectPtr<AActor> HitActor;

    /** Component hit (can be null). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    TWeakObjectPtr<UPrimitiveComponent> HitComponent;

    /** Distance along the trace. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    float Distance = 0.0f;
};

// Helper to convert internal FHitResult traces into the public OmniTrace
// hit representation. Implemented in a corresponding .cpp file.
struct FHitResult;
FOmniTraceHitResult MakeOmniTraceHitResult(const FHitResult& InHit);

/**
 * Trace shape used by OmniTrace.
 */
UENUM(BlueprintType)
enum class EOmniTraceShape : uint8
{
    Line        UMETA(DisplayName = "Line Trace"),
    SphereSweep UMETA(DisplayName = "Sphere Sweep"),
    BoxSweep    UMETA(DisplayName = "Box Sweep"),
    CapsuleSweep UMETA(DisplayName = "Capsule Sweep"),
};

/**
 * High-level pattern family for ray generation.
 */
UENUM(BlueprintType)
enum class EOmniTraceTraceFamily : uint8
{
    Forward     UMETA(DisplayName = "Forward Fan / Cone"),
    Target      UMETA(DisplayName = "Target Arc / Fan"),
    Orbit       UMETA(DisplayName = "Orbit Around Target / Origin"),
    Radial3D    UMETA(DisplayName = "3D Radial Burst"),
};

/**
 * Forward pattern variants.
 */
UENUM(BlueprintType)
enum class EOmniTraceForwardPattern : uint8
{
    SingleRay   UMETA(DisplayName = "Single Ray"),
    MultiSpread UMETA(DisplayName = "Multi Spread / Fan"),
};

/**
 * Debug draw options for OmniTrace.
 */
USTRUCT(BlueprintType)
struct FOmniTraceDebugOptions
{
    GENERATED_BODY()

    /** Enable/disable debug drawing for this trace request. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Debug")
    bool bEnableDebug = false;

    /** Debug color for rays / hits. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Debug")
    FLinearColor Color = FLinearColor::Green;

    /** Lifetime for debug primitives. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Debug")
    float Lifetime = 0.25f;

    /** Line thickness for debug rays. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Debug")
    float Thickness = 1.5f;
};

/**
 * A single ray result used inside FOmniTracePatternResult.
 */
USTRUCT(BlueprintType)
struct FOmniTraceSingleRayResult
{
    GENERATED_BODY()

    /** Index of this ray in the pattern. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    int32 RayIndex = 0;

    /** Ray start location. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    FVector Start = FVector::ZeroVector;

    /** Ray end location (either hit location or max distance). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    FVector End = FVector::ZeroVector;

    /** Whether this ray hit something. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    bool bHit = false;

    /** Hit result (only meaningful if bHit == true). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    FOmniTraceHitResult Hit;
};

/**
 * Result container for an OmniTrace pattern call.
 * Mirrors what OmniTraceBlueprintLibrary::OmniTrace_Pattern fills in.
 */
USTRUCT(BlueprintType)
struct FOmniTracePatternResult
{
    GENERATED_BODY()

    /** All rays that were fired for this pattern. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    TArray<FOmniTraceSingleRayResult> Rays;

    /** Total number of rays fired. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    int32 TotalRays = 0;

    /** Whether ANY ray hit something. */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    bool bAnyHit = false;

    /** First blocking hit encountered (if any). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    FOmniTraceHitResult FirstBlockingHit;

    /** Nearest hit to the origin (if any). */
    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Result")
    FOmniTraceHitResult NearestHit;
};

/**
 * High-level request describing a pattern generation job.
 * This struct matches the fields used in OmniTraceBlueprintLibrary.cpp.
 */
USTRUCT(BlueprintType)
struct FOmniTraceRequest
{
    GENERATED_BODY()

    /** Pattern family to use when generating directions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    EOmniTraceTraceFamily PatternFamily = EOmniTraceTraceFamily::Forward;

    /** Forward pattern variant (used when PatternFamily == Forward). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    EOmniTraceForwardPattern ForwardPattern = EOmniTraceForwardPattern::MultiSpread;

    /** What trace shape to use when firing each ray. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    EOmniTraceShape Shape = EOmniTraceShape::Line;

    /** Radius used for sphere/capsule sweeps (defaults match previous OmniTrace behavior). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request", meta=(ClampMin="0.0"))
    float ShapeRadius = 25.0f;

    /** Half-extents used for box sweeps (X=forward). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    FVector BoxHalfExtents = FVector(30.0f, 30.0f, 60.0f);

    /** Capsule half-height used when Shape == CapsuleSweep. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request", meta=(ClampMin="0.0"))
    float CapsuleHalfHeight = 60.0f;

    /** Collision channel used for all traces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    /** Whether to trace complex collision. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    bool bTraceComplex = false;

    /** Maximum distance for each ray. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    float MaxDistance = 2000.0f;

    /** Number of rays to fire for this pattern. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    int32 RayCount = 11;

    /** Spread angle in degrees for forward cone patterns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request",
        meta=(EditCondition="PatternFamily == EOmniTraceTraceFamily::Forward"))
    float SpreadAngleDegrees = 45.0f;

    /** Arc angle in degrees for target-centred patterns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request",
        meta=(EditCondition="PatternFamily == EOmniTraceTraceFamily::Target"))
    float ArcAngleDegrees = 90.0f;

    /** Orbit radius used when PatternFamily == Orbit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request",
        meta=(EditCondition="PatternFamily == EOmniTraceTraceFamily::Orbit"))
    float OrbitRadius = 500.0f;

    /** Optional origin actor (takes priority over location override). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    TWeakObjectPtr<AActor> OriginActor;

    /** Optional origin component (highest priority for origin). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    TWeakObjectPtr<USceneComponent> OriginComponent;

    /** Optional target actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    TWeakObjectPtr<AActor> TargetActor;

    /** Fallback origin location override (if no actor/component). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    FVector OriginLocationOverride = FVector::ZeroVector;

    /** Fallback origin rotation override (if no actor/component). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    FRotator OriginRotationOverride = FRotator::ZeroRotator;

    /** Fallback target location override (if no target actor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    FVector TargetLocationOverride = FVector::ZeroVector;

    /** Extra actors to ignore while tracing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    TArray<TObjectPtr<AActor>> ActorsToIgnore;

    /** Debug draw settings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Request")
    FOmniTraceDebugOptions DebugOptions;
};

/**
 * Lightweight info used to expose presets to Blueprints / UI.
 * This matches OmniTraceBlueprintLibrary::OmniTrace_GetLibraryPresetInfos.
 */
USTRUCT(BlueprintType)
struct FOmniTracePresetInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Preset")
    FName PresetId;

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Preset")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Preset")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Preset")
    FName Category;

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace|Preset")
    FLinearColor FamilyColor = FLinearColor::Green;
};
/**
 * Built-in preset high level category.
 */
UENUM(BlueprintType)
enum class EOmniTraceBuiltinPresetCategory : uint8
{
    Vision      UMETA(DisplayName = "Vision / FOV"),
    Coverage    UMETA(DisplayName = "Coverage / Area Scan"),
    Orbit       UMETA(DisplayName = "Orbit / Perimeter"),
    Debug       UMETA(DisplayName = "Debug / Utility"),
};

/**
 * Built-in vision presets (forward cones, target arcs, etc).
 */
UENUM(BlueprintType)
enum class EOmniTraceVisionBuiltinPreset : uint8
{
    Fwd_Cone_Close_5R        UMETA(DisplayName = "Forward Cone – Close (5R)"),
    Fwd_Cone_Medium_11R      UMETA(DisplayName = "Forward Cone – Medium (11R)"),
    Fwd_Cone_Long_21R        UMETA(DisplayName = "Forward Cone – Long (21R)"),
    Target_Arc_Short_9R      UMETA(DisplayName = "Target Arc – Short (9R)"),
    Target_Fan_Wide_21R      UMETA(DisplayName = "Target Fan – Wide (21R)"),
};

/**
 * Built-in coverage presets (radial bursts, etc).
 */
UENUM(BlueprintType)
enum class EOmniTraceCoverageBuiltinPreset : uint8
{
    Radial_Sparse_64R        UMETA(DisplayName = "Radial Burst – Sparse (64R)"),
    Radial_Dense_128R        UMETA(DisplayName = "Radial Burst – Dense (128R)"),
    Radial_VeryDense_256R    UMETA(DisplayName = "Radial Burst – Very Dense (256R)"),
};

/**
 * Built-in orbit presets (rings around a point).
 */
UENUM(BlueprintType)
enum class EOmniTraceOrbitBuiltinPreset : uint8
{
    Orbit_SingleRing_24R     UMETA(DisplayName = "Orbit – Single Ring (24R)"),
    Orbit_SingleRing_48R     UMETA(DisplayName = "Orbit – Single Ring (48R)"),
    Orbit_MultiRing_3x24R    UMETA(DisplayName = "Orbit – Multi Ring (3 x 24R)"),
};

/**
 * Built-in debug / utility presets (simple forward rays, etc).
 */
UENUM(BlueprintType)
enum class EOmniTraceDebugBuiltinPreset : uint8
{
    Debug_Fwd_LongRay        UMETA(DisplayName = "Debug – Forward Long Ray"),
    Debug_Fwd_WideCone_9R    UMETA(DisplayName = "Debug – Wide Cone (9R)"),
};

/**
 * Structured handle for selecting a built-in preset in Blueprints.
 * Only the enum matching Category will be editable (via EditCondition).
 */
USTRUCT(BlueprintType)
struct FOmniTraceBuiltinPresetKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Presets")
    EOmniTraceBuiltinPresetCategory Category = EOmniTraceBuiltinPresetCategory::Vision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Presets",
        meta=(EditCondition="Category == EOmniTraceBuiltinPresetCategory::Vision"))
    EOmniTraceVisionBuiltinPreset VisionPreset = EOmniTraceVisionBuiltinPreset::Fwd_Cone_Medium_11R;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Presets",
        meta=(EditCondition="Category == EOmniTraceBuiltinPresetCategory::Coverage"))
    EOmniTraceCoverageBuiltinPreset CoveragePreset = EOmniTraceCoverageBuiltinPreset::Radial_Sparse_64R;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Presets",
        meta=(EditCondition="Category == EOmniTraceBuiltinPresetCategory::Orbit"))
    EOmniTraceOrbitBuiltinPreset OrbitPreset = EOmniTraceOrbitBuiltinPreset::Orbit_SingleRing_24R;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Presets",
        meta=(EditCondition="Category == EOmniTraceBuiltinPresetCategory::Debug"))
    EOmniTraceDebugBuiltinPreset DebugPreset = EOmniTraceDebugBuiltinPreset::Debug_Fwd_LongRay;
};

/**
 * High-level grouping used for UI, switches, and validation.
 * This should stay STABLE – add new families at the end only.
 */
UENUM(BlueprintType)
enum class EOmniTracePatternFamily : uint8
{
    Line        UMETA(DisplayName = "Line / Segment"),
    Arc         UMETA(DisplayName = "Arc / Circular"),
    Orbit       UMETA(DisplayName = "Orbit / Spiral"),
    Grid        UMETA(DisplayName = "Grid / Lattice"),
    NoiseWalk   UMETA(DisplayName = "Noise Walk / Random Path"),
    Radial      UMETA(DisplayName = "Radial / Starburst"),
    Scatter     UMETA(DisplayName = "Scatter / Sampling"),
    Spline      UMETA(DisplayName = "Spline Conform"),
    Volume      UMETA(DisplayName = "Volume Fill"),
};

/**
 * Line / segment patterns – everything that feels "linear".
 */
UENUM(BlueprintType)
enum class EOmniTraceLinePattern : uint8
{
    SimpleLine      UMETA(DisplayName = "Simple Line"),
    MultiSegment    UMETA(DisplayName = "Multi Segment Line"),
    ZigZag          UMETA(DisplayName = "Zig-Zag"),
    PingPong        UMETA(DisplayName = "Ping-Pong (Back & Forth)"),
    BezierLike      UMETA(DisplayName = "Bezier-like Curve"),
    StairStep       UMETA(DisplayName = "Stair-Step / Manhattan"),
};

/**
 * Arc patterns – short circular segments, fans, and the F-set arcs.
 */
UENUM(BlueprintType)
enum class EOmniTraceArcPattern : uint8
{
    SingleArc       UMETA(DisplayName = "Single Arc"),
    TargetMultiArc  UMETA(DisplayName = "Target Multi-Arc"),
    OrbitArc        UMETA(DisplayName = "Orbit Arc"),
    Bidirectional   UMETA(DisplayName = "Bidirectional Arc"),
    ArcFan          UMETA(DisplayName = "Arc Fan (Radial Fan)"),
};

/**
 * Orbit patterns – continuous orbital motion / long-span spirals.
 */
UENUM(BlueprintType)
enum class EOmniTraceOrbitPattern : uint8
{
    CircularOrbit   UMETA(DisplayName = "Circular Orbit"),
    EllipticalOrbit UMETA(DisplayName = "Elliptical Orbit"),
    SpiralIn        UMETA(DisplayName = "Spiral In"),
    SpiralOut       UMETA(DisplayName = "Spiral Out"),
    FigureEight     UMETA(DisplayName = "Figure Eight / Lissajous"),
};

/**
 * Grid / lattice sweeps – perfect for waypoints, patrols, coverage.
 */
UENUM(BlueprintType)
enum class EOmniTraceGridPattern : uint8
{
    RowScan         UMETA(DisplayName = "Row Scan (Left-to-Right)"),
    ColumnScan      UMETA(DisplayName = "Column Scan (Bottom-to-Top)"),
    SnakeRows       UMETA(DisplayName = "Snake Rows (Boustrophedon)"),
    SnakeColumns    UMETA(DisplayName = "Snake Columns"),
    SpiralOut       UMETA(DisplayName = "Grid Spiral Out"),
    Checkerboard    UMETA(DisplayName = "Checkerboard Cells"),
    RandomCells     UMETA(DisplayName = "Random Cells"),
};

/**
 * Noise-driven "walks" – for natural-looking wandering paths.
 */
UENUM(BlueprintType)
enum class EOmniTraceNoiseWalkPattern : uint8
{
    RandomWalk      UMETA(DisplayName = "Pure Random Walk"),
    PerlinDrift     UMETA(DisplayName = "Perlin Drift Walk"),
    JitteredLine    UMETA(DisplayName = "Jittered Line"),
    BrownianLoop    UMETA(DisplayName = "Brownian Loop / Return-To-Start"),
    BranchingWalk   UMETA(DisplayName = "Branching Walk"),
};

/**
 * Radial / starburst motifs – great for hubs, AOEs, and layouts.
 */
UENUM(BlueprintType)
enum class EOmniTraceRadialPattern : uint8
{
    Starburst           UMETA(DisplayName = "Starburst"),
    RadialSpokes        UMETA(DisplayName = "Radial Spokes"),
    RadialRings         UMETA(DisplayName = "Radial Rings"),
    GoldenAngleSpiral   UMETA(DisplayName = "Golden-Angle Spiral"),
};

/**
 * Scatter / sampling – point clouds on surfaces / planes.
 */
UENUM(BlueprintType)
enum class EOmniTraceScatterPattern : uint8
{
    UniformRandom   UMETA(DisplayName = "Uniform Random"),
    PoissonDisk     UMETA(DisplayName = "Poisson Disk"),
    JitteredGrid    UMETA(DisplayName = "Jittered Grid"),
    Clustered       UMETA(DisplayName = "Clustered Scatter"),
};

/**
 * Spline-conforming patterns – all variants of "follow this curve".
 */
UENUM(BlueprintType)
enum class EOmniTraceSplinePattern : uint8
{
    EvenDistance        UMETA(DisplayName = "Even Distance Along Spline"),
    FixedSegmentCount   UMETA(DisplayName = "Fixed Segment Count"),
    SplineKnots         UMETA(DisplayName = "At Spline Knots / Points"),
    RandomOnSpline      UMETA(DisplayName = "Random Positions On Spline"),
};

/**
 * Volume fill patterns – 3D waypoint/voxel volumes.
 */
UENUM(BlueprintType)
enum class EOmniTraceVolumePattern : uint8
{
    BoxGrid         UMETA(DisplayName = "Box Grid"),
    BoxSpiral       UMETA(DisplayName = "Box Spiral"),
    SphereShell     UMETA(DisplayName = "Sphere Shell"),
    SphereVolume    UMETA(DisplayName = "Sphere Volume"),
    CylinderShell   UMETA(DisplayName = "Cylinder Shell"),
    CylinderVolume  UMETA(DisplayName = "Cylinder Volume"),
};

/**
 * Configurable pattern selector for higher-level path / layout helpers.
 */
USTRUCT(BlueprintType)
struct FOmniTracePatternConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern")
    EOmniTracePatternFamily PatternFamily = EOmniTracePatternFamily::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Line"))
    EOmniTraceLinePattern LinePattern = EOmniTraceLinePattern::SimpleLine;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Arc"))
    EOmniTraceArcPattern ArcPattern = EOmniTraceArcPattern::SingleArc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Orbit"))
    EOmniTraceOrbitPattern OrbitPattern = EOmniTraceOrbitPattern::CircularOrbit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Grid"))
    EOmniTraceGridPattern GridPattern = EOmniTraceGridPattern::RowScan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::NoiseWalk"))
    EOmniTraceNoiseWalkPattern NoiseWalkPattern = EOmniTraceNoiseWalkPattern::RandomWalk;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Radial"))
    EOmniTraceRadialPattern RadialPattern = EOmniTraceRadialPattern::Starburst;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Scatter"))
    EOmniTraceScatterPattern ScatterPattern = EOmniTraceScatterPattern::UniformRandom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Spline"))
    EOmniTraceSplinePattern SplinePattern = EOmniTraceSplinePattern::EvenDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OmniTrace|Pattern",
        meta = (EditCondition = "PatternFamily == EOmniTracePatternFamily::Volume"))
    EOmniTraceVolumePattern VolumePattern = EOmniTraceVolumePattern::BoxGrid;
};
