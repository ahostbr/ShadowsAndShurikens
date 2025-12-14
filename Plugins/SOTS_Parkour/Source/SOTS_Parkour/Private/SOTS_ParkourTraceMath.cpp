#include "SOTS_ParkourTraceMath.h"

void USOTS_ParkourTraceMath::BuildMantleSurfaceTrace(
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings
)
{
	// CGF "Check Mantle Surface" parity:
	//   - Uses WarpTopPoint as the base.
	//   - Offsets the capsule center by CapsuleHalfHeight ± 8 on Z.
	//   - Uses Radius = 25 on the dedicated parkour trace channel.
	//
	// In C++ we represent that as a small vertical capsule segment
	// whose endpoints are:
	//   StartZ = CapsuleHalfHeight + 8
	//   EndZ   = CapsuleHalfHeight - 8
	//
	// The actual collision channel is chosen by the caller; this
	// helper only prepares the geometry.
	constexpr float Radius = 25.0f;
	constexpr float OffsetDelta = 8.0f;

	const float StartZOffset = CapsuleHalfHeight + OffsetDelta;
	const float EndZOffset   = CapsuleHalfHeight - OffsetDelta;

	BuildVerticalSegmentCapsule(
		WarpTopPoint,
		CapsuleHalfHeight,
		StartZOffset,
		EndZOffset,
		Radius,
		OutSettings
	);
}

void USOTS_ParkourTraceMath::BuildVaultSurfaceTrace(
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings
)
{
	// CGF "Check Vault Surface" parity:
	//   - Uses WarpTopPoint as the base.
	//   - Uses CapsuleHalfHeight + 18 as the upper sample.
	//   - Uses CapsuleHalfHeight + 5  as the lower sample.
	//   - Radius is again fixed at 25.
	//
	// This yields a slightly taller vertical segment biased above the
	// warp point, matching the original Blueprint behavior.
	constexpr float Radius = 25.0f;

	const float StartZOffset = CapsuleHalfHeight + 18.0f;
	const float EndZOffset   = CapsuleHalfHeight + 5.0f;

	BuildVerticalSegmentCapsule(
		WarpTopPoint,
		CapsuleHalfHeight,
		StartZOffset,
		EndZOffset,
		Radius,
		OutSettings
	);
}

void USOTS_ParkourTraceMath::BuildVerticalSegmentCapsule(
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	float StartZOffset,
	float EndZOffset,
	float Radius,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings
)
{
	// Shared helper used by both mantle and vault surface checks.
	//
	// We treat WarpTopPoint as "ground truth" from the CGF parity math
	// and build a simple vertical segment along +Z, using the provided
	// Z offsets and a small symmetric HalfHeight.
	//
	// NOTE:
	//   - HalfHeight is intentionally kept small relative to the player
	//     capsule, since the Start/End are already offset by the full
	//     CapsuleHalfHeight. This matches the original Blueprint intent
	//     of probing a tight region around the warp point.
	const FVector UpVector = FVector::UpVector;

	const FVector Start = WarpTopPoint + UpVector * StartZOffset;
	const FVector End   = WarpTopPoint + UpVector * EndZOffset;

	OutSettings.Start = Start;
	OutSettings.End   = End;

	OutSettings.Radius = Radius;

	// Using a compact HalfHeight keeps the probe localized while still
	// benefiting from capsule "thickness". We tie it loosely to the
	// player capsule size so it scales with different characters.
	const float LocalHalfHeight =
		FMath::Clamp(CapsuleHalfHeight * 0.2f, 12.0f, 40.0f);

	OutSettings.HalfHeight = LocalHalfHeight;
}
