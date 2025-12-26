from __future__ import annotations

import argparse
import sys

from sots_backup import main


def parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Restore the latest SOTS restic snapshot to a target folder.")
    p.add_argument("--target", required=True, help="Target directory (will be created if missing)")
    p.add_argument("--src", default=None, help="Override project root (optional)")
    p.add_argument("--config", default=None, help="Override config path (optional)")
    return p.parse_args(argv)


if __name__ == "__main__":
    args = parse_args(sys.argv[1:])

    passthrough: list[str] = []
    if args.config:
        passthrough += ["--config", args.config]
    if args.src:
        passthrough += ["--src", args.src]

    passthrough += ["restore-latest", "--target", args.target]

    raise SystemExit(main(passthrough))
