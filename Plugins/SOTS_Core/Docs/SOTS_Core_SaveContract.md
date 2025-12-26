# SOTS_Core Save Contract

SOTS_Core defines save-contract interfaces and types only. It does not perform IO,
manage profile folders, or own save slot behavior. Those responsibilities remain in
SOTS_ProfileShared (or a future save orchestration plugin).

## Contract Types
- FSOTS_SaveRequestContext: caller intent for the save request.
- FSOTS_SaveParticipantStatus: quick "can save?" response plus optional block reason.
- FSOTS_SavePayloadFragment: opaque bytes identified by a fragment id.

## Participant Interface
Implement ISOTS_CoreSaveParticipant in your plugin and register via modular features.
Participants must be fast and non-blocking; no IO or heavy work should run here.

## Registration (pseudocode)
```
class FSOTS_SaveParticipant : public ISOTS_CoreSaveParticipant
{
    FName GetParticipantId() const override { return TEXT("MyPlugin"); }
};

void FMyPluginModule::StartupModule()
{
    FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(&Participant);
}

void FMyPluginModule::ShutdownModule()
{
    FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(&Participant);
}
```

## Notes
- SOTS_Core is dependency-root: the contract is intentionally generic and engine-only.
- ProfileShared will decide when to call QuerySaveStatus/BuildSaveFragment/ApplySaveFragment.
