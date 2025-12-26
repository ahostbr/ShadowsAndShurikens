from __future__ import annotations

import argparse
import sys

from sots_backup import main


def parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Run a full baseline SOTS restic snapshot (tag=full).")
    p.add_argument("--config", default=None, help="Override config path (optional)")
    p.add_argument("--src", default=None, help="Override project root (optional)")
    p.add_argument("--note", default=None, help="Optional short note to tag the snapshot")
    return p.parse_args(argv)


if __name__ == "__main__":
    args = parse_args(sys.argv[1:])
    passthrough: list[str] = []
    if args.config:
        passthrough += ["--config", args.config]
    if args.src:
        passthrough += ["--src", args.src]
    passthrough += ["full"]
    if args.note:
        passthrough += ["--note", args.note]
    raise SystemExit(main(passthrough))
