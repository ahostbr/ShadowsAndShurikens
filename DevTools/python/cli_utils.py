from __future__ import annotations


def confirm_start(tool_name: str) -> None:
    print(f"=== {tool_name} ===")
    input("Press Enter to start...")


def confirm_exit() -> None:
    input("Press Enter to exit...")
