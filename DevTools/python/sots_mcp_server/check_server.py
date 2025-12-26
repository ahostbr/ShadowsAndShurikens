
import os
import psutil


def _norm(s: str) -> str:
    return os.path.normcase(os.path.normpath(s))


# VS Code launches the MCP server via stdio as:
#   <python.exe> <...>\DevTools\python\sots_mcp_server\server.py
SERVER_SCRIPT_BASENAME = "server.py"
SERVER_PARENT_DIRNAME = "sots_mcp_server"


this_pid = os.getpid()
found = False

for proc in psutil.process_iter(["pid", "cmdline", "exe"]):
    try:
        if proc.info.get("pid") == this_pid:
            continue

        cmdline = proc.info.get("cmdline") or []
        if not cmdline:
            continue

        # Heuristic: any python process whose cmdline includes
        # ...\sots_mcp_server\server.py
        joined = " ".join(cmdline)
        if SERVER_SCRIPT_BASENAME not in joined:
            continue
        if SERVER_PARENT_DIRNAME not in joined:
            continue

        exe = proc.info.get("exe") or ""
        if "python.exe" not in os.path.basename(_norm(exe)):
            continue

        print(f"ONLINE: Found MCP server process (pid={proc.info['pid']}): {joined}")
        found = True
        break
    except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        continue

if not found:
    print("OFFLINE: The MCP server process (server.py) was not found.")

