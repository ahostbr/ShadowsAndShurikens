# Buddy Worklog — ShaderWarmup include fallback

- **Goal**: Fix missing `CoreUObjectDelegates` header on UE 5.7p build of SOTS_UI.
- **Changes**: Added `__has_include` fallback in `SOTS_ShaderWarmupSubsystem.cpp` to include `UObject/CoreUObjectDelegates.h` or `CoreUObjectDelegates.h` depending on engine layout; emits explicit error if neither is available.
- **Files**: Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- **Notes/Risks**: Behavior unchanged; only include path adjusted for engine layout differences. UNVERIFIED build.
- **Verification**: UNVERIFIED (no build/run executed).
- **Cleanup**: Deleted Plugins/SOTS_UI/Binaries and Intermediate.
- **Follow-ups**: Ryan to rebuild SOTS_UI on UE 5.7p; if header still missing, confirm actual engine header name/path and adjust include accordingly.
