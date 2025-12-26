from __future__ import annotations

import argparse
import datetime as _dt
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable


def _ts() -> str:
    return _dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def _sanitize_tag(value: str, *, max_len: int = 48) -> str:
    # Restic tags should be simple tokens. Keep to a conservative charset.
    cleaned = []
    for ch in value.strip():
        if ch.isalnum() or ch in "._-":
            cleaned.append(ch)
        else:
            cleaned.append("_")
    out = "".join(cleaned).strip("_.-")
    if not out:
        return "x"
    return out[:max_len]


class BackupLogger:
    def __init__(self, log_path: Path) -> None:
        self.log_path = log_path
        self._fh = log_path.open("w", encoding="utf-8", newline="\n")

    def close(self) -> None:
        try:
            self._fh.close()
        except Exception:
            pass

    def line(self, text: str) -> None:
        line = text.rstrip("\n")
        print(line)
        self._fh.write(line + "\n")
        self._fh.flush()


def load_config(config_path: Path) -> dict:
    if not config_path.is_file():
        raise FileNotFoundError(f"Config file not found: {config_path}")
    return json.loads(config_path.read_text(encoding="utf-8"))


def default_config_path(script_file: Path) -> Path:
    return script_file.resolve().parent / "sots_backup_config.json"


def resolve_project_root(config: dict, src_arg: str | None) -> Path:
    if src_arg:
        return Path(src_arg).expanduser().resolve()

    env_override = os.environ.get("SOTS_PROJECT_ROOT", "").strip()
    if env_override:
        return Path(env_override).expanduser().resolve()

    return Path(config["project_root_default"]).expanduser().resolve()


def _guard_project_root(src: Path) -> None:
    src_norm = str(src).lower().replace("/", "\\")
    banned = "\\devtools\\python\\sots_mcp_server"
    if banned in src_norm:
        raise RuntimeError(
            "Refusing to use DevTools\\python\\sots_mcp_server as a project root. "
            "Set --src or SOTS_PROJECT_ROOT to the repo root (e.g. E:\\SAS\\ShadowsAndShurikens)."
        )


def ensure_dirs(config: dict, logger: BackupLogger) -> None:
    backup_root = Path(config["backup_root"]).expanduser()
    restic_repo = Path(config["restic_repo"]).expanduser()
    log_dir = Path(config["log_dir"]).expanduser()

    for p in [backup_root, restic_repo, log_dir]:
        if not p.exists():
            logger.line(f"[dirs] Creating: {p}")
            p.mkdir(parents=True, exist_ok=True)


def _find_restic_exe(logger: BackupLogger) -> str | None:
    found = shutil.which("restic")
    if found:
        return "restic"

    # Portable install fallback: allow placing restic.exe under the default backup root.
    # This mirrors the GUI fallback and avoids requiring winget on some machines.
    for candidate in [
        Path("C:/UE5/SOTS_Backup/restic.exe"),
        Path("C:/UE5/SOTS_Backup/bin/restic.exe"),
        Path("C:/UE5/SOTS_Backup/restic/restic.exe"),
        Path("C:/UE5/SOTS_Backup/tools/restic.exe"),
    ]:
        try:
            if candidate.is_file():
                logger.line(f"[restic] Using portable exe: {candidate}")
                return str(candidate)
        except Exception:
            continue

    local_app_data = os.environ.get("LOCALAPPDATA", "").strip()
    if not local_app_data:
        return None

    pkg_root = Path(local_app_data) / "Microsoft" / "WinGet" / "Packages"
    if not pkg_root.is_dir():
        return None

    candidates: list[Path] = []
    for pkg in pkg_root.glob("restic.restic_*"):
        try:
            # winget portable often drops restic_*.exe right in the package folder
            candidates.extend(list(pkg.glob("restic*.exe")))
        except Exception:
            continue

    if not candidates:
        return None

    best = max(candidates, key=lambda p: p.stat().st_mtime)
    logger.line(f"[restic] Using winget-installed exe: {best}")
    return str(best)


def restic_exists(logger: BackupLogger) -> bool:
    restic = _find_restic_exe(logger)
    if not restic:
        return False

    try:
        rc = run_cmd([restic, "version"], logger=logger, cwd=None)
        return rc == 0
    except FileNotFoundError:
        return False


def check_passfile(passfile: Path, logger: BackupLogger) -> None:
    if not passfile.is_file():
        logger.line(f"[error] Restic passfile missing: {passfile}")
        logger.line("[error] Create it once with a long random password (see Docs/SOTS_Backup_Restic.md).")
        raise FileNotFoundError(str(passfile))


def is_repo_initialized(restic_repo: Path) -> bool:
    return (restic_repo / "config").is_file()


def run_cmd(
    args: list[str],
    *,
    logger: BackupLogger,
    cwd: Path | None,
    env: dict[str, str] | None = None,
) -> int:
    cmd_str = " ".join([_quote_arg(a) for a in args])
    logger.line(f"[cmd] {cmd_str}")

    proc = subprocess.Popen(
        args,
        cwd=str(cwd) if cwd else None,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    assert proc.stdout is not None
    for line in proc.stdout:
        logger.line(line.rstrip("\n"))

    returncode = proc.wait()
    logger.line(f"[cmd] exit={returncode}")
    return returncode


def run_cmd_capture(
    args: list[str],
    *,
    logger: BackupLogger,
    cwd: Path | None,
    env: dict[str, str] | None = None,
) -> tuple[int, str]:
    cmd_str = " ".join([_quote_arg(a) for a in args])
    logger.line(f"[cmd] {cmd_str}")

    proc = subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    out = proc.stdout or ""
    for line in out.splitlines():
        logger.line(line)
    logger.line(f"[cmd] exit={proc.returncode}")
    return proc.returncode, out


def _git_exists(src: Path) -> bool:
    return (src / ".git").exists() and shutil.which("git") is not None


def _git_branch(src: Path, logger: BackupLogger) -> str | None:
    if not _git_exists(src):
        return None
    try:
        proc = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=str(src),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if proc.returncode != 0:
            return None
        b = (proc.stdout or "").strip()
        return b or None
    except Exception as e:
        logger.line(f"[git] branch lookup failed: {e}")
        return None


def _git_dirty_counts(src: Path, logger: BackupLogger) -> tuple[int, int] | None:
    if not _git_exists(src):
        return None
    try:
        proc = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=str(src),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if proc.returncode != 0:
            return None
        lines = [ln for ln in (proc.stdout or "").splitlines() if ln.strip()]
        staged = 0
        unstaged = 0
        for ln in lines:
            # porcelain format: XY <path>
            if len(ln) < 2:
                continue
            x = ln[0]
            y = ln[1]
            if x != " ":
                staged += 1
            if y != " ":
                unstaged += 1
        return staged, unstaged
    except Exception as e:
        logger.line(f"[git] status lookup failed: {e}")
        return None


def _build_auto_tags(src: Path, logger: BackupLogger, note: str | None) -> list[str]:
    tags: list[str] = []
    tags.append(_sanitize_tag(f"ts_{_ts()}"))

    if note:
        tags.append(_sanitize_tag(f"note_{note}"))

    branch = _git_branch(src, logger)
    if branch:
        tags.append(_sanitize_tag(f"git_{branch}"))

    dirty = _git_dirty_counts(src, logger)
    if dirty:
        staged, unstaged = dirty
        tags.append(_sanitize_tag(f"dirty_s{staged}_u{unstaged}"))

    return tags


def _quote_arg(a: str) -> str:
    if not a:
        return '""'
    if any(ch.isspace() for ch in a) or '"' in a:
        return '"' + a.replace('"', '\\"') + '"'
    return a


def build_exclude_args(src: Path, config: dict, logger: BackupLogger) -> list[str]:
    excludes_cfg = config.get("excludes", {})

    regen_enabled = bool(excludes_cfg.get("regen_excludes_enabled", True))
    optional_enabled = bool(excludes_cfg.get("optional_excludes_enabled", True))

    args: list[str] = []

    def add_exclude_path(p: Path) -> None:
        # Only add if it exists to keep output clean.
        if p.exists():
            args.extend(["--exclude", str(p)])
            logger.line(f"[exclude] {p}")

    if regen_enabled:
        for rel in excludes_cfg.get("regen", []):
            add_exclude_path(src / rel)

    if optional_enabled:
        for rel in excludes_cfg.get("optional", []):
            add_exclude_path(src / rel)

        plugins_dir = src / "Plugins"
        plugin_opt = excludes_cfg.get("plugin_optional", {})
        exclude_plugins_intermediate = bool(plugin_opt.get("exclude_plugins_intermediate", True))
        exclude_plugins_binaries = bool(plugin_opt.get("exclude_plugins_binaries", True))

        if plugins_dir.is_dir() and (exclude_plugins_intermediate or exclude_plugins_binaries):
            for child in plugins_dir.iterdir():
                if not child.is_dir():
                    continue
                if exclude_plugins_intermediate:
                    add_exclude_path(child / "Intermediate")
                if exclude_plugins_binaries:
                    add_exclude_path(child / "Binaries")

    return args


def _restic_base_args(config: dict, restic_exe: str) -> tuple[Path, Path, list[str]]:
    restic_repo = Path(config["restic_repo"]).expanduser()
    passfile = Path(config["passfile"]).expanduser()

    base = [restic_exe, "-r", str(restic_repo), "--password-file", str(passfile)]
    return restic_repo, passfile, base


def cmd_init(src: Path, config: dict, logger: BackupLogger) -> int:
    _guard_project_root(src)
    ensure_dirs(config, logger)

    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found on PATH. Install it and retry.")
        logger.line("[error] Quick install options: winget install restic.restic (or choco install restic)")
        return 1

    # sanity check
    try:
        rc = run_cmd([restic_exe, "version"], logger=logger, cwd=None)
        if rc != 0:
            return rc
    except FileNotFoundError:
        logger.line("[error] restic executable not runnable.")
        return 1

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    check_passfile(passfile, logger)

    if is_repo_initialized(restic_repo):
        logger.line(f"[init] Repo already initialized: {restic_repo}")
        return 0

    logger.line(f"[init] Initializing repo: {restic_repo}")
    rc = run_cmd(base + ["init"], logger=logger, cwd=None)
    return rc


def _backup(src: Path, config: dict, logger: BackupLogger, tag: str) -> int:
    rc = cmd_init(src, config, logger)
    if rc != 0:
        return rc

    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found after init.")
        return 1

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    _ = restic_repo
    _ = passfile

    exclude_args = build_exclude_args(src, config, logger)

    logger.line(f"[backup] Source: {src}")
    # Always include the required base tag, plus auto-tags.
    auto_tags = _build_auto_tags(src, logger, config.get("_note"))
    all_tags = [tag] + auto_tags
    logger.line(f"[backup] Tags: {all_tags}")

    tag_args: list[str] = []
    for t in all_tags:
        tag_args.extend(["--tag", _sanitize_tag(str(t))])

    args = base + ["backup", str(src)] + tag_args + exclude_args
    rc = run_cmd(args, logger=logger, cwd=None)
    return rc


def cmd_full(src: Path, config: dict, logger: BackupLogger) -> int:
    return _backup(src, config, logger, tag="full")


def cmd_push(src: Path, config: dict, logger: BackupLogger) -> int:
    rc = _backup(src, config, logger, tag="push")
    if rc != 0:
        return rc

    logger.line("[push] Printing last 3 snapshots:")
    snaps = list_snapshots(src, config, logger)
    if snaps:
        for s in snaps[-3:]:
            short_id = (s.get("short_id") or s.get("id") or "")[:8]
            logger.line(
                f"  {short_id}  {s.get('time','')}  tags={s.get('tags',[])}  paths={s.get('paths',[])}"
            )
    return 0


def list_snapshots(_src: Path, config: dict, logger: BackupLogger) -> list[dict]:
    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found.")
        return []

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    _ = restic_repo
    _ = passfile

    rc, out = run_cmd_capture(base + ["snapshots", "--json"], logger=logger, cwd=None)
    if rc != 0:
        return []

    try:
        data = json.loads(out)
    except Exception as e:
        logger.line(f"[error] Failed to parse snapshots JSON: {e}")
        return []

    def key(item: dict) -> str:
        return str(item.get("time", ""))

    data_sorted = sorted(data, key=key)
    return data_sorted


def cmd_snapshots(src: Path, config: dict, logger: BackupLogger) -> int:
    snaps = list_snapshots(src, config, logger)
    logger.line(f"[snapshots] count={len(snaps)}")
    return 0


def cmd_diff(src: Path, config: dict, logger: BackupLogger) -> int:
    snaps = list_snapshots(src, config, logger)
    if len(snaps) < 2:
        logger.line("[diff] Need at least 2 snapshots.")
        return 1

    a = snaps[-2].get("id")
    b = snaps[-1].get("id")
    if not a or not b:
        logger.line("[diff] Could not determine snapshot ids.")
        return 1

    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found.")
        return 1

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    _ = restic_repo
    _ = passfile

    logger.line(f"[diff] {a} -> {b}")
    rc = run_cmd(base + ["diff", a, b], logger=logger, cwd=None)
    return rc


def cmd_restore_latest(src: Path, config: dict, logger: BackupLogger, target: Path) -> int:
    snaps = list_snapshots(src, config, logger)
    if not snaps:
        logger.line("[restore] No snapshots found.")
        return 1

    latest_id = snaps[-1].get("id")
    if not latest_id:
        logger.line("[restore] Latest snapshot has no id.")
        return 1

    target = target.expanduser().resolve()
    if not target.exists():
        logger.line(f"[restore] Creating target: {target}")
        target.mkdir(parents=True, exist_ok=True)

    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found.")
        return 1

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    _ = restic_repo
    _ = passfile

    logger.line(f"[restore] Restoring snapshot {latest_id} -> {target}")
    rc = run_cmd(base + ["restore", latest_id, "--target", str(target)], logger=logger, cwd=None)
    return rc


def cmd_forget_prune(
    src: Path,
    config: dict,
    logger: BackupLogger,
    *,
    keep_last: int | None,
    keep_weekly: int | None,
    keep_monthly: int | None,
) -> int:
    _ = src
    restic_exe = _find_restic_exe(logger)
    if not restic_exe:
        logger.line("[error] restic not found.")
        return 1

    restic_repo, passfile, base = _restic_base_args(config, restic_exe)
    _ = restic_repo
    _ = passfile

    args = base + ["forget", "--prune"]
    if keep_last is not None:
        args += ["--keep-last", str(keep_last)]
    if keep_weekly is not None:
        args += ["--keep-weekly", str(keep_weekly)]
    if keep_monthly is not None:
        args += ["--keep-monthly", str(keep_monthly)]

    logger.line("[forget] Running retention policy")
    rc = run_cmd(args, logger=logger, cwd=None)
    return rc


def make_logger(config: dict, command_name: str) -> BackupLogger:
    log_dir = Path(config["log_dir"]).expanduser()
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"sots_backup_{command_name}_{_ts()}.log"
    return BackupLogger(log_path)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="SOTS local snapshot backup tooling (restic).",
    )
    parser.add_argument(
        "--config",
        default=str(default_config_path(Path(__file__))),
        help="Path to sots_backup_config.json",
    )
    parser.add_argument(
        "--src",
        default=None,
        help="Project root to back up (overrides SOTS_PROJECT_ROOT and config).",
    )

    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("init")
    full_p = sub.add_parser("full")
    full_p.add_argument("--note", default=None, help="Optional short note to tag the snapshot (commit-message-like).")

    push_p = sub.add_parser("push")
    push_p.add_argument("--note", default=None, help="Optional short note to tag the snapshot (commit-message-like).")

    sub.add_parser("snapshots")
    sub.add_parser("diff")

    restore_p = sub.add_parser("restore-latest")
    restore_p.add_argument("--target", required=True, help="Target directory to restore into")

    forget_p = sub.add_parser("forget-prune")
    forget_p.add_argument("--keep-last", type=int, default=None)
    forget_p.add_argument("--keep-weekly", type=int, default=None)
    forget_p.add_argument("--keep-monthly", type=int, default=None)

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    argv = argv if argv is not None else sys.argv[1:]
    args = parse_args(argv)

    config_path = Path(args.config).expanduser().resolve()
    config = load_config(config_path)

    src = resolve_project_root(config, args.src)

    logger = make_logger(config, args.command)
    try:
        logger.line(f"[sots_backup] command={args.command}")
        logger.line(f"[sots_backup] config={config_path}")
        logger.line(f"[sots_backup] src={src}")

        # Pass per-command note through config for minimal plumbing.
        if getattr(args, "note", None):
            config["_note"] = str(getattr(args, "note"))

        if args.command == "init":
            return cmd_init(src, config, logger)
        if args.command == "full":
            return cmd_full(src, config, logger)
        if args.command == "push":
            return cmd_push(src, config, logger)
        if args.command == "snapshots":
            return cmd_snapshots(src, config, logger)
        if args.command == "diff":
            return cmd_diff(src, config, logger)
        if args.command == "restore-latest":
            target = Path(args.target)
            return cmd_restore_latest(src, config, logger, target=target)
        if args.command == "forget-prune":
            return cmd_forget_prune(
                src,
                config,
                logger,
                keep_last=args.keep_last,
                keep_weekly=args.keep_weekly,
                keep_monthly=args.keep_monthly,
            )

        logger.line(f"[error] Unknown command: {args.command}")
        return 1
    except FileNotFoundError as e:
        logger.line(f"[error] {e}")
        return 1
    except Exception as e:
        logger.line(f"[error] {type(e).__name__}: {e}")
        return 1
    finally:
        logger.close()


if __name__ == "__main__":
    raise SystemExit(main())
