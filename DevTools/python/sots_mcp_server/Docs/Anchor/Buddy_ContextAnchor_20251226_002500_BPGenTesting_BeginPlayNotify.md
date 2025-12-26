Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_002500 | Plugin: sots_mcp_server | Pass/Topic: BPGen_BeginPlay_Notify_BP_BPGen_Testing | Owner: Buddy
Scope: Attempted to author BP_BPGen_Testing BeginPlay->Delay->ShowNotification via BPGen; recorded spawner keys; blocked by Unreal/BPGen connectivity.

DONE
- Ran RAG index + query for BPGen node wiring/subsystem-get patterns.
- Confirmed required BP node spawner keys exist:
  - Event BeginPlay
  - KismetSystemLibrary::Delay
  - Get SOTS_UIRouterSubsystem
  - SOTS_UIRouterSubsystem::ShowNotification

VERIFIED
- None (no mutation/compile/save; Unreal not connected).

UNVERIFIED / ASSUMPTIONS
- Once Unreal is running and BPGen bridge is reachable on 55557, these nodes can be created and wired in `/Game/BPGen/BP_BPGen_Testing` EventGraph.
- ShowNotification call node will expose a target (`self`/`Target`) pin that can be connected from the subsystem getter output (pin name must be confirmed via node describe).

FILES TOUCHED
- DevTools/python/sots_mcp_server/Docs/Worklogs/BuddyWorklog_20251226_002500_BPGenTesting_BeginPlayNotify.md

NEXT (Ryan)
- Open UE project, verify UnrealMCP + BPGen bridge are loaded and listening on 55557.
- Run bpgen ping; then create nodes + connect pins; compile + save BP.

ROLLBACK
- None needed (no mutations were applied).
[/CONTEXT_ANCHOR]
