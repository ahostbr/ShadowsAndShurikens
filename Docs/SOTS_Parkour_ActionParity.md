CGF Parkour → SOTS action-tag parity

- Source data: `Content/UEFNParkourActionVariablesDT.json` (39 CGF Parkour actions) vs. runtime tag mapping in `SOTS_ParkourComponent::GetActionTag`.
- SOTS exposes 8 logical action tags (Mantle, Vault, LedgeMove, Drop, TicTac, BackHop, PredictJump, AirHang) and folds multiple CGF rows into a single tag in several cases.
- 24 CGF actions are not represented at all; 15 are only covered via coarse grouping; 6 SOTS-only tags are absent from CGF.

| CGF action tag | In SOTS? | Notes |
| --- | --- | --- |
| Parkour.Action.Mantle | Yes (coarse) | Mapped via Mantle (covers Mantle/Low/Distance) |
| Parkour.Action.LowMantle | Yes (coarse) | Mapped via Mantle (covers Mantle/Low/Distance) |
| Parkour.Action.ThinVault | Yes (coarse) | Mapped via Vault (coarse) |
| Parkour.Action.Vault | Yes (coarse) | Mapped via Vault (coarse) |
| Parkour.Action.HighVault | Yes (coarse) | Mapped via Vault (coarse) |
| Parkour.Action.Climb | No | Not represented in SOTS actions |
| Parkour.Action.ClimbingUp | No | Not represented in SOTS actions |
| Parkour.Action.FreeHangClimb | No | Not represented in SOTS actions |
| Parkour.Action.FreeHangClimbUp | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopUp | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopLeft | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopRight | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopLeftUp | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopRightUp | No | Not represented in SOTS actions |
| Parkour.Action.ClimbHopDown | No | Not represented in SOTS actions |
| Parkour.Action.FreeClimbHopLeft | No | Not represented in SOTS actions |
| Parkour.Action.FreeClimbHopRight | No | Not represented in SOTS actions |
| Parkour.Action.FreeClimbHopDown | No | Not represented in SOTS actions |
| Parkour.Action.DropDown | No | Not represented in SOTS actions |
| Parkour.Action.FreeHangDropDown | No | Not represented in SOTS actions |
| Parkour.Action.FallingBraced | No | Not represented in SOTS actions |
| Parkour.Action.FallingFreeHang | No | Not represented in SOTS actions |
| Parkour.Action.VaultToBraced | Yes (coarse) | Mapped via Vault (coarse) |
| Parkour.Action.ClimbUpVault | No | Not represented in SOTS actions |
| Parkour.Action.BackHopBracedClimb | Yes (coarse) | Mapped via BackHop (coarse) |
| Parkour.Action.BackHopFreeHangClimb | Yes (coarse) | Mapped via BackHop (coarse) |
| Parkour.Action.BackHopToFalling | Yes (coarse) | Mapped via BackHop (coarse) |
| Parkour.Action.BracedClimbTurn | No | Not represented in SOTS actions |
| Parkour.Action.FreeHangClimbTurn | No | Not represented in SOTS actions |
| Parkour.Action.PredictJumpLong | Yes (coarse) | Mapped via PredictJump (coarse) |
| Parkour.Action.PredictJumpShortL | Yes (coarse) | Mapped via PredictJump (coarse) |
| Parkour.Action.PredictJumpShortR | Yes (coarse) | Mapped via PredictJump (coarse) |
| Parkour.Action.DistanceIdleToClimb | No | Not represented in SOTS actions |
| Parkour.Action.DistanceIdleToFreeHang | No | Not represented in SOTS actions |
| Parkour.Action.DistanceMantle | Yes (coarse) | Mapped via Mantle (covers Mantle/Low/Distance) |
| Parkour.Action.HighFallingBraced | No | Not represented in SOTS actions |
| Parkour.Action.HighFallingFreeHang | No | Not represented in SOTS actions |
| Parkour.Action.TicTacLeftBracedHang | Yes (coarse) | Mapped via TicTac (coarse) |
| Parkour.Action.TicTacRightBracedHang | Yes (coarse) | Mapped via TicTac (coarse) |

SOTS-only action tags (not present in CGF list): `Parkour.Action.AirHang`, `Parkour.Action.BackHop`, `Parkour.Action.Drop`, `Parkour.Action.LedgeMove`, `Parkour.Action.PredictJump`, `Parkour.Action.TicTac`.

Recon notes (SOTS implementation quick recap)
- Detection is event-driven via `PerformParkourDetection`; traces use OmniTrace wrappers and obey LOD/budget gating.
- The component stores `ESOTS_ParkourAction`/`ESOTS_ParkourResultType` and resolves tags in `GetActionTag` (coarse mapping above) plus state tags in `UpdateGameplayTagsFromState`.
- Animation bridge: `OnParkourStarted/Ended` + `UpdateParkourAction`/`UpdateParkourState` notify actor and AnimBP interfaces; motion-warp targets are precomputed per action result.