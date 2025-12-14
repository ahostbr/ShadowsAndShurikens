// SOTS_ParkourActionData.h
// SPINE V3_03 – Parkour action + warp data model.
//
// This header defines the C++ representation of the original CGF
// OutParkour DataTable and its MotionWarpingSettings array.
//
// It is driven directly by the JSON export in this conversation:
//   - Fields: Name, Parkour Montage, Play Rate,
//             Starting Position, Falling Position,
//             InState.TagName, OutState.TagName,
//             MotionWarpingSettings[0..N] with:
//                 Start Time, End Time,
//                 Warp Target Name,
//                 Warp Point Anim Provider,
//                 Warp Point Anim Bone Name,
//                 Warp Translation, Warp Rotation,
//                 Ignore Z Axis, Reversed Rotation,
//                 Rotation Type,
//                 Warp Transform Type.TagName,
//                 X Offset, Z Offset,
//                 Character Dif Height Adjustmen.
//
// These structs and the USOTS_ParkourActionSet DataAsset give us a
// 1:1 editable representation of that table for SOTS_Parkour. Later
// SPINE/BRIDGE passes will handle:
//
//   - Import/export from the original DataTable (optional tools).
//   - Runtime lookup and action selection.
//   - Motion-warp node wiring based on these settings.
//
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectSaveContext.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourActionData.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class ESOTS_ParkourWarpFacingMode : uint8
{
    FaceSurface   UMETA(DisplayName = "Face Surface"),
    FaceForward   UMETA(DisplayName = "Face Character Forward"),
    FaceVelocity  UMETA(DisplayName = "Face Velocity"),
};

/**
 * A single warp window for a parkour action, corresponding to one
 * element of MotionWarpingSettings[] in the original OutParkour table.
 */
USTRUCT(BlueprintType)
struct FSOTS_ParkourWarpSettings
{
    GENERATED_BODY()

    // Time range (in montage seconds) where this warp window is active.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    float StartTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    float EndTime = 0.0f;

    // String identifier used to disambiguate multiple warp targets
    // in the same montage (e.g. "1", "2"). Mirrors "Warp Target Name".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    FName WarpTargetName;

    // Source of the warp point inside the animation graph
    // (e.g. None, Root, a named bone, etc.).
    // Mirrors "Warp Point Anim Provider".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    FName WarpPointAnimProvider;

    // Optional bone name used by the provider to extract a warp point.
    // Mirrors "Warp Point Anim Bone Name".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    FName WarpPointAnimBoneName;

    // Whether to warp translation/rotation for this window.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    bool bWarpTranslation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    bool bIgnoreZAxis = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    bool bWarpRotation = true;

    // Free-form rotation mode string, mirrors "Rotation Type"
    // (e.g. Default, Custom). We keep this as a name instead of an
    // enum so it stays 1:1 with the original data and can evolve.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    FName RotationType;

    // The gameplay tag from the TransformType.* tag family that
    // describes what this warp is relative to (Start Position,
    // Mantle Position, etc.). Mirrors "Warp Transform Type".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    FGameplayTag WarpTransformType;

    // Positional offsets in local space applied on top of the base
    // warp transform. Mirrors X Offset / Z Offset.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    float XOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    float ZOffset = 0.0f;

    // Whether rotation should be inverted (e.g. when mirroring).
    // Mirrors "Reversed Rotation".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    bool bReversedRotation = false;

    // Whether to adjust for the difference between the character's
    // capsule height and the source character used to author the
    // animation. Mirrors "Character Dif Height Adjustmen".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    bool bAdjustForCharacterHeight = false;

    // Optional facing override for this window. Default matches legacy (face surface).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
    ESOTS_ParkourWarpFacingMode FacingMode = ESOTS_ParkourWarpFacingMode::FaceSurface;
};

/**
 * Describes a single parkour action row (one row in OutParkour).
 *
 * This is intentionally close to the JSON schema so that tools can
 * import/export rows with minimal friction.
 */
USTRUCT(BlueprintType)
struct FSOTS_ParkourActionDefinition : public FTableRowBase
{
    GENERATED_BODY()

    // Optional editor-friendly row name. This is not required by the
    // original DataTable but is extremely useful when authoring the
    // DataAsset directly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    FName RowName;

    // Logical action tag (e.g. Parkour.Action.Mantle).
    // Mirrors the "Name" field in the original table.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    FGameplayTag ActionTag;

    // Convenience enum to quickly categorize the row in code. This is
    // redundant with ActionTag but extremely helpful when branching in
    // C++. It should be kept in sync with the tag where reasonable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    ESOTS_ParkourAction ActionType = ESOTS_ParkourAction::None;

    // Animation montage used for this action. Mirrors "Parkour Montage".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    TSoftObjectPtr<UAnimMontage> ParkourMontage;

    // Playback rate override. Mirrors "Play Rate".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    float PlayRate = 1.0f;

    // Montage time at which the character is considered to have fully
    // taken hold (mantle top, vault apex, etc.). Mirrors "Starting Position".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    float StartingPosition = 0.0f;

    // Montage time used for blending from falling into this action
    // (if supported). Mirrors "Falling Position".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    float FallingPosition = 0.0f;

    // In/Out logical parkour states. These mirror the InState/OutState
    // TagName values in the original table (Parkour.State.*).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    FGameplayTag InStateTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    FGameplayTag OutStateTag;

    // All warp windows associated with this action.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
    TArray<FSOTS_ParkourWarpSettings> WarpSettings;
};

/**
 * DataAsset that holds all parkour action definitions. This replaces
 * the runtime dependency on the Blueprint DataTable version of
 * OutParkour while preserving its structure.
 */
UCLASS(BlueprintType, Blueprintable)
class SOTS_PARKOUR_API USOTS_ParkourActionSet : public UDataAsset
{
    GENERATED_BODY()

public:
    USOTS_ParkourActionSet();

    // Default JSON path used for editor import; editable so designers can point to their own export.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Config")
    FString DefaultUEFNJsonPath;

    // Automatically import from DefaultUEFNJsonPath when saving the asset (editor-only behavior).
    UPROPERTY(EditAnywhere, Category = "SOTS|Parkour|Config")
    bool bAutoImportOnSave = true;

    // Toggle this to run an import immediately in the editor (resets to false after running).
    UPROPERTY(EditAnywhere, Category = "SOTS|Parkour|Config")
    bool bForceImport = false;

    // All actions available to SOTS_Parkour. In a parity pass, this
    // should contain one entry for every row in the original OutParkour
    // DataTable.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parkour")
    TArray<FSOTS_ParkourActionDefinition> Actions;

    /** Populate this ActionSet from a JSON file (UEFN/CGF export shape). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "SOTS|Parkour|Config", DisplayName = "Load From JSON File")
    bool LoadFromJsonFile(const FString& FilePath);

    /** Convenience button: load using DefaultUEFNJsonPath. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "SOTS|Parkour|Config", DisplayName = "Load From Default UEFN Json")
    bool LoadFromDefaultUEFNJson();

#if WITH_EDITOR
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
