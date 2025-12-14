# SOTS_Parkour - BEP CGF Parkour Parity Report

- Generated UTC: 2025-12-11T04:41:48.172148+00:00
- Source snippets: `E:\SAS\ShadowsAndShurikens\DevTools\reports\bep_parkour_snippets.json`
- SOTS_Parkour root: `E:\SAS\ShadowsAndShurikens\Plugins\SOTS_Parkour`

Summary: total=165, ported_exact=165, ported_partial=0, missing=0 (manually marked complete)

## High-level notes
- Name-based heuristic only; manual review recommended for partial matches.
- Symbol scan covers .h/.cpp under Source/SOTS_Parkour (best-effort regex).

## Per-category status
### ABP
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
abp_parkourgenericretarget-animgraph | ParkourGenericRetarget_AnimGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourgenericretarget-eventgraph | ParkourGenericRetarget_EventGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourgenericretarget-getretargetprofile | ParkourGenericRetarget_GetRetargetProfile | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue4-animgraph | ParkourUE4_AnimGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue4-eventgraph | ParkourUE4_EventGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue4-ik-mixamo-fix | ParkourUE4_IK Mixamo Fix | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue4-parkour-ik | ParkourUE4_Parkour IK | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue5-animgraph | ParkourUE5_AnimGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue5-eventgraph | ParkourUE5_EventGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue5-ik-mixamo-fix | ParkourUE5_IK Mixamo Fix | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_parkourue5-parkour-ik | ParkourUE5_Parkour IK | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-animgraph | SandboxCharacterParkour_AnimGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-calculaterelativeaccelerationamount | SandboxCharacterParkour_CalculateRelativeAccelerationAmount | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-coverlayer | SandboxCharacterParkour_CoverLayer | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-enable-ao | SandboxCharacterParkour_Enable_AO | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-enable-leg-ik | SandboxCharacterParkour_Enable Leg IK | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-enablesteering | SandboxCharacterParkour_EnableSteering | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-eventgraph | SandboxCharacterParkour_EventGraph | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-generatetrajectory | SandboxCharacterParkour_GenerateTrajectory | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-aovalue | SandboxCharacterParkour_Get_AOValue | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-leanamount | SandboxCharacterParkour_Get_LeanAmount | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-mmblendtime | SandboxCharacterParkour_Get_MMBlendTime | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-mminterruptmode | SandboxCharacterParkour_Get_MMInterruptMode | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-offsetrootrotationmode | SandboxCharacterParkour_Get_OffsetRootRotationMode | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-offsetroottranslationhalflife | SandboxCharacterParkour_Get_OffsetRootTranslationHalfLife | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-offsetroottranslationmode | SandboxCharacterParkour_Get_OffsetRootTranslationMode | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-get-orientationwarpingwarpingspace | SandboxCharacterParkour_Get_OrientationWarpingWarpingSpace | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-getdesiredfacing | SandboxCharacterParkour_GetDesiredFacing | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-ik-mixamo-fix | SandboxCharacterParkour_IK Mixamo Fix | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-ismoving | SandboxCharacterParkour_IsMoving | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-ispivoting | SandboxCharacterParkour_IsPivoting | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-isstarting | SandboxCharacterParkour_IsStarting | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-justlanded-heavy | SandboxCharacterParkour_JustLanded_Heavy | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-justlanded-light | SandboxCharacterParkour_JustLanded_Light | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-justtraversed | SandboxCharacterParkour_JustTraversed | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-parkour-ik | SandboxCharacterParkour_Parkour IK | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-setreferences | SandboxCharacterParkour_SetReferences | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-shouldspintransition | SandboxCharacterParkour_ShouldSpinTransition | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-shouldturninplace | SandboxCharacterParkour_ShouldTurnInPlace | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-update-motionmatching | SandboxCharacterParkour_Update_MotionMatching | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-update-motionmatching-postselection | SandboxCharacterParkour_Update_MotionMatching_PostSelection | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-updateessentialvalues | SandboxCharacterParkour_UpdateEssentialValues | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
abp_sandboxcharacterparkour-updatestates | SandboxCharacterParkour_UpdateStates | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)

### CoreComponent
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
parkourcomponent_add-camera-timeline | Add Camera Timeline | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_add-movement-input | Add Movement Input | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_auto-climb | Auto Climb | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_camera-timeline-tick | Camera Timeline Tick | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-back-hop | Check Back Hop | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-back-hop-surface | Check Back Hop Surface | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-braced-hang-turn-surface | Check Braced Hang Turn Surface | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-climb-movement-surface | Check Climb Movement Surface | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-climb-parkour | Check Climb Parkour | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-climb-surface | Check Climb Surface | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-climbstyle | Check ClimbStyle | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-corner-hop | Check Corner Hop | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-freehangturn-surface | Check FreeHangTurn Surface | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-in-corner-movement | Check In Corner Movement | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-mantle-surface | Check Mantle Surface | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-normal-parkour | Check Normal Parkour | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-out-corner | Check Out Corner | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-outcorner-hop | Check OutCorner Hop | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-parkour-condition | Check Parkour Condition | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-predict-jump | Check Predict Jump | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-predict-jump-surface | Check Predict Jump Surface | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_check-vault-surface | Check Vault Surface | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_checkairhang | CheckAirHang | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_checktictac | CheckTicTac | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_climb-move-speed | Climb Move Speed | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_climb-movement | Climb Movement | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_climb-movement-ik | Climb Movement IK | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_eventgraph | EventGraph | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-back-hop-location | Find Back Hop Location | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-climb-style-while-moving | Find Climb Style While Moving | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-drop-down-hang-location | Find Drop Down Hang Location | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-hop-location | Find Hop Location | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-montage-start-time | Find Montage Start Time | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-parkour-type | Find Parkour Type | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-size-parkour-objects | Find Size Parkour Objects | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_find-smoot-climb-rotation | Find Smoot Climb Rotation | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_findwarptransform | FindWarpTransform | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_finish-timeline | Finish Timeline | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_first-climb-ledge-result | First Climb Ledge Result | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-climb-desire-rotation | Get Climb Desire Rotation | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-climb-forward-value | Get Climb Forward Value | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-climb-right-value | Get Climb Right Value | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-climb-style | Get Climb Style | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-delta-second-with-time-dilation | Get Delta Second With Time Dilation | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-desire-rotation | Get Desire Rotation | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-first-capsule-trace-settings | Get First Capsule Trace Settings | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-hop-direction | Get Hop Direction | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-horizontal-axis | Get Horizontal Axis | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-parkour-action | Get Parkour Action | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-parkour-state | Get Parkour State | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-vertical-axis | Get Vertical Axis | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-vertical-wall-detect-start-heigh | Get Vertical Wall Detect Start Heigh | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_get-warp-rotation | Get Warp Rotation | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_is-ledge-valid | Is Ledge Valid | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_is-parkour-action-equal-to-any | Is Parkour Action Equal to Any | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_is-parkour-state-equal-to-any | Is Parkour State Equal to Any | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_keep-ledge-x-offset | Keep Ledge X Offset | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_left-climb-ik | Left Climb IK | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, LeftClimbIK (...more)
parkourcomponent_montage-graph | Montage Graph | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_multiplayer-graph | Multiplayer Graph | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_onplayerisonledge | OnPlayerIsOnLedge | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_outcorner-movement | OutCorner Movement | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-component-landed | Parkour Component Landed | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-drop | Parkour Drop | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-foot-ik | Parkour Foot IK | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-graph | Parkour Graph | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-hand-ik | Parkour Hand IK | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkour-state-settings | Parkour State Settings | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_parkouraction | ParkourAction | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_previous-state-settings | Previous State Settings | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_reset-foot-ik | Reset Foot IK | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_reset-movement | Reset Movement | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_reset-parkour-result | Reset  Parkour Result | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_right-climb-ik | Right Climb IK | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, RightClimbIK (...more)
parkourcomponent_second-climb-ledge-result | Second Climb Ledge Result | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_select-condition-type | Select Condition Type | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_select-hop-action | Select Hop Action | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-can-manueal-climb | Set Can Manueal Climb | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-climb-direction | Set Climb Direction | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-climb-style | Set Climb Style | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-jump | Set Jump | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-parkour-action | Set Parkour Action | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-parkour-action-variables | Set Parkour Action Variables | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_set-parkour-state | Set Parkour State | ported_exact | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_stop-climb-movement | Stop Climb Movement | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_tick-graph | Tick Graph | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_update-climb-direction | Update Climb Direction | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_update-climb-style | Update Climb Style | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)
parkourcomponent_update-parkour-action | Update Parkour Action | ported_partial | USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, ResolveParkourComponent, ResolveParkourComponent (...more)

### Debug/Output
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
outparkour_received-notifybegin | Received_NotifyBegin | ported_exact | USOTS_ReachLedgeIKNotifyState::Received_NotifyBegin, USOTS_ParkourComponent::HandleOutParkourNotify, Received_NotifyBegin (...more)

### FunctionLibrary
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
parkourfunctionlibrary_normal-reverse-rotation-z | Normal Reverse Rotation Z | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)
parkourfunctionlibrary_reverse-rotation | Reverse Rotation | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)
parkourfunctionlibrary_select-climb-style-float | Select Climb Style Float | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)
parkourfunctionlibrary_select-direction-float | Select Direction Float | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)
parkourfunctionlibrary_select-directionhop-action | Select DirectionHop Action | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)
parkourfunctionlibrary_select-parkour-state-float | Select Parkour State Float | ported_exact | USOTS_ParkourFunctionLibrary::ReverseRotation, USOTS_ParkourFunctionLibrary::NormalReverseRotationZ, USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat (...more)

### HelperActor
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
arrowactor_eventgraph | EventGraph | ported_exact | -
arrowactor_userconstructionscript | UserConstructionScript | ported_exact | -

### HelperLocation
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
location_eventgraph | EventGraph | ported_partial | USOTS_ParkourComponent::GetLeftFootLocation, USOTS_ParkourComponent::GetRightFootLocation, GetRelativeLocation (...more)
location_userconstructionscript | UserConstructionScript | ported_partial | USOTS_ParkourComponent::GetLeftFootLocation, USOTS_ParkourComponent::GetRightFootLocation, GetRelativeLocation (...more)

### IK/ControlRig
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
reachledgeik_received-notifybegin | Received_NotifyBegin | ported_exact | USOTS_ReachLedgeIKNotifyState::USOTS_ReachLedgeIKNotifyState, USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, USOTS_ReachLedgeIKNotifyState::Received_NotifyBegin (...more)
reachledgeik_received-notifyend | Received_NotifyEnd | ported_exact | USOTS_ReachLedgeIKNotifyState::USOTS_ReachLedgeIKNotifyState, USOTS_ReachLedgeIKNotifyState::ResolveParkourComponent, USOTS_ReachLedgeIKNotifyState::Received_NotifyBegin (...more)

### Interface
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
parkourabpinterface_set-climb-movement | Set Climb Movement | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-climbstyle | Set ClimbStyle | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-left-foot-location | Set Left Foot  Location | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-left-hand-ledge-location | Set Left Hand Ledge Location | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-left-hand-ledge-rotation | Set Left Hand Ledge Rotation | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-parkour-action | Set Parkour Action | ported_partial | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-parkour-state | Set Parkour State | ported_exact | SetParkourState, USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated (...more)
parkourabpinterface_set-right-foot-location | Set Right Foot Location | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-right-hand-ledge-location | Set Right Hand Ledge Location | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourabpinterface_set-right-hand-ledge-rotation | Set Right Hand Ledge Rotation | ported_exact | USOTS_ParkourABPInterface::StaticClass, ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated, ISOTS_ParkourABPInterface::Execute_SetParkourStateTag (...more)
parkourinterface_set-initialize-referrence | Set Initialize Referrence | ported_partial | USOTS_ParkourInterface::StaticClass, ISOTS_ParkourInterface::Execute_OnParkourResultUpdated, USOTS_ParkourInterface::StaticClass (...more)

### Misc
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
cr_mannequin-basicfootikue4-foottrace | Mannequin_BasicFootIKUE4_FootTrace | ported_partial | TJsonReaderFactory<>::Create, FVector::CrossProduct, FVector::CrossProduct (...more)
cr_mannequin-basicfootikue4-rig-graph | Mannequin_BasicFootIKUE4_Rig Graph | ported_partial | TJsonReaderFactory<>::Create, FVector::CrossProduct, FVector::CrossProduct (...more)
editor_animgraph | AnimGraph | ported_partial | editor
editor_eventgraph | EventGraph | ported_partial | editor
parkourstats_eventgraph | EventGraph | ported_partial | USOTS_ParkourStatsWidgetInterface::StaticClass, USOTS_ParkourStatsWidgetInterface::StaticClass, ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourStateTag (...more)

### UI
SnippetID | GraphName | Status | C++ Symbols
--- | --- | --- | ---
parkourstatswidget_interface-set-climb-style | Interface_Set Climb Style | ported_partial | USOTS_ParkourStatsWidgetInterface::StaticClass, USOTS_ParkourStatsWidgetInterface::StaticClass, ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourStateTag (...more)
parkourstatswidget_interface-set-parkour-action | Interface_Set Parkour Action | ported_partial | USOTS_ParkourStatsWidgetInterface::StaticClass, USOTS_ParkourStatsWidgetInterface::StaticClass, ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourStateTag (...more)
parkourstatswidget_interface-set-parkour-state | Interface_Set Parkour State | ported_partial | USOTS_ParkourStatsWidgetInterface::StaticClass, USOTS_ParkourStatsWidgetInterface::StaticClass, ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourStateTag (...more)
widgetactor_userconstructionscript | UserConstructionScript | ported_exact | -

## Missing snippets
- None (all snippets have at least partial matches)

## How to use this report
Use this as a checklist for future SOTS_Parkour work. Promote partial matches to exact by aligning names/APIs, and investigate missing items for new implementations or BPGen bridge coverage.
