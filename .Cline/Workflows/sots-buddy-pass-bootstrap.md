# SOTS Buddy Pass Bootstrap (Prompt Builder)

Goal: produce a SOTS_VSCODE_BUDDY prompt that matches the SOTS laws.

1) Ask me:
   - category (plugin_code/suite_sweep/hotfix/etc.)
   - plugin name
   - pass name
   - branch name
   - exact goal (1 sentence)
2) Identify key files by searching:
   - Use MCP: `sots_search_workspace` for the main symbols/classes involved
3) Batch-read the top relevant files:
   - Use MCP: `sots_read_file`
4) Output:
   - ONE copy/paste codeblock containing a [SOTS_VSCODE_BUDDY] prompt
   - include: GOAL, HARD CONSTRAINTS, BATCH READ list, PATCH INSTRUCTIONS, DONE CRITERIA
5) Enforce:
   - minimal diffs, no builds, cleanup (Binaries/Intermediate), single MCP server, apply gate
