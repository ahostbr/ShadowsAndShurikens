#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Features/IModularFeatures.h"

FSOTS_CoreLifecycleListenerHandle::FSOTS_CoreLifecycleListenerHandle(ISOTS_CoreLifecycleListener* InListener)
{
    Register(InListener);
}

FSOTS_CoreLifecycleListenerHandle::~FSOTS_CoreLifecycleListenerHandle()
{
    Unregister();
}

void FSOTS_CoreLifecycleListenerHandle::Register(ISOTS_CoreLifecycleListener* InListener)
{
    if (bRegistered && Listener == InListener)
    {
        return;
    }

    Unregister();

    if (!InListener)
    {
        return;
    }

    Listener = InListener;

    if (!IModularFeatures::IsAvailable())
    {
        return;
    }

    IModularFeatures::Get().RegisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, Listener);
    bRegistered = true;
}

void FSOTS_CoreLifecycleListenerHandle::Unregister()
{
    if (!bRegistered || !Listener)
    {
        Listener = nullptr;
        bRegistered = false;
        return;
    }

    if (IModularFeatures::IsAvailable())
    {
        IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, Listener);
    }

    Listener = nullptr;
    bRegistered = false;
}

void FSOTS_CoreLifecycleListenerHandle::RegisterListener(ISOTS_CoreLifecycleListener* InListener)
{
    if (!InListener || !IModularFeatures::IsAvailable())
    {
        return;
    }

    IModularFeatures::Get().RegisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, InListener);
}

void FSOTS_CoreLifecycleListenerHandle::UnregisterListener(ISOTS_CoreLifecycleListener* InListener)
{
    if (!InListener || !IModularFeatures::IsAvailable())
    {
        return;
    }

    IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, InListener);
}
