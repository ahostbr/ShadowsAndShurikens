# Buddy Worklog — UE5.7 ComponentOps MakeError overload

Goal
- Unblock UE 5.7.1 compilation by fixing `SOTS_BPGenBridgeComponentOps.cpp` returning the wrong `MakeError(...)` overload.

What changed
- Added `MakeError` overloads that accept `const TCHAR*` (and mixed `TCHAR*`/`FString`) so calls using `TEXT("...")` prefer our helper and do not resolve to UE's `TValueOrError` `MakeError` template.

Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeComponentOps.cpp

Notes / risks / unknowns
- UNVERIFIED: This was validated only via code inspection and editor diagnostics; needs a full UE build to confirm the original `C2440` return-conversion errors are gone.
- If UE introduces additional overloads/macros, renaming the helper away from `MakeError` might still be preferable, but this patch aims to be minimal.

Verification status
- UNVERIFIED: No engine build run here.

Next steps (Ryan)
- Rebuild `ShadowsAndShurikensEditor` and confirm `SOTS_BPGen_Bridge` compiles past `SOTS_BPGenBridgeComponentOps.cpp`.
