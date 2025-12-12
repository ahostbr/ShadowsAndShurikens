#include "OmniTrace.h"

#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"

#include "OmniTraceDebugSubsystem.h"

DEFINE_LOG_CATEGORY(LogOmniTrace);

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
}

void FOmniTraceModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOmniTraceModule, OmniTrace)
