# SOTS Context Anchor Generator

Goal: produce a rolling, deduped [CONTEXT_ANCHOR] block for the end of the current chat response.

1) Summarize the last 3 assistant messages:
   - what decisions/locks were made
   - what was planned
   - what was confirmed done (ONLY if verified)
2) Merge any prior anchors found in those last 3 assistant messages.
3) Output ONLY the final [CONTEXT_ANCHOR] in a single fenced code block.
