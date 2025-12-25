# Context Anchor Rules (MANDATORY)

- Every assistant response should include one anchor at the bottom in a single codeblock.
- Anchor summarizes the last 3 chat messages.
- If any of those last 3 messages contain anchors, merge them forward (rolling/compounding).
- Promote verified-done items into passes_confirmed_done; remove duplicates and contradictions.
- Never list the same item in multiple sections.
