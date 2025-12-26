#pragma once

#include "CoreMinimal.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class SOTS_CORE_API FSOTS_CoreLifecycleListenerHandle
{
public:
    FSOTS_CoreLifecycleListenerHandle() = default;
    explicit FSOTS_CoreLifecycleListenerHandle(ISOTS_CoreLifecycleListener* InListener);
    ~FSOTS_CoreLifecycleListenerHandle();

    void Register(ISOTS_CoreLifecycleListener* InListener);
    void Unregister();

    bool IsRegistered() const { return bRegistered; }
    ISOTS_CoreLifecycleListener* GetListener() const { return Listener; }

    static void RegisterListener(ISOTS_CoreLifecycleListener* InListener);
    static void UnregisterListener(ISOTS_CoreLifecycleListener* InListener);

private:
    ISOTS_CoreLifecycleListener* Listener = nullptr;
    bool bRegistered = false;
};
