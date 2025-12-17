[CONTEXT_ANCHOR]
ID: 20250208_1541 | Plugin: SOTS_UI | Pass/Topic: StreamableHandleForwardDecl | Owner: Buddy
Scope: ShaderWarmup subsystem forward declaration compatibility fix.

DONE
- Changed FStreamableHandle forward declaration in ShaderWarmupSubsystem.h from class to struct.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Forward-decl adjustment resolves compile error; runtime behavior unchanged.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h

NEXT (Ryan)
- Rebuild to confirm ShaderWarmup compiles.
- Quick warmup run to ensure delegates still bind and dispatch.

ROLLBACK
- Revert the touched header file.