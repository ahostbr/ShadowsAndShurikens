# SOTS GSM — Minimum Tag Schema (Prompt 5)

**What GSM writes**
- AI awareness tiers (per-AI): SAS.AI.Alert.{Calm,Investigating,Searching,Alerted,Hostile}
- AI focus tags (per-AI): SAS.AI.Focus.{Player,Unknown}
- AI reason tags (per-AI, forwarded/fallback): SAS.AI.Reason.{Sight,Hearing,Shadow,Damage,Generic}
- Stealth tier tags (player): SOTS.Stealth.State.{Hidden,Cautious,Danger,Compromised}
- Light/shadow tags (player): SOTS.Stealth.Light.{Bright,Dark}
- Global alertness bands (player/global): SAS.Global.Alertness.{Calm,Tense,Alert,Critical}

**Required vs optional**
- Required for GSM correctness: all of the above. GSM skips any missing tag safely.
- No new reason schemas are invented; incoming ReasonTag is forwarded when valid.

**Missing-tag behavior**
- Tags are requested via `RequestTagSafe` (ErrorIfNotFound=false).
- If a requested tag is missing, GSM logs one warning per missing tag name and skips applying it.
- Tag writes always check `IsValid()` before add/remove.

**Where applied**
- Per-AI awareness/focus/reason tags applied to the AI actor via `SOTS_TagLibrary` in `ApplyAwarenessTags`.
- Player stealth tier and light/dark tags applied to the player actor in `ApplyStealthStateTags` / `UpdateGameplayTags`.
- Global alertness band tags applied to the player actor in `ApplyGlobalAlertnessTags`.
