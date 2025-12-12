from __future__ import annotations
import argparse
from pathlib import Path
import zipfile

from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def list_zip(zip_path: Path):
    with zipfile.ZipFile(zip_path, "r") as zf:
        return sorted(zf.namelist())


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare contents of two plugin zip files.")
    parser.add_argument("zip_a", type=str, help="First zip path (A).")
    parser.add_argument("zip_b", type=str, help="Second zip path (B).")
    args = parser.parse_args()

    confirm_start("compare_plugin_zips")

    zip_a = Path(args.zip_a).resolve()
    zip_b = Path(args.zip_b).resolve()

    if not zip_a.exists() or not zip_b.exists():
        print("[ERROR] One or both zip files do not exist.")
        print_llm_summary(
            "compare_plugin_zips",
            ZIP_A=str(zip_a),
            ZIP_B=str(zip_b),
            ERROR="ZIP_NOT_FOUND",
        )
        confirm_exit()
        return

    files_a = list_zip(zip_a)
    files_b = list_zip(zip_b)

    only_a = sorted(set(files_a) - set(files_b))
    only_b = sorted(set(files_b) - set(files_a))

    print("[ONLY IN A]")
    for f in only_a[:50]:
        print("  ", f)
    if len(only_a) > 50:
        print(f"  ... ({len(only_a) - 50} more)")

    print("[ONLY IN B]")
    for f in only_b[:50]:
        print("  ", f)
    if len(only_b) > 50:
        print(f"  ... ({len(only_b) - 50} more)")

    print_llm_summary(
        "compare_plugin_zips",
        ZIP_A=str(zip_a),
        ZIP_B=str(zip_b),
        ONLY_A=len(only_a),
        ONLY_B=len(only_b),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
