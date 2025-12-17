# SOTS_UI Shader Warmup Load Lists

## Default load list
- The default warmup load list lives in `USOTS_UISettings::DefaultShaderWarmupLoadList`.
- If a warmup request does not provide an override, the subsystem resolves this setting first.

## Per-mission override (convention)
- Author a data asset named `DA_ShaderWarmup_<MapName>`.
- Store it near the map asset or in `/Game/SOTS/Data/ShaderWarmup/`.
- MissionDirector may pass this asset as the request override when scheduling travel.

## Fallback scan rule
- AssetRegistry dependency scanning runs only when:
  - no load list is resolved, OR
  - the resolved load list exists but contains zero assets.
