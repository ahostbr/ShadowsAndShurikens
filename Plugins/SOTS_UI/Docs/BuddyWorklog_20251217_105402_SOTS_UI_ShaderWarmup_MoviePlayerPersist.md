# Buddy Worklog - SOTS_UI Shader Warmup MoviePlayer Persistence (2025-12-17 10:54:02)

**Goal**: Use the real UMG shader warmup widget in MoviePlayer and keep it visible across OpenLevel while warmup runs.

**Changes**
- Warmup subsystem now creates the loading widget via the router and stores a weak reference for MoviePlayer.
- MoviePlayer setup uses the widget's Slate (`TakeWidget()`), with manual stop enabled; PreLoadMap plays the movie and EndWarmup stops it.
- Added cleanup: unbind Pre/Post map delegates on EndWarmup and Deinitialize, reset movie state flags.
- Added router helpers to create widgets by id and optionally push a provided widget instance (for consistent ownership).

**Files touched**
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h

**Verification**
- Not run (code-only change; no build executed per SOTS law).

**Notes**
- MoviePlayer uses a router-created UMG instance; the viewport widget is still pushed via the router to avoid reparenting conflicts.
