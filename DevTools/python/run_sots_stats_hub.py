from __future__ import annotations

import subprocess
import sys
from pathlib import Path

PREFERRED_PY = Path(r"C:\Users\Ryan\AppData\Local\Programs\Python\Python312\python.exe")


def main() -> int:
    base_exe = PREFERRED_PY if PREFERRED_PY.exists() else Path(sys.executable)
    pythonw = base_exe.with_name(base_exe.stem + "w.exe")
    exe = pythonw if pythonw.exists() else base_exe
    cmd = [str(exe), "-m", "sots_stats_hub"]
    print(f"[INFO] Launching SOTS Stats Hub: {' '.join(cmd)}")
    flags = 0
    if sys.platform.startswith("win"):
        flags = 0x00000008 | 0x00000200  # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
    try:
        subprocess.Popen(cmd, cwd=str(Path(__file__).resolve().parent), creationflags=flags)
        return 0
    except Exception as exc:  # noqa: BLE001
        print(f"[ERROR] Could not launch SOTS Stats Hub: {exc}")
        print("Hint: ensure PySide6 is installed for the selected interpreter or adjust PREFERRED_PY.")
        input("Press Enter to close...")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
