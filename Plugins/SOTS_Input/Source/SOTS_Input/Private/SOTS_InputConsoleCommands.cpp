#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#include "SOTS_InputRouterComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace SOTS::Input::ConsoleCommands
{
    namespace
    {
        static TUniquePtr<FAutoConsoleCommandWithWorld> DumpCommand;
        static TUniquePtr<FAutoConsoleCommandWithWorld> RefreshCommand;

        USOTS_InputRouterComponent* GetFirstRouter(UWorld* World)
        {
            if (!World)
            {
                return nullptr;
            }

            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (USOTS_InputRouterComponent* Router = PC->FindComponentByClass<USOTS_InputRouterComponent>())
                {
                    return Router;
                }

                if (APawn* Pawn = PC->GetPawn())
                {
                    if (USOTS_InputRouterComponent* PawnRouter = Pawn->FindComponentByClass<USOTS_InputRouterComponent>())
                    {
                        return PawnRouter;
                    }
                }
            }

            return nullptr;
        }

        void Dump(UWorld* World)
        {
            if (USOTS_InputRouterComponent* Router = GetFirstRouter(World))
            {
                TArray<FSOTS_ActiveInputLayerSummary> LayerSummaries;
                Router->GetActiveLayerSummaries(LayerSummaries);

                TArray<FGameplayTag> OpenChannels;
                Router->GetOpenBufferChannels(OpenChannels);
                const FGameplayTag TopChannel = Router->GetTopBufferChannel();

                TArray<FSOTS_InputBindingKey> BindingSnapshot;
                Router->GetBindingSnapshot(BindingSnapshot);

                UE_LOG(LogTemp, Log, TEXT("sots.input.dump: Layers=%d"), LayerSummaries.Num());
                for (const FSOTS_ActiveInputLayerSummary& Summary : LayerSummaries)
                {
                    UE_LOG(LogTemp, Log, TEXT("  Tag=%s Priority=%d Block=%s Consume=%d Contexts=%d Handlers=%d"),
                        *Summary.LayerTag.ToString(),
                        Summary.Priority,
                        Summary.bBlocksLowerPriorityLayers ? TEXT("true") : TEXT("false"),
                        static_cast<int32>(Summary.ConsumePolicy),
                        Summary.MappingContextCount,
                        Summary.HandlerCount);
                }

                const FString OpenChannelsStr = FString::JoinBy(OpenChannels, TEXT(","), [](const FGameplayTag& Tag){ return Tag.ToString(); });
                UE_LOG(LogTemp, Log, TEXT("  Buffers: Top=%s Open=[%s]"), *TopChannel.ToString(), *OpenChannelsStr);

                const FString BindingStr = FString::JoinBy(BindingSnapshot, TEXT(","), [](const FSOTS_InputBindingKey& Key){ return Key.ToString(); });
                UE_LOG(LogTemp, Log, TEXT("  Bindings (%d): %s"), BindingSnapshot.Num(), *BindingStr);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("sots.input.dump: no router found"));
            }
        }

        void Refresh(UWorld* World)
        {
            if (USOTS_InputRouterComponent* Router = GetFirstRouter(World))
            {
                Router->RefreshRouter();
                UE_LOG(LogTemp, Log, TEXT("sots.input.refresh: router refreshed"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("sots.input.refresh: no router found"));
            }
        }
    }

    void Register()
    {
        DumpCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("sots.input.dump"),
            TEXT("Dump SOTS_Input router layers, bindings, and buffers"),
            FConsoleCommandWithWorldDelegate::CreateStatic(&Dump));

        RefreshCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("sots.input.refresh"),
            TEXT("Refresh SOTS_Input router bindings and contexts"),
            FConsoleCommandWithWorldDelegate::CreateStatic(&Refresh));
    }

    void Unregister()
    {
        DumpCommand.Reset();
        RefreshCommand.Reset();
    }
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
