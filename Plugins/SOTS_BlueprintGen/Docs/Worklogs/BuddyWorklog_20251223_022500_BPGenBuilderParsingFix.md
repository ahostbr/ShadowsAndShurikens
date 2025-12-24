goal
- Fix the BPGen builder parsing helpers so SOTS_BlueprintGen and the bridge compile cleanly under UE 5.7 without missing-header dependencies.

what changed
- Reworked `FillPinTypeFromBPGen`/`TryResolvePinAlias` to stop referencing undefined tokens and to only accept valid pin descriptors.
- Added `<cstdlib>` plus UTF-8 conversions and replaced the float/int parsing helpers with `std::strtof`/`std::strtoll` to avoid unavailable engine headers.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

notes + risks/unknowns
- Build has not been rerun yet; there could still be hidden compilation issues or runtime parsing gaps.
- New parsing relies on the standard library conversion functions; confirm platform support and that the input string strictly consumes the whole value.

verification status
- UNVERIFIED (no Unreal build or bridge invocation executed after these edits).

follow-ups / next steps
- Rebuild SOTS_BlueprintGen/SOTS_BPGen_Bridge under UE 5.7 to ensure no missing-symbol errors remain.
- Restart the BPGen bridge and execute the `create_data_asset` request for `/Game/SOTS/Data/testDA.uasset` to confirm success.
