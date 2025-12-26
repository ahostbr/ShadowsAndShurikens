#include "OmniTrace.h"

#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"

#include "OmniTraceDebugSubsystem.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"
#include "OmniTraceCoreBridgeSettings.h"
#include "OmniTraceCoreLifecycleHook.h"

DEFINE_LOG_CATEGORY(LogOmniTrace);

namespace
{
    TUniquePtr<FOmniTrace_CoreLifecycleHook> GOmniTraceCoreLifecycleHook;
    TUniquePtr<FSOTS_CoreLifecycleListenerHandle> GOmniTraceCoreLifecycleHandle;
}

FOmniTraceHitResult MakeOmniTraceHitResult(const FHitResult& InHit)
{
    FOmniTraceHitResult Out;
    Out.bBlockingHit = InHit.bBlockingHit;
    Out.Location     = InHit.Location;
    Out.ImpactPoint  = InHit.ImpactPoint;
    Out.Normal       = InHit.Normal;
    Out.HitActor     = InHit.GetActor();
    Out.HitComponent = InHit.GetComponent();
    Out.Distance     = InHit.Distance;
    return Out;
}

#define LOCTEXT_NAMESPACE "FOmniTraceModule"

void FOmniTraceModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const UOmniTraceCoreBridgeSettings* Settings = UOmniTraceCoreBridgeSettings::Get();
    if (Settings && Settings->bEnableSOTSCoreLifecycleBridge)
    {
        GOmniTraceCoreLifecycleHook = MakeUnique<FOmniTrace_CoreLifecycleHook>();
        GOmniTraceCoreLifecycleHandle = MakeUnique<FSOTS_CoreLifecycleListenerHandle>(GOmniTraceCoreLifecycleHook.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogOmniTrace, Verbose, TEXT("OmniTrace CoreBridge: Registered Core lifecycle listener."));
        }
    }
#endif
}

void FOmniTraceModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    if (GOmniTraceCoreLifecycleHook)
    {
        GOmniTraceCoreLifecycleHook->Shutdown();
    }

    GOmniTraceCoreLifecycleHandle.Reset();
    GOmniTraceCoreLifecycleHook.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOmniTraceModule, OmniTrace)
