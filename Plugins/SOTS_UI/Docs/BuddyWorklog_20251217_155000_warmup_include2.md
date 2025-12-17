# Buddy Worklog — ShaderWarmup include fix v2

- **Goal**: Remove missing header guard error and include the correct engine header that defines `FCoreUObjectDelegates`.
- **Changes**: Dropped the `__has_include` block for `CoreUObjectDelegates` and now rely on `UObject/UObjectGlobals.h` (where `FCoreUObjectDelegates` is declared in UE5.7p/5.7.1).
- **Files**: Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- **Notes/Risks**: Behavior unchanged; should resolve missing header compile error. UNVERIFIED build.
- **Verification**: UNVERIFIED (no build/run executed).
- **Cleanup**: Deleted Plugins/SOTS_UI/Binaries and Intermediate.
- **Follow-ups**: Ryan to rebuild SOTS_UI on UE 5.7p; confirm the compile succeeds without header errors.
