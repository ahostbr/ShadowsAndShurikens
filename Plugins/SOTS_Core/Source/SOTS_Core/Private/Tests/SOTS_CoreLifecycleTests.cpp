#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Settings/SOTS_CoreSettings.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"
#include "UObject/Package.h"

namespace SOTS::Core::Tests
{
    struct FSettingsGuard
    {
        USOTS_CoreSettings* Settings = GetMutableDefault<USOTS_CoreSettings>();
        bool bEnableLifecycleListenerDispatch = false;
        bool bSuppressDuplicateNotifications = true;
        bool bEnableMapTravelBridge = false;

        FSettingsGuard()
        {
            if (Settings)
            {
                bEnableLifecycleListenerDispatch = Settings->bEnableLifecycleListenerDispatch;
                bSuppressDuplicateNotifications = Settings->bSuppressDuplicateNotifications;
                bEnableMapTravelBridge = Settings->bEnableMapTravelBridge;
            }
        }

        ~FSettingsGuard()
        {
            if (Settings)
            {
                Settings->bEnableLifecycleListenerDispatch = bEnableLifecycleListenerDispatch;
                Settings->bSuppressDuplicateNotifications = bSuppressDuplicateNotifications;
                Settings->bEnableMapTravelBridge = bEnableMapTravelBridge;
            }
        }
    };

    USOTS_CoreLifecycleSubsystem* CreateSubsystem()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<USOTS_CoreLifecycleSubsystem>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSOTS_CoreDispatchGatedTest,
    "SOTS.Core.Lifecycle.DispatchGated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSOTS_CoreDispatchGatedTest::RunTest(const FString& Parameters)
{
    using namespace SOTS::Core::Tests;
    (void)Parameters;

    FSettingsGuard SettingsGuard;
    if (SettingsGuard.Settings)
    {
        SettingsGuard.Settings->bEnableLifecycleListenerDispatch = false;
        SettingsGuard.Settings->bSuppressDuplicateNotifications = true;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = CreateSubsystem();
    TestNotNull(TEXT("Subsystem should be created"), Subsystem);

    struct FListener final : ISOTS_CoreLifecycleListener
    {
        int32 PawnPossessedCount = 0;

        void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override
        {
            ++PawnPossessedCount;
        }
    };

    FListener Listener;
    IModularFeatures::Get().RegisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &Listener);

    APlayerController* PC = NewObject<APlayerController>(GetTransientPackage());
    APawn* Pawn = NewObject<APawn>(GetTransientPackage());
    Subsystem->NotifyPawnPossessed(PC, Pawn);

    TestEqual(TEXT("Dispatch disabled should not invoke listeners"), Listener.PawnPossessedCount, 0);

    IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &Listener);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSOTS_CoreDuplicateSuppressionTest,
    "SOTS.Core.Lifecycle.DuplicateSuppression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSOTS_CoreDuplicateSuppressionTest::RunTest(const FString& Parameters)
{
    using namespace SOTS::Core::Tests;
    (void)Parameters;

    FSettingsGuard SettingsGuard;
    if (SettingsGuard.Settings)
    {
        SettingsGuard.Settings->bSuppressDuplicateNotifications = true;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = CreateSubsystem();
    TestNotNull(TEXT("Subsystem should be created"), Subsystem);

    int32 PossessedCount = 0;
    Subsystem->OnPawnPossessed_Native.AddLambda(
        [&PossessedCount](APlayerController* PC, APawn* Pawn)
        {
            ++PossessedCount;
        });

    APlayerController* PC = NewObject<APlayerController>(GetTransientPackage());
    APawn* Pawn = NewObject<APawn>(GetTransientPackage());

    Subsystem->NotifyPawnPossessed(PC, Pawn);
    Subsystem->NotifyPawnPossessed(PC, Pawn);

    TestEqual(TEXT("Duplicate pawn possessed should fire once"), PossessedCount, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSOTS_CoreSnapshotBeforeDispatchTest,
    "SOTS.Core.Lifecycle.SnapshotBeforeDispatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSOTS_CoreSnapshotBeforeDispatchTest::RunTest(const FString& Parameters)
{
    using namespace SOTS::Core::Tests;
    (void)Parameters;

    FSettingsGuard SettingsGuard;
    if (SettingsGuard.Settings)
    {
        SettingsGuard.Settings->bEnableLifecycleListenerDispatch = true;
        SettingsGuard.Settings->bSuppressDuplicateNotifications = true;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = CreateSubsystem();
    TestNotNull(TEXT("Subsystem should be created"), Subsystem);

    struct FSnapshotListener final : ISOTS_CoreLifecycleListener
    {
        USOTS_CoreLifecycleSubsystem* Subsystem = nullptr;
        bool bSnapshotMatched = false;

        void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override
        {
            if (!Subsystem)
            {
                return;
            }

            const FSOTS_CoreLifecycleSnapshot Snapshot = Subsystem->GetCurrentSnapshot();
            bSnapshotMatched = (Snapshot.PrimaryPC == PC && Snapshot.PrimaryPawn == Pawn);
        }
    };

    FSnapshotListener Listener;
    Listener.Subsystem = Subsystem;
    IModularFeatures::Get().RegisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &Listener);

    APlayerController* PC = NewObject<APlayerController>(GetTransientPackage());
    APawn* Pawn = NewObject<APawn>(GetTransientPackage());
    Subsystem->NotifyPawnPossessed(PC, Pawn);

    TestTrue(TEXT("Snapshot should be updated before listener dispatch"), Listener.bSnapshotMatched);

    IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &Listener);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSOTS_CoreMapTravelDuplicateTest,
    "SOTS.Core.Travel.DuplicateSuppression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSOTS_CoreMapTravelDuplicateTest::RunTest(const FString& Parameters)
{
    using namespace SOTS::Core::Tests;
    (void)Parameters;

    FSettingsGuard SettingsGuard;
    if (SettingsGuard.Settings)
    {
        SettingsGuard.Settings->bSuppressDuplicateNotifications = true;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = CreateSubsystem();
    TestNotNull(TEXT("Subsystem should be created"), Subsystem);

    int32 PreLoadCount = 0;
    Subsystem->OnPreLoadMap_Native.AddLambda(
        [&PreLoadCount](const FString& MapName)
        {
            ++PreLoadCount;
        });

    Subsystem->NotifyPreLoadMap(TEXT("TestMap"));
    Subsystem->NotifyPreLoadMap(TEXT("TestMap"));
    Subsystem->NotifyPreLoadMap(TEXT("TestMap_B"));

    TestEqual(TEXT("Duplicate PreLoadMap should fire once per map name"), PreLoadCount, 2);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
