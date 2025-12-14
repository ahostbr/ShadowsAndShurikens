#include "SOTS_ParkourSurfaceProbes.h"

#include "SOTS_ParkourTraceMath.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "OmniTraceBlueprintLibrary.h"
#include "OmniTraceTypes.h"

namespace
{
	bool ConvertOmniTraceHit_Surface(const FOmniTracePatternResult& Result, const FVector& Start, const FVector& End, FHitResult& OutHit)
	{
		if (!Result.bAnyHit)
		{
			OutHit = FHitResult();
			return false;
		}

		const FOmniTraceHitResult& First = Result.FirstBlockingHit;
		FHitResult Hit(First.HitActor.Get(), First.HitComponent.Get(), First.ImpactPoint, First.Normal);
		Hit.bBlockingHit = First.bBlockingHit;
		Hit.Location = First.Location;
		Hit.ImpactPoint = First.ImpactPoint;
		Hit.Normal = First.Normal;
		Hit.ImpactNormal = First.Normal;
		Hit.Distance = First.Distance;
		Hit.TraceStart = Start;
		Hit.TraceEnd = End;

		OutHit = Hit;
		return First.bBlockingHit;
	}

	bool RunSingleOmniTrace(
		UObject* WorldContextObject,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel TraceChannel,
		EOmniTraceShape Shape,
		float ShapeRadius,
		float CapsuleHalfHeight,
		const FVector& BoxHalfExtents,
		FHitResult& OutHit)
	{
		if (!WorldContextObject)
		{
			OutHit = FHitResult();
			return false;
		}

		const FVector Delta = End - Start;
		const float Distance = Delta.Size();
		const FVector Dir = Delta.IsNearlyZero() ? FVector::ForwardVector : Delta.GetSafeNormal();

		FOmniTraceRequest Request;
		Request.PatternFamily          = EOmniTraceTraceFamily::Forward;
		Request.ForwardPattern         = EOmniTraceForwardPattern::SingleRay;
		Request.Shape                  = Shape;
		Request.ShapeRadius            = ShapeRadius;
		Request.CapsuleHalfHeight      = CapsuleHalfHeight;
		Request.BoxHalfExtents         = BoxHalfExtents.IsNearlyZero() ? Request.BoxHalfExtents : BoxHalfExtents;
		Request.TraceChannel           = TraceChannel;
		Request.RayCount               = 1;
		Request.MaxDistance            = FMath::Max(Distance, KINDA_SMALL_NUMBER);
		Request.OriginLocationOverride = Start;
		Request.OriginRotationOverride = Dir.Rotation();
		Request.DebugOptions.bEnableDebug = false;

		const FOmniTracePatternResult PatternResult = UOmniTraceBlueprintLibrary::OmniTrace_Pattern(WorldContextObject, Request);
		return ConvertOmniTraceHit_Surface(PatternResult, Start, Start + Dir * Request.MaxDistance, OutHit);
	}
}

bool USOTS_ParkourSurfaceProbes::ProbeMantleSurface(
	UObject* WorldContextObject,
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	OutHit = FHitResult();

	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
	if (!World)
	{
		return false;
	}

	FSOTS_ParkourCapsuleTraceSettings TraceSettings;
	USOTS_ParkourTraceMath::BuildMantleSurfaceTrace(WarpTopPoint, CapsuleHalfHeight, TraceSettings);

	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		TraceSettings.Start,
		TraceSettings.End,
		TraceChannel,
		EOmniTraceShape::CapsuleSweep,
		TraceSettings.Radius,
		TraceSettings.HalfHeight,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;

		DrawDebugCapsule(
			World,
			TraceSettings.Start,
			TraceSettings.HalfHeight,
			TraceSettings.Radius,
			FQuat::Identity,
			Color,
			false,
			DebugLifetime,
			0,
			1.0f);

		if (!TraceSettings.End.Equals(TraceSettings.Start))
		{
			DrawDebugCapsule(
				World,
				TraceSettings.End,
				TraceSettings.HalfHeight,
				TraceSettings.Radius,
				FQuat::Identity,
				Color,
				false,
				DebugLifetime,
				0,
				1.0f);

			DrawDebugLine(
				World,
				TraceSettings.Start,
				TraceSettings.End,
				Color,
				false,
				DebugLifetime,
				0,
				1.0f);
		}

		if (bHit && OutHit.IsValidBlockingHit())
		{
			DrawDebugPoint(
				World,
				OutHit.ImpactPoint,
				10.0f,
				FColor::Yellow,
				false,
				DebugLifetime);
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}

bool USOTS_ParkourSurfaceProbes::ProbeVaultSurface(
	UObject* WorldContextObject,
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	OutHit = FHitResult();

	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
	if (!World)
	{
		return false;
	}

	FSOTS_ParkourCapsuleTraceSettings TraceSettings;
	USOTS_ParkourTraceMath::BuildVaultSurfaceTrace(WarpTopPoint, CapsuleHalfHeight, TraceSettings);

	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		TraceSettings.Start,
		TraceSettings.End,
		TraceChannel,
		EOmniTraceShape::CapsuleSweep,
		TraceSettings.Radius,
		TraceSettings.HalfHeight,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;

		DrawDebugCapsule(
			World,
			TraceSettings.Start,
			TraceSettings.HalfHeight,
			TraceSettings.Radius,
			FQuat::Identity,
			Color,
			false,
			DebugLifetime,
			0,
			1.0f);

		if (!TraceSettings.End.Equals(TraceSettings.Start))
		{
			DrawDebugCapsule(
				World,
				TraceSettings.End,
				TraceSettings.HalfHeight,
				TraceSettings.Radius,
				FQuat::Identity,
				Color,
				false,
				DebugLifetime,
				0,
				1.0f);

			DrawDebugLine(
				World,
				TraceSettings.Start,
				TraceSettings.End,
				Color,
				false,
				DebugLifetime,
				0,
				1.0f);
		}

		if (bHit && OutHit.IsValidBlockingHit())
		{
			DrawDebugPoint(
				World,
				OutHit.ImpactPoint,
				10.0f,
				FColor::Yellow,
				false,
				DebugLifetime);
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}

namespace
{
}

bool USOTS_ParkourSurfaceProbes::ProbeDropHangPrimary(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::SphereSweep,
		35.0f,
		35.0f,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr)
		{
			const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;
			DrawDebugSphere(World, Start, 35.0f, 16, Color, false, DebugLifetime);
			if (!End.Equals(Start))
			{
				DrawDebugSphere(World, End, 35.0f, 16, Color, false, DebugLifetime);
				DrawDebugLine(World, Start, End, Color, false, DebugLifetime, 0, 1.0f);
			}
			if (bHit && OutHit.IsValidBlockingHit())
			{
				DrawDebugPoint(World, OutHit.ImpactPoint, 10.0f, FColor::Yellow, false, DebugLifetime);
			}
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}

bool USOTS_ParkourSurfaceProbes::ProbeDropHangSecondary(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::SphereSweep,
		5.0f,
		5.0f,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr)
		{
			const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;
			DrawDebugSphere(World, Start, 5.0f, 16, Color, false, DebugLifetime);
			if (!End.Equals(Start))
			{
				DrawDebugSphere(World, End, 5.0f, 16, Color, false, DebugLifetime);
				DrawDebugLine(World, Start, End, Color, false, DebugLifetime, 0, 1.0f);
			}
			if (bHit && OutHit.IsValidBlockingHit())
			{
				DrawDebugPoint(World, OutHit.ImpactPoint, 10.0f, FColor::Yellow, false, DebugLifetime);
			}
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}

bool USOTS_ParkourSurfaceProbes::ProbeCapsuleConfirm(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	float Radius,
	float HalfHeight,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::CapsuleSweep,
		Radius,
		HalfHeight,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr)
		{
			const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;

			DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, Color, false, DebugLifetime, 0, 1.0f);
			if (!End.Equals(Start))
			{
				DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, Color, false, DebugLifetime, 0, 1.0f);
				DrawDebugLine(World, Start, End, Color, false, DebugLifetime, 0, 1.0f);
			}
			if (bHit && OutHit.IsValidBlockingHit())
			{
				DrawDebugPoint(World, OutHit.ImpactPoint, 10.0f, FColor::Yellow, false, DebugLifetime);
			}
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}

bool USOTS_ParkourSurfaceProbes::ProbeTicTac(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	bool bDrawDebug,
	float DebugLifetime)
{
	// Default radius 30, debug lifetime 5s per CGF traces.
	const bool bHit = RunSingleOmniTrace(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::SphereSweep,
		30.0f,
		30.0f,
		FVector::ZeroVector,
		OutHit);

	if (bDrawDebug)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr)
		{
			const FColor Color = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;
			DrawDebugSphere(World, Start, 30.0f, 16, Color, false, DebugLifetime);
			if (!End.Equals(Start))
			{
				DrawDebugSphere(World, End, 30.0f, 16, Color, false, DebugLifetime);
				DrawDebugLine(World, Start, End, Color, false, DebugLifetime, 0, 1.0f);
			}
			if (bHit && OutHit.IsValidBlockingHit())
			{
				DrawDebugPoint(World, OutHit.ImpactPoint, 10.0f, FColor::Yellow, false, DebugLifetime);
			}
		}
	}

	return bHit && OutHit.IsValidBlockingHit();
}
