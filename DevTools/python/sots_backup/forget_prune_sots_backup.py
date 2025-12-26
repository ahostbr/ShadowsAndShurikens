from __future__ import annotations

import argparse
import sys

from sots_backup import main


def parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Apply a retention policy (restic forget --prune).")
    p.add_argument("--keep-last", type=int, default=None)
    p.add_argument("--keep-weekly", type=int, default=None)
    p.add_argument("--keep-monthly", type=int, default=None)
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

    if args.keep_last is not None:
        passthrough += ["forget-prune", "--keep-last", str(args.keep_last)]
    else:
        passthrough += ["forget-prune"]

    if args.keep_weekly is not None:
        passthrough += ["--keep-weekly", str(args.keep_weekly)]
    if args.keep_monthly is not None:
        passthrough += ["--keep-monthly", str(args.keep_monthly)]

    raise SystemExit(main(passthrough))
