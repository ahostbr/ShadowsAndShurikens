from __future__ import annotations

import argparse
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent
LOG_DIR = ROOT / "logs"
OUTBOX_DIR = ROOT / "patch_outbox"
LOG_DIR.mkdir(exist_ok=True)
OUTBOX_DIR.mkdir(exist_ok=True)


def now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="SOTS DevTools: save patch/file payload from ChatGPT prompt"
    )
    ap.add_argument("--mode", choices=["file", "patch"], required=True)
    ap.add_argument("--target", required=True, help="Target path (informational)")
    ap.add_argument("--label", default="chatgpt_block")
    ap.add_argument(
        "--source",
        required=True,
        help="Path to the full prompt .txt file (from inbox)",
    )
    args = ap.parse_args(argv)

    ts = now_tag()
    base_name = f"{ts}_{args.label}"

    src_path = Path(args.source).resolve()
    if not src_path.is_file():
        print(f"[PATCH_FROM_BLOCK] ERROR: source not found: {src_path}")
        return 1

    text = src_path.read_text(encoding="utf-8", errors="replace")

    if args.mode == "patch":
        out_path = OUTBOX_DIR / f"{base_name}.patch.txt"
    else:
        safe_target = str(args.target).replace("\\", "_").replace("/", "_")
        out_path = OUTBOX_DIR / f"{base_name}_{safe_target}.txt"

    out_path.write_text(text, encoding="utf-8")

    log_path = LOG_DIR / f"patch_from_block_{ts}.log"
    with log_path.open("w", encoding="utf-8") as log:
        print(
            f"[PATCH_FROM_BLOCK] mode={args.mode} target={args.target} "
            f"label={args.label} source={src_path}",
            file=log,
        )
        print(f"[PATCH_FROM_BLOCK] wrote: {out_path}", file=log)

    print(f"[PATCH_FROM_BLOCK] Done. Out: {out_path} Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
