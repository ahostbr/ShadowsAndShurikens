#include "SOTS_InputModule.h"

#include "SOTS_InputBlueprintLibrary.h"
#include "SOTS_InputCoreBridgeSettings.h"
#include "SOTS_InputRouterComponent.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "FSOTS_InputModule"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace SOTS::Input::ConsoleCommands
{
	void Register();
	void Unregister();
}
#endif

namespace SOTS::Input::CoreBridge
{
	const USOTS_InputCoreBridgeSettings* GetSettings()
	{
		return GetDefault<USOTS_InputCoreBridgeSettings>();
	}

	bool IsBridgeEnabled(const USOTS_InputCoreBridgeSettings* Settings)
	{
		return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
	}

	bool ShouldLogVerbose(const USOTS_InputCoreBridgeSettings* Settings)
	{
		return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
	}

	class FSOTS_InputCoreLifecycleHook final : public ISOTS_CoreLifecycleListener
	{
	public:
		void OnSOTS_PlayerControllerBeginPlay(APlayerController* PC) override
		{
			const USOTS_InputCoreBridgeSettings* Settings = GetSettings();
			if (!IsBridgeEnabled(Settings) || !PC)
			{
				return;
			}

			if (LastPC.Get() == PC)
			{
				return;
			}

			LastPC = PC;
			LastPawn.Reset();
			MaybeInitForPlayerController(PC, Settings);
		}

		void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override
		{
			const USOTS_InputCoreBridgeSettings* Settings = GetSettings();
			if (!IsBridgeEnabled(Settings) || !PC || !Pawn)
			{
				return;
			}

			if (LastPC.Get() == PC && LastPawn.Get() == Pawn)
			{
				return;
			}

			LastPC = PC;
			LastPawn = Pawn;
			MaybeBindPawn(PC, Pawn, Settings);
		}

	private:
		void MaybeInitForPlayerController(APlayerController* PC, const USOTS_InputCoreBridgeSettings* Settings)
		{
			if (ShouldLogVerbose(Settings))
			{
				UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input CoreBridge: PlayerControllerBeginPlay %s"), *GetNameSafe(PC));
			}

			USOTS_InputRouterComponent* Router = USOTS_InputBlueprintLibrary::EnsureRouterOnPlayerController(nullptr, PC);
			if (ShouldLogVerbose(Settings) && Router)
			{
				UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input CoreBridge: Router ensured for %s"), *GetNameSafe(PC));
			}
		}

		void MaybeBindPawn(APlayerController* PC, APawn* Pawn, const USOTS_InputCoreBridgeSettings* Settings)
		{
			if (ShouldLogVerbose(Settings))
			{
				UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input CoreBridge: PawnPossessed PC=%s Pawn=%s"), *GetNameSafe(PC), *GetNameSafe(Pawn));
			}

			USOTS_InputRouterComponent* Router = PC ? PC->FindComponentByClass<USOTS_InputRouterComponent>() : nullptr;
			if (!Router)
			{
				Router = USOTS_InputBlueprintLibrary::EnsureRouterOnPlayerController(nullptr, PC);
			}

			if (Router)
			{
				Router->RefreshRouter();
				if (ShouldLogVerbose(Settings))
				{
					UE_LOG(LogTemp, Verbose, TEXT("SOTS_Input CoreBridge: Router refreshed for %s"), *GetNameSafe(PC));
				}
			}
		}

		TWeakObjectPtr<APlayerController> LastPC;
		TWeakObjectPtr<APawn> LastPawn;
	};

	FSOTS_InputCoreLifecycleHook GHook;
	FSOTS_CoreLifecycleListenerHandle GHookHandle;

	void Register()
	{
		GHookHandle.Register(&GHook);
	}

	void Unregister()
	{
		GHookHandle.Unregister();
	}
}

void FSOTS_InputModule::StartupModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SOTS::Input::ConsoleCommands::Register();
#endif
	SOTS::Input::CoreBridge::Register();
}

void FSOTS_InputModule::ShutdownModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SOTS::Input::ConsoleCommands::Unregister();
#endif
	SOTS::Input::CoreBridge::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSOTS_InputModule, SOTS_Input)
