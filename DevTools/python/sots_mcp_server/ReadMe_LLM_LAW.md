[PATH ROOT RULES]

RepoRoot (canonical project root):
- E:\SAS\ShadowsAndShurikens

Hard rule:
- NEVER treat E:\SAS\ShadowsAndShurikens\DevTools\python\sots_mcp_server as a “root”, “project_root”, or “repo root”.
- It is a subfolder/component ONLY.

Path expectations (everything must resolve under RepoRoot):
- DevTools live under:  %RepoRoot%\DevTools\python\
- Plugins live under:   %RepoRoot%\Plugins\
- Docs live under:      %RepoRoot%\Docs\  (and/or %RepoRoot%\Plugins\<Plugin>\Docs\)

When a script/tool asks for “project_root” / “repo root” / “workspace root”:
- Always pass: E:\SAS\ShadowsAndShurikens

Validation (do this mentally / in code if you’re writing helpers):
- Normalize paths and confirm they start with RepoRoot.
- If someone tries to use the MCP server folder as root, treat it as an error and correct it to RepoRoot.
[/PATH ROOT RULES]