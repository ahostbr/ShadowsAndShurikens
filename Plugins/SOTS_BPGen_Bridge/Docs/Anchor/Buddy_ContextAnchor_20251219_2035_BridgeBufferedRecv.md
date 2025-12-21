[CONTEXT_ANCHOR]
ID: 20251219_2035 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeBufferedRecv | Owner: Buddy
Scope: Accept loop pending-check plus buffered newline receive and response logging/normalization.

DONE
- Accept loop now checks HasPendingConnection with 50ms backoff before calling Accept.
- Connection handler buffers UTF-8 receives into a FString, splits on `\n`, trims `\r`, and logs first 50 bytes in hex before parsing the first complete message; enforces MaxRequestBytes in the loop.
- SendResponseAndClose trims trailing CR/LF, re-appends a single `\n`, logs response length/string (Verbose), then uses the existing retry loop.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Buffered newline handling and logging should improve malformed JSON diagnosis; assumes single request per connection.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, rerun bpgen_ping; confirm responses are valid JSON and tail with one newline.
- If malformed persists, grab the Verbose response log and hex recv sample to inspect framing.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp; restore plugin binaries/intermediate if needed.
