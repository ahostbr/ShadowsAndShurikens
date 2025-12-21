# Buddy Worklog
- goal: allow MCP image tools to read any asset under DevTools/python/SOTS_Capture while keeping VisualDigest defaults intact
- what changed: expanded image sandbox root to DevTools/python/SOTS_Capture and kept default digest path; updated tool description to reflect broader scope
- files changed: DevTools/python/sots_mcp_server/server.py
- notes + risks/unknowns: no runtime verification performed; assumes callers still request paths within SOTS_Capture
- verification status: UNVERIFIED (not run)
- follow-ups / next steps: confirm tools can read non-VisualDigest files under SOTS_Capture; adjust documentation if further path tweaks are needed
