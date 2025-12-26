#include "SOTS_Core.h"

#include "Diagnostics/SOTS_CoreDiagnostics.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTS_Core);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace SOTS::Core::ConsoleCommands
{
    namespace
    {
        static TUniquePtr<FAutoConsoleCommandWithWorld> DumpSettingsCommand;
        static TUniquePtr<FAutoConsoleCommandWithWorld> DumpLifecycleCommand;
        static TUniquePtr<FAutoConsoleCommandWithWorld> DumpListenersCommand;
        static TUniquePtr<FAutoConsoleCommandWithWorld> DumpSaveParticipantsCommand;
        static TUniquePtr<FAutoConsoleCommandWithWorld> HealthCommand;

        void DumpSettings(UWorld* World)
        {
            (void)World;
            FSOTS_CoreDiagnostics::DumpSettings();
        }

        void DumpLifecycle(UWorld* World)
        {
            FSOTS_CoreDiagnostics::DumpLifecycleSnapshot(World);
        }

        void DumpListeners(UWorld* World)
        {
            (void)World;
            FSOTS_CoreDiagnostics::DumpRegisteredLifecycleListeners();
        }

        void DumpSaveParticipants(UWorld* World)
        {
            (void)World;
            FSOTS_CoreDiagnostics::DumpRegisteredSaveParticipants();
        }

        void PrintHealth(UWorld* World)
        {
            FSOTS_CoreDiagnostics::PrintHealthReport(World);
        }
    }

    void Register()
    {
        DumpSettingsCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("SOTS.Core.DumpSettings"),
            TEXT("Dump SOTS_Core settings."),
            FConsoleCommandWithWorldDelegate::CreateStatic(&DumpSettings));

        DumpLifecycleCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("SOTS.Core.DumpLifecycle"),
            TEXT("Dump SOTS_Core lifecycle snapshot for the current world."),
            FConsoleCommandWithWorldDelegate::CreateStatic(&DumpLifecycle));

        DumpListenersCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("SOTS.Core.DumpListeners"),
            TEXT("List registered SOTS_Core lifecycle listeners."),
            FConsoleCommandWithWorldDelegate::CreateStatic(&DumpListeners));

        DumpSaveParticipantsCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("SOTS.Core.DumpSaveParticipants"),
            TEXT("List registered SOTS_Core save participants (if enabled)."),
            FConsoleCommandWithWorldDelegate::CreateStatic(&DumpSaveParticipants));

        HealthCommand = MakeUnique<FAutoConsoleCommandWithWorld>(
            TEXT("SOTS.Core.Health"),
            TEXT("Print a SOTS_Core health report."),
            FConsoleCommandWithWorldDelegate::CreateStatic(&PrintHealth));
    }

    void Unregister()
    {
        DumpSettingsCommand.Reset();
        DumpLifecycleCommand.Reset();
        DumpListenersCommand.Reset();
        DumpSaveParticipantsCommand.Reset();
        HealthCommand.Reset();
    }
}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

class FSOTS_CoreModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        SOTS::Core::ConsoleCommands::Register();
#endif
    }

    void ShutdownModule() override
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        SOTS::Core::ConsoleCommands::Unregister();
#endif
    }
};

IMPLEMENT_MODULE(FSOTS_CoreModule, SOTS_Core)
