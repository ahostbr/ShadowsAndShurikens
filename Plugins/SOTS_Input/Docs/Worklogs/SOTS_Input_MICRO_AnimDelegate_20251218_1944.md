# SOTS_Input MICRO AnimDelegate (2025-12-18 19:44)

- Switched montage delegates in `SOTS_InputBufferComponent` to UE5.7 dynamic binding (`AddDynamic/RemoveDynamic`) and marked handlers as `UFUNCTION`.
- Removes invalid `FDelegateHandle` usage against multicast script delegates; aligns with engine signature changes.
- No behavior changes intended; no build/run performed in this log.
