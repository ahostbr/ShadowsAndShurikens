
Keep this file simple, but concrete enough that future you can hook SOTS_Steam into MissionDirector / GSM without re-reading all plugin code.

---

### 7. Sanity Checks & Cleanup

Before you finish this pass:

1. `.uplugin`:
   - Type = `Runtime`
   - `CanContainContent` = `false`
   - Category = `"Online"`

2. Build.cs:
   - `PCHUsage` set to `UseExplicitOrSharedPCHs`.
   - Dependencies clean and minimal.
   - `OnlineSubsystem` + `OnlineSubsystemUtils` in Private deps.
   - `OnlineSubsystemSteam` in `DynamicallyLoadedModuleNames`.

3. Settings:
   - `USOTS_SteamSettings` is a proper `UDeveloperSettings` with `config=Game, defaultconfig`.
   - Categories & tooltips grouped as described.
   - `GetCategoryName` / section text consistent with your other SOTS_* plugins.

4. Blueprint categories:
   - All SOTS_Steam APIs use `SOTS|Steam|...` prefix.
   - Debug utilities under `SOTS|Steam|Debug|...`.

5. Logging:
   - `LogSOTS_Steam` used consistently.
   - Verbose detail respects `bEnableVerboseLogging`.

6. Docs:
   - `Plugins/SOTS_Steam/Docs/SOTS_Steam_QuickStart.md` exists and is filled in as above.

**Cleanup per SOTS laws:**

- If present, delete:
  - `Plugins/SOTS_Steam/Binaries/`
  - `Plugins/SOTS_Steam/Intermediate/`
- Do **not** trigger a build.

---

That’s Pass 10 done: SOTS_Steam should now look and feel like a first-class, shippable SOTS plugin with clear settings, clean Blueprint API, and a built-in QuickStart so you never have to guess “how did we wire Steam into this again?”.
::contentReference[oaicite:0]{index=0}
