
import psutil
import sys

# The presence of this string in the command line is a unique identifier for the server process.
SERVER_SCRIPT_NAME = "sots_mcp_server"
python_executable = sys.executable

found = False
for proc in psutil.process_iter(['cmdline', 'exe']):
    try:
        # Check if the process executable matches the one running this script
        # and if the server script name is in its command line arguments.
        if proc.info['exe'] and proc.info['cmdline'] and SERVER_SCRIPT_NAME in " ".join(proc.info['cmdline']):
            # To be more certain, let's check if it's a python process from the expected venv
            if "python.exe" in proc.info['exe']:
                print(f"ONLINE: Found server process running with command: {' '.join(proc.info['cmdline'])}")
                found = True
                break
    except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        pass

if not found:
    print("OFFLINE: The MCP server process was not found.")

