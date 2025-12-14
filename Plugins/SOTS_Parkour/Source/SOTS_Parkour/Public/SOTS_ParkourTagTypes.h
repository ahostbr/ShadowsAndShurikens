// SOTS_ParkourTagTypes.h
// SPINE V3_02 – Tag-driven type schema for Parkour.
//
// This header defines small, focused enums that mirror the GameplayTag
// DataTable used by the original CGF Parkour system.
//
// They are derived directly from the Parkour Tag Data JSON in this convo:
//   - Parkour.State.*
//   - Parkour.Action.*           (indirectly, via logical state/usage)
//   - Parkour.ConditionType.*
//   - Parkour.ClimbStyle.*       (aligned with ESOTS_ClimbStyle in
//                                 SOTS_ParkourTypes.h)
//   - Parkour.Direction.*
//   - Parkour.TicTacSide.*
//
// These enums DO NOT replace GameplayTags; they exist to give the C++
// side a strongly-typed view of the same conceptual space so we can:
//
//   - Keep Parkour logic compact and readable in C++.
//   - Cleanly map between DataTable rows (tags) and internal decisions.
//   - Avoid scattering raw FNames/FGameplayTags all over runtime code.
//
// Later SPINE/BRIDGE passes will add explicit helpers to convert between
// GameplayTags and these enums (e.g., Tag → ESOTS_ParkourLogicalState).
// For now, this file focuses purely on the type schema.

#pragma once

#include "CoreMinimal.h"
#include "SOTS_ParkourTagTypes.generated.h"

/**
 * Logical Parkour "mode" used by the action rows and state tags.
 *
 * This mirrors Parkour.State.* but uses a distinct enum name to avoid
 * clashing with the runtime ESOTS_ParkourLogicalState declared in
 * SOTS_ParkourTypes.h.
 */
UENUM(BlueprintType)
enum class ESOTS_ParkourLogicalTagState : uint8
{
    NotBusy     UMETA(DisplayName = "Not Busy"),
    Mantle      UMETA(DisplayName = "Mantle"),
    Vault       UMETA(DisplayName = "Vault"),
    Climb       UMETA(DisplayName = "Climb"),
    ReachLedge  UMETA(DisplayName = "Reach Ledge")
};

/**
 * Mirrors Parkour.ConditionType.* from the Tag DataTable:
 *
 *   Parkour.ConditionType.Velocity
 *   Parkour.ConditionType.WallHeight
 *   Parkour.ConditionType.WallDepth
 *   Parkour.ConditionType.VaultHeight
 *
 * These are used by CGF to decide which metric a given check uses, and
 * will be consumed by the classification logic once ported.
 */
UENUM(BlueprintType)
enum class ESOTS_ParkourConditionType : uint8
{
    Velocity    UMETA(DisplayName = "Velocity"),
    WallHeight  UMETA(DisplayName = "Wall Height"),
    WallDepth   UMETA(DisplayName = "Wall Depth"),
    VaultHeight UMETA(DisplayName = "Vault Height")
};

/**
 * Mirrors Parkour.Direction.* from the Tag DataTable:
 *
 *   Parkour.Direction.Forward
 *   Parkour.Direction.Right
 *   Parkour.Direction.Left
 *   Parkour.Direction.Backward
 *   Parkour.Direction.ForwardRight
 *   Parkour.Direction.ForwardLeft
 *   Parkour.Direction.BackwardRight
 *   Parkour.Direction.BackwardLeft
 *   Parkour.Direction.NoDirection
 *
 * This will be used when picking OutParkour rows based on direction
 * (e.g. hop left/right, forward/back variants).
 */
UENUM(BlueprintType)
enum class ESOTS_ParkourDirection : uint8
{
    NoDirection     UMETA(DisplayName = "No Direction"),
    Forward         UMETA(DisplayName = "Forward"),
    Right           UMETA(DisplayName = "Right"),
    Left            UMETA(DisplayName = "Left"),
    Backward        UMETA(DisplayName = "Backward"),
    ForwardRight    UMETA(DisplayName = "Forward Right"),
    ForwardLeft     UMETA(DisplayName = "Forward Left"),
    BackwardRight   UMETA(DisplayName = "Backward Right"),
    BackwardLeft    UMETA(DisplayName = "Backward Left")
};

/**
 * Mirrors Parkour.TicTacSide.* from the Tag DataTable:
 *
 *   Parkour.TicTacSide.NoDirection
 *   Parkour.TicTacSide.Left
 *   Parkour.TicTacSide.Right
 *
 * This is mainly used by TicTac actions and related OutParkour rows.
 */
UENUM(BlueprintType)
enum class ESOTS_ParkourTicTacSide : uint8
{
    NoDirection UMETA(DisplayName = "No Direction"),
    Left        UMETA(DisplayName = "Left"),
    Right       UMETA(DisplayName = "Right")
};
