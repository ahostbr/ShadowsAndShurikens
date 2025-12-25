# SOTS_Core Lifecycle Listener Integration

This doc describes how other plugins can receive UE-native lifecycle callbacks without adding
dependencies to SOTS_Core. The mechanism uses modular features and a lightweight C++ interface.

## Pseudocode (illustrative, not full compilable code)
```
class FSOTS_InputLifecycleHook : public ISOTS_CoreLifecycleListener
{
public:
    virtual void OnSOTS_PlayerControllerBeginPlay(APlayerController* PC) override
    {
        // forward to SOTS_Input subsystem or router
    }

    virtual void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override
    {
        // update input layer bindings for the new pawn
    }
};

class FSOTS_InputModule : public IModuleInterface
{
    FSOTS_InputLifecycleHook Hook;
    FSOTS_CoreLifecycleListenerHandle Handle;

    void StartupModule() override
    {
        Handle.Register(&Hook);
    }

    void ShutdownModule() override
    {
        Handle.Unregister();
    }
};
```

## Notes
- Keep listener work lightweight; these callbacks fire on the game thread.
- Dispatch is OFF by default; enable via `USOTS_CoreSettings`.
- This avoids adapter layers and avoids direct dependencies on project Blueprints.
