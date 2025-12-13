# SOTS UDSBridge Notes

## Purpose
- Thin bridge that discovers UDS, player pawn, and DLWE component; pushes sun direction into GSM and applies DLWE weather policy via settings surface or toggle functions.
- No rendering or effects: only drives UDS DLWE_Interaction state.

## Key Runtime Hooks
- Console command: `SOTS.UDSBridge.Refresh` (forces rescan + immediate policy apply).
- Telemetry (gated): compact line with UDS/DLWE/GSM presence, weather flags, apply mode, and toggles. Verbose adds sun dir and actor name.
- Warnings (gated + throttled): missing UDS/DLWE/GSM/apply surface; warn-once plus 10s throttle.

## Apply Modes
- SettingsAsset: uses `DLWE_Func_SetInteractionSettings` with weather-specific assets (Snow/Rain/Dust/Clear) when signature is single UObject* param.
- ToggleFunctions: uses `DLWE_EnableSnow/Rain/Dust_Function` when each is a single bool param.
- If neither signature matches, mode is None and a warning is issued.

## Config Highlights
- Discovery: soft ref (preferred), tag, or name-contains for UDS; name-contains for DLWE component.
- Weather: map snow/rain/dust via bool property names or dot-paths.
- Sun: function returning FVector, light actor property, or rotator property; optional forward inversion.
- Telemetry: `bEnableBridgeTelemetry`, interval; `bEnableVerboseTelemetry` for sun dir + actor name.
- Policy toggles: `bEnableSnowInteractionWhenSnowy`, `bEnablePuddlesWhenRaining`, `bEnableDustInteractionWhenDusty`.

### Quick config example (common UDS setup)
```
UDSActorSoftRef: /Game/World/DayNightSky.DayNightSky
UDSActorTag: UDS
UDSActorNameContains: UDS
DLWEComponentNameContains: DLWE_Interaction

Weather_bSnowy_Property: ED_Snowy
Weather_bRaining_Property: ED_Raining
Weather_bDusty_Property: ED_Dusty

SunDirectionFunctionName: GetSunDirection
SunLightActorProperty: SunLight
SunWorldRotationProperty: SunWorldRotation
bInvertLightForwardVector: true

DLWE_Func_SetInteractionSettings: SetInteractionSettings
DLWE_Settings_Snow: /Game/DLWE/Settings/SnowSettings.SnowSettings
DLWE_Settings_Rain: /Game/DLWE/Settings/RainSettings.RainSettings
DLWE_Settings_Dust: /Game/DLWE/Settings/DustSettings.DustSettings
DLWE_Settings_Clear: /Game/DLWE/Settings/ClearSettings.ClearSettings

DLWE_EnableSnow_Function: SetSnowEnabled
DLWE_EnableRain_Function: SetRainEnabled
DLWE_EnableDust_Function: SetDustEnabled
bEnableBridgeTelemetry: true
bEnableVerboseTelemetry: false
TelemetryIntervalSeconds: 2.0
```

### Telemetry sample
```
UDSBridge | UDS=OK DLWE=OK GSM=OK | Snow=1 Rain=0 Dust=0 | SunDirValid=1 | AppliedMode=Settings | SnowOn=1 PudOn=0 DustOn=0 | Settings=SnowSettings
```

### Console workflow
- Run `SOTS.UDSBridge.Refresh` to force rediscovery and apply once immediately (includes log of what was found).
- Telemetry respects `bEnableBridgeTelemetry`; warnings respect `bEnableBridgeWarnings` and are throttled.

## GSM Interaction
- Pushes dominant sun direction into `USOTS_GlobalStealthManagerSubsystem`; change-detected with small tolerance.

## Hygiene / Safety
- No ticks beyond timer-driven updates with jitter.
- Reflection calls are signature-checked (single bool or single UObject* param) before `ProcessEvent`.
- Do not build in this workflow; clean plugin Binaries/Intermediate after edits.