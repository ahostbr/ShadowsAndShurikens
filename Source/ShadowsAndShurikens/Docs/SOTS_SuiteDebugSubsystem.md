# SOTS Suite Debug Subsystem Usage

This subsystem aggregates read-only, short-status summaries from core SOTS subsystems and provides a single log dump helper.

How to call from C++:

```cpp
if (USOTS_SuiteDebugSubsystem* Debug = USOTS_SuiteDebugSubsystem::Get(WorldContextObject))
{
    Debug->DumpSuiteStateToLog();
}
```

How to call from Blueprint:
- Drag a `Get SOTS Suite Debug Subsystem` node (callable via Get with WorldContext), then call `DumpSuiteStateToLog`.

Widget helper & Blueprint
- The read-only suite debug subsystem now lives in the `SOTS_Debug` plugin (`Plugins/SOTS_Debug/Source/SOTS_Debug/Public/SOTS_SuiteDebugSubsystem.h`), but a base UMG widget remains in the game module as `USOTS_SuiteDebugWidget` (see `Source/ShadowsAndShurikens/Public/SOTS_SuiteDebugWidget.h`). The widget still exposes getters that forward to `USOTS_SuiteDebugSubsystem` so UMG blueprints can bind text directly.
    - To use: create a `WBP_SOTS_SuiteDebugOverlay` blueprint that inherits from `USOTS_SuiteDebugWidget` and bind `TextBlock` text to the functions, e.g. `GetGSMStateText()`.

Notes:
- This is intentionally read-only and uses `FindComponentByClass` on the first player pawn to extract player-specific data (stats, abilities, inventory).
- The subsystem depends on the following plugins: SOTS_TagManager, SOTS_INV, SOTS_GlobalStealthManager, SOTS_GAS_Plugin, SOTS_MMSS, SOTS_MissionDirector, SOTS_FX_Plugin, and SOTS_Stats.
