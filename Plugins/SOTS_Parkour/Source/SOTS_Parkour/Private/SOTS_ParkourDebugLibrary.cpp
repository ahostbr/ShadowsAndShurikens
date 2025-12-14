#include "SOTS_ParkourDebugLibrary.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "SOTS_ParkourActionData.h"
#include "SOTS_ParkourComponent.h"
#include "Misc/Paths.h"

namespace
{
ACharacter* ResolveParkourCharacter(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PC = It->Get())
		{
			if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
			{
				return Character;
			}
		}
	}

	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		if (ACharacter* Character = *It)
		{
			return Character;
		}
	}

	return nullptr;
}

void RunParkourTracesWithDebug(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ACharacter* Character = ResolveParkourCharacter(World);
	if (!Character)
	{
		Ar.Logf(TEXT("SOTS_Parkour: no character found to run traces."));
		return;
	}

	USOTS_ParkourComponent* ParkourComp = Character->FindComponentByClass<USOTS_ParkourComponent>();
	if (!ParkourComp)
	{
		Ar.Logf(TEXT("SOTS_Parkour: character %s has no USOTS_ParkourComponent."), *GetNameSafe(Character));
		return;
	}

	const bool bOriginalDebug = ParkourComp->bEnableDebugLogging;
	const int32 OriginalTraceBudget = ParkourComp->MaxTracesPerFrame;

	ParkourComp->bEnableDebugLogging = true;
	ParkourComp->MaxTracesPerFrame = 0; // disable per-frame cap so all traces fire.

	FSOTS_ParkourResult Result;
	const bool bFound = ParkourComp->DebugRunDetectionBypass(Result);

	ParkourComp->MaxTracesPerFrame = OriginalTraceBudget;
	ParkourComp->bEnableDebugLogging = bOriginalDebug;

	if (bFound)
	{
		USOTS_ParkourDebugLibrary::LogParkourResult(ParkourComp, Result, TEXT("ConsoleRunAllTraces"));
	}

	Ar.Logf(TEXT("SOTS_Parkour: ran traces with debug for %s. HasResult=%s IsValid=%s Action=%d"),
		*GetNameSafe(Character),
		Result.bHasResult ? TEXT("true") : TEXT("false"),
		Result.bIsValid ? TEXT("true") : TEXT("false"),
		static_cast<int32>(Result.Action));
}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRunParkourTracesWithDebug(
	TEXT("sots.parkour.run_all_traces_debug"),
	TEXT("Run all SOTS_Parkour traces once with debug enabled (disables trace budget for the run)."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RunParkourTracesWithDebug));

void USOTS_ParkourDebugLibrary::DrawParkourCapsuleTrace(
	UObject* WorldContextObject,
	const FSOTS_ParkourCapsuleTraceSettings& Settings,
	bool bHit,
	const FHitResult& Hit,
	float Duration
)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
	if (!World)
	{
		return;
	}

	const FColor Color = (bHit && Hit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;

	// Start capsule
	DrawDebugCapsule(
		World,
		Settings.Start,
		Settings.HalfHeight,
		Settings.Radius,
		FQuat::Identity,
		Color,
		false,
		Duration,
		0,
		1.0f
	);

	// End capsule + connecting line (if any travel)
	if (!Settings.End.Equals(Settings.Start))
	{
		DrawDebugCapsule(
			World,
			Settings.End,
			Settings.HalfHeight,
			Settings.Radius,
			FQuat::Identity,
			Color,
			false,
			Duration,
			0,
			1.0f
		);

		DrawDebugLine(
			World,
			Settings.Start,
			Settings.End,
			Color,
			false,
			Duration,
			0,
			1.0f
		);
	}

	// Impact point
	if (bHit && Hit.IsValidBlockingHit())
	{
		DrawDebugPoint(
			World,
			Hit.ImpactPoint,
			12.0f,
			FColor::Yellow,
			false,
			Duration
		);
	}
}

void USOTS_ParkourDebugLibrary::LogParkourResult(
	UObject* WorldContextObject,
	const FSOTS_ParkourResult& Result,
	const FString& ContextLabel
)
{
	// WorldContextObject is currently only used to keep the signature
	// consistent with other debug helpers; we may route this through a
	// dedicated log category later.
	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("[SOTS_Parkour][%s] bHasResult=%s bIsValid=%s Action=%d ResultType=%d ClimbStyle=%d HeightDelta=%.2f WorldLoc=(%.1f, %.1f, %.1f) TargetLoc=(%.1f, %.1f, %.1f)"),
		*ContextLabel,
		Result.bHasResult ? TEXT("true") : TEXT("false"),
		Result.bIsValid ? TEXT("true") : TEXT("false"),
		static_cast<int32>(Result.Action),
		static_cast<int32>(Result.ResultType),
		static_cast<int32>(Result.ClimbStyle),
		Result.HeightDelta,
		Result.WorldLocation.X, Result.WorldLocation.Y, Result.WorldLocation.Z,
		Result.TargetLocation.X, Result.TargetLocation.Y, Result.TargetLocation.Z
	);
}

bool USOTS_ParkourDebugLibrary::ImportParkourActionsFromJson(
	USOTS_ParkourActionSet* TargetAsset,
	const FString& FilePath
)
{
#if WITH_EDITOR
	if (!TargetAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("ImportParkourActionsFromJson: TargetAsset is null"));
		return false;
	}

	FString ResolvedPath = FilePath;

	// Allow relative paths under Content/.
	if (!FPaths::FileExists(ResolvedPath))
	{
		if (!FilePath.IsEmpty())
		{
			const FString Candidate = FPaths::ProjectContentDir() / FilePath;
			if (FPaths::FileExists(Candidate))
			{
				ResolvedPath = Candidate;
			}
		}
	}

	const bool bLoaded = TargetAsset->LoadFromJsonFile(ResolvedPath);
	if (!bLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("ImportParkourActionsFromJson: failed to load %s"), *ResolvedPath);
	}
	return bLoaded;
#else
	UE_LOG(LogTemp, Warning, TEXT("ImportParkourActionsFromJson is editor-only."));
	return false;
#endif
}
