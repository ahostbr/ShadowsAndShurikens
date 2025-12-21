#!/usr/bin/env python3
"""
sots_http_fetch.py

A small, dependency-free HTTP fetch/post helper Buddy can use from the repo.

HARD SAFETY GATE:
- This script ALWAYS requires an explicit Enter press in the terminal before it will do anything network-related.
- Confirmation is read from the real terminal device (CONIN$ on Windows, /dev/tty on Unix) so piping stdin
  (e.g. echo ... | script) will NOT accidentally “confirm” or consume payload input.

Features:
- Supports GET/POST/PUT/PATCH/DELETE
- Optional JSON body + pretty-print JSON responses
- Writes a timestamped log file every run (never silent)
- Includes a convenience mode for posting to the local Send2SOTS bridge

Examples:
  # Simple GET
  python sots_http_fetch.py fetch https://example.com --pretty

  # POST JSON
  python sots_http_fetch.py fetch https://httpbin.org/post --method POST --json '{"hello":"world"}' --pretty

  # POST raw data from a file
  python sots_http_fetch.py fetch https://httpbin.org/post --method POST --data @payload.txt

  # Send2SOTS convenience (reads stdin as content; still requires Enter confirmation from terminal)
  echo "hello from buddy" | python sots_http_fetch.py send2sots --kind send2sots_cli
"""

from __future__ import annotations

import argparse
import json
import os
import ssl
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from typing import Dict, Optional, Tuple
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib.request import Request, urlopen


SCRIPT_VERSION = "1.0.1"


@dataclass
class FetchResult:
    ok: bool
    status: int
    reason: str
    headers: Dict[str, str]
    body_text: str
    body_bytes: bytes
    elapsed_ms: int
    error: Optional[str] = None


def _ts() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def _default_log_path() -> str:
    return os.path.join(os.getcwd(), f"http_fetch_{_ts()}.log")


def _write_log(path: str, text: str) -> None:
    try:
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
    except Exception as e:
        print(f"[http_fetch] WARN: failed to write log: {path} :: {e}", file=sys.stderr)


def _parse_kv_headers(items) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for raw in items or []:
        if ":" not in raw:
            raise ValueError(f"Invalid header '{raw}'. Use 'Key: Value'.")
        k, v = raw.split(":", 1)
        out[k.strip()] = v.strip()
    return out


def _read_data_arg(data_arg: Optional[str]) -> Optional[bytes]:
    if not data_arg:
        return None
    if data_arg.startswith("@"):
        path = data_arg[1:]
        with open(path, "rb") as f:
            return f.read()
    return data_arg.encode("utf-8")


def _read_stdin_if_requested(use_stdin: bool) -> Optional[bytes]:
    if not use_stdin:
        return None
    data = sys.stdin.buffer.read()
    return data if data else None


def _decode_body(body: bytes, content_type: str) -> str:
    charset = "utf-8"
    if content_type:
        parts = content_type.split(";")
        for p in parts[1:]:
            p = p.strip()
            if p.lower().startswith("charset="):
                charset = p.split("=", 1)[1].strip() or "utf-8"
                break
    try:
        return body.decode(charset, errors="replace")
    except Exception:
        return body.decode("utf-8", errors="replace")


def _pretty_json_if_possible(text: str) -> Tuple[bool, str]:
    try:
        obj = json.loads(text)
        return True, json.dumps(obj, indent=2, ensure_ascii=False, sort_keys=False)
    except Exception:
        return False, text


def _enforce_allowlist(url: str, allow_hosts: Optional[list]) -> None:
    if not allow_hosts:
        return
    host = urlparse(url).hostname or ""
    if host not in allow_hosts:
        raise ValueError(f"Blocked by allowlist. Host '{host}' not in --allow-host list: {allow_hosts}")


def _open_terminal_in_stream():
    """
    Return a readable stream that is *the terminal*, not stdin.
    - Windows: CONIN$
    - Unix: /dev/tty
    """
    try:
        if os.name == "nt":
            return open("CONIN$", "r", encoding="utf-8", errors="replace")
        return open("/dev/tty", "r", encoding="utf-8", errors="replace")
    except Exception:
        return None


def _open_terminal_out_stream():
    """
    Return a writable stream that is *the terminal*, not stdout.
    - Windows: CONOUT$
    - Unix: /dev/tty (write)
    """
    try:
        if os.name == "nt":
            return open("CONOUT$", "w", encoding="utf-8", errors="replace")
        return open("/dev/tty", "w", encoding="utf-8", errors="replace")
    except Exception:
        return None


def require_enter_confirmation(command_preview: str) -> None:
    """
    Hard gate: the script cannot proceed without an explicit Enter press in the terminal.

    Important: reads from terminal device so piped stdin does not auto-confirm and does not
    consume payload input.
    """
    term_out = _open_terminal_out_stream()
    out = term_out if term_out is not None else sys.stderr

    print("", file=out)
    print("============================================================", file=out)
    print("SOTS HTTP FETCH — CONFIRMATION REQUIRED", file=out)
    print("============================================================", file=out)
    print("About to run:", file=out)
    print(f"  {command_preview}", file=out)
    print("", file=out)
    print("Press ENTER to continue, or Ctrl+C to abort.", file=out)
    print("============================================================", file=out)
    out.flush()

    term_in = _open_terminal_in_stream()
    try:
        if term_in is not None:
            _ = term_in.readline()  # Wait for Enter
        else:
            # Fallback (still blocks), but may conflict with piped stdin
            # We keep it as last resort if terminal device isn't available.
            _ = input()
    except EOFError:
        raise RuntimeError("No terminal input available for confirmation (EOF). Aborting.")
    finally:
        try:
            if term_in is not None:
                term_in.close()
        except Exception:
            pass
        try:
            if term_out is not None:
                term_out.close()
        except Exception:
            pass


def http_fetch(
    url: str,
    method: str = "GET",
    headers: Optional[Dict[str, str]] = None,
    body: Optional[bytes] = None,
    timeout_s: int = 20,
    insecure: bool = False,
    retries: int = 0,
    retry_delay_ms: int = 400,
) -> FetchResult:
    method = method.upper().strip()
    headers = dict(headers or {})

    ctx = None
    if url.lower().startswith("https://"):
        ctx = ssl._create_unverified_context() if insecure else ssl.create_default_context()

    last_err = None

    for attempt in range(retries + 1):
        t0 = time.perf_counter()
        try:
            req = Request(url=url, data=body, method=method)
            for k, v in headers.items():
                req.add_header(k, v)

            with urlopen(req, timeout=timeout_s, context=ctx) as resp:
                status = getattr(resp, "status", 0) or 0
                reason = getattr(resp, "reason", "") or ""
                resp_headers = {k: v for (k, v) in resp.headers.items()}
                raw = resp.read() or b""
                content_type = resp_headers.get("Content-Type", "")
                text = _decode_body(raw, content_type)
                elapsed_ms = int((time.perf_counter() - t0) * 1000)
                return FetchResult(
                    ok=True,
                    status=status,
                    reason=reason,
                    headers=resp_headers,
                    body_text=text,
                    body_bytes=raw,
                    elapsed_ms=elapsed_ms,
                )

        except HTTPError as e:
            try:
                raw = e.read() or b""
            except Exception:
                raw = b""
            try:
                content_type = e.headers.get("Content-Type", "") if e.headers else ""
            except Exception:
                content_type = ""
            text = _decode_body(raw, content_type)
            elapsed_ms = int((time.perf_counter() - t0) * 1000)
            last_err = f"HTTPError {getattr(e, 'code', '??')}: {getattr(e, 'reason', '')}".strip()
            if attempt >= retries:
                return FetchResult(
                    ok=False,
                    status=int(getattr(e, "code", 0) or 0),
                    reason=str(getattr(e, "reason", "")),
                    headers={k: v for (k, v) in (e.headers.items() if getattr(e, "headers", None) else [])},
                    body_text=text,
                    body_bytes=raw,
                    elapsed_ms=elapsed_ms,
                    error=last_err,
                )

        except URLError as e:
            elapsed_ms = int((time.perf_counter() - t0) * 1000)
            last_err = f"URLError: {e}"
            if attempt >= retries:
                return FetchResult(
                    ok=False,
                    status=0,
                    reason="URLError",
                    headers={},
                    body_text="",
                    body_bytes=b"",
                    elapsed_ms=elapsed_ms,
                    error=last_err,
                )

        except Exception as e:
            elapsed_ms = int((time.perf_counter() - t0) * 1000)
            last_err = f"Exception: {e}"
            if attempt >= retries:
                return FetchResult(
                    ok=False,
                    status=0,
                    reason="Exception",
                    headers={},
                    body_text="",
                    body_bytes=b"",
                    elapsed_ms=elapsed_ms,
                    error=last_err,
                )

        time.sleep(max(0.0, retry_delay_ms / 1000.0))

    return FetchResult(False, 0, "Unknown", {}, "", b"", 0, error=last_err or "Unknown failure")


def cmd_fetch(args) -> int:
    _enforce_allowlist(args.url, args.allow_host)

    headers = _parse_kv_headers(args.header)
    body = None

    stdin_data = _read_stdin_if_requested(args.stdin)
    file_or_inline = _read_data_arg(args.data)

    if args.json is not None:
        body = args.json.encode("utf-8")
        headers.setdefault("Content-Type", "application/json; charset=utf-8")
    elif stdin_data is not None:
        body = stdin_data
    elif file_or_inline is not None:
        body = file_or_inline

    if body is not None:
        headers.setdefault("Content-Length", str(len(body)))

    res = http_fetch(
        url=args.url,
        method=args.method,
        headers=headers,
        body=body,
        timeout_s=args.timeout,
        insecure=args.insecure,
        retries=args.retries,
        retry_delay_ms=args.retry_delay_ms,
    )

    body_text = res.body_text
    pretty_applied = False
    if args.pretty:
        okj, pretty = _pretty_json_if_possible(body_text)
        if okj:
            body_text = pretty
            pretty_applied = True

    max_bytes = args.max_print_bytes
    display_text = body_text
    if max_bytes > 0 and len(res.body_bytes) > max_bytes:
        display_text = _decode_body(res.body_bytes[:max_bytes], res.headers.get("Content-Type", "")) + "\n...[truncated]..."

    if args.out:
        try:
            with open(args.out, "wb") as f:
                f.write(res.body_bytes)
        except Exception as e:
            print(f"[http_fetch] ERROR: failed writing --out '{args.out}': {e}", file=sys.stderr)

    log_path = args.log or _default_log_path()
    log_lines = []
    log_lines.append(f"[http_fetch] version={SCRIPT_VERSION}")
    log_lines.append(f"[http_fetch] ts={datetime.now().isoformat()}")
    log_lines.append(f"[http_fetch] request: {args.method} {args.url}")
    log_lines.append(f"[http_fetch] timeout_s={args.timeout} retries={args.retries} insecure={args.insecure}")
    if headers:
        log_lines.append("[http_fetch] request_headers:")
        for k, v in headers.items():
            log_lines.append(f"  {k}: {v}")
    if body is not None:
        log_lines.append(f"[http_fetch] request_body_bytes={len(body)}")
    log_lines.append(f"[http_fetch] response_ok={res.ok} status={res.status} reason={res.reason} elapsed_ms={res.elapsed_ms}")
    if res.headers:
        log_lines.append("[http_fetch] response_headers:")
        for k, v in res.headers.items():
            log_lines.append(f"  {k}: {v}")
    if res.error:
        log_lines.append(f"[http_fetch] error={res.error}")
    log_lines.append(f"[http_fetch] response_body_bytes={len(res.body_bytes)}")
    log_lines.append("[http_fetch] response_body_text:")
    log_lines.append(body_text)
    _write_log(log_path, "\n".join(log_lines) + "\n")

    print(f"[http_fetch] {args.method} {args.url}")
    print(f"[http_fetch] ok={res.ok} status={res.status} reason={res.reason} elapsed_ms={res.elapsed_ms}")
    if res.error:
        print(f"[http_fetch] error={res.error}")
    if args.out:
        print(f"[http_fetch] wrote body -> {args.out} ({len(res.body_bytes)} bytes)")
    else:
        print("[http_fetch] body:")
        print(display_text)
        if pretty_applied:
            print("[http_fetch] (pretty JSON applied)")
        if max_bytes > 0 and len(res.body_bytes) > max_bytes:
            print(f"[http_fetch] (printed first {max_bytes} bytes; full body in log)")
    print(f"[http_fetch] log -> {log_path}")

    return 0 if res.ok else 2


def cmd_send2sots(args) -> int:
    url = args.url
    _enforce_allowlist(url, args.allow_host)

    content_bytes = _read_stdin_if_requested(True)
    if not content_bytes:
        print("[send2sots] ERROR: no stdin content. Pipe text into this command.", file=sys.stderr)
        return 2

    payload = {
        "kind": args.kind,
        "ts": datetime.now().isoformat(),
        "url": args.source_url or "",
        "content": content_bytes.decode("utf-8", errors="replace"),
    }
    body = json.dumps(payload, indent=2, ensure_ascii=False).encode("utf-8")
    headers = {"Content-Type": "application/json; charset=utf-8"}

    res = http_fetch(
        url=url,
        method="POST",
        headers=headers,
        body=body,
        timeout_s=args.timeout,
        insecure=args.insecure,
        retries=args.retries,
        retry_delay_ms=args.retry_delay_ms,
    )

    log_path = args.log or _default_log_path()
    log_text = (
        f"[send2sots] version={SCRIPT_VERSION}\n"
        f"[send2sots] ts={datetime.now().isoformat()}\n"
        f"[send2sots] url={url}\n"
        f"[send2sots] kind={args.kind}\n"
        f"[send2sots] ok={res.ok} status={res.status} reason={res.reason} elapsed_ms={res.elapsed_ms}\n"
        f"[send2sots] response_body_bytes={len(res.body_bytes)}\n"
        f"[send2sots] response_body_text:\n{res.body_text}\n"
    )
    _write_log(log_path, log_text)

    print(f"[send2sots] POST {url}")
    print(f"[send2sots] ok={res.ok} status={res.status} reason={res.reason} elapsed_ms={res.elapsed_ms}")
    if res.error:
        print(f"[send2sots] error={res.error}")
    print(f"[send2sots] log -> {log_path}")

    return 0 if res.ok else 2


def build_argparser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="sots_http_fetch.py", description="HTTP fetch/post helper (stdlib-only).")
    p.add_argument("--version", action="version", version=f"%(prog)s {SCRIPT_VERSION}")

    sub = p.add_subparsers(dest="cmd", required=True)

    f = sub.add_parser("fetch", help="Fetch a URL with optional body/headers.")
    f.add_argument("url", help="Target URL (http/https).")
    f.add_argument("--method", default="GET", help="HTTP method (GET/POST/PUT/PATCH/DELETE). Default: GET")
    f.add_argument("--header", action="append", default=[], help="Header in 'Key: Value' form. Can repeat.")
    f.add_argument("--data", default=None, help="Raw body string OR '@path/to/file' to read bytes from file.")
    f.add_argument("--json", default=None, help="JSON string body (sets Content-Type).")
    f.add_argument("--stdin", action="store_true", help="Read request body from stdin (bytes).")
    f.add_argument("--timeout", type=int, default=20, help="Timeout seconds. Default: 20")
    f.add_argument("--retries", type=int, default=0, help="Retry count on failure. Default: 0")
    f.add_argument("--retry-delay-ms", type=int, default=400, help="Delay between retries in ms. Default: 400")
    f.add_argument("--pretty", action="store_true", help="Pretty-print JSON response if possible.")
    f.add_argument("--max-print-bytes", type=int, default=200_000, help="Max response bytes to print. 0=print all. Default: 200000")
    f.add_argument("--out", default=None, help="Write raw response body to this file.")
    f.add_argument("--log", default=None, help="Write log to this file (default: timestamped file in CWD).")
    f.add_argument("--allow-host", action="append", default=None, help="If set, ONLY allow these hosts. Can repeat.")
    f.add_argument("--insecure", action="store_true", help="Disable TLS verification (HTTPS only).")

    s = sub.add_parser("send2sots", help="Post stdin content to the local Send2SOTS bridge.")
    s.add_argument("--url", default="http://127.0.0.1:5050/send2sots", help="Bridge endpoint URL.")
    s.add_argument("--kind", default="send2sots_cli", help="Payload kind string.")
    s.add_argument("--source-url", default="", help="Optional source URL to include in payload.")
    s.add_argument("--timeout", type=int, default=20, help="Timeout seconds. Default: 20")
    s.add_argument("--retries", type=int, default=0, help="Retry count on failure. Default: 0")
    s.add_argument("--retry-delay-ms", type=int, default=400, help="Delay between retries in ms. Default: 400")
    s.add_argument("--log", default=None, help="Write log to this file (default: timestamped file in CWD).")
    s.add_argument("--allow-host", action="append", default=None, help="If set, ONLY allow these hosts. Can repeat.")
    s.add_argument("--insecure", action="store_true", help="Disable TLS verification (HTTPS only).")

    return p


def main() -> int:
    ap = build_argparser()
    args = ap.parse_args()

    # HARD gate: must confirm every run (and must be a real terminal Enter press).
    cmd_preview = " ".join([os.path.basename(sys.argv[0])] + sys.argv[1:])
    require_enter_confirmation(cmd_preview)

    try:
        if args.cmd == "fetch":
            return cmd_fetch(args)
        if args.cmd == "send2sots":
            return cmd_send2sots(args)
        print(f"[http_fetch] ERROR: unknown command: {args.cmd}", file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        print("[http_fetch] interrupted.")
        return 130
    except Exception as e:
        log_path = getattr(args, "log", None) or _default_log_path()
        _write_log(log_path, f"[http_fetch] FATAL: {e}\n")
        print(f"[http_fetch] FATAL: {e}", file=sys.stderr)
        print(f"[http_fetch] log -> {log_path}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
