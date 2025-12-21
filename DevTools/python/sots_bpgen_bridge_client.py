"""Lightweight client for the BPGen TCP NDJSON bridge (SPINE_F contract).

Responsibilities:
- open a short-lived TCP connection to the UE bridge
- send a single JSON line: {"tool":"bpgen","action":...,"request_id":...,"params":{...}}
- read one JSON line response, decode, and return a dict
- enforce small timeouts and max-bytes guard to keep tooling safe
"""

from __future__ import annotations

import json
import os
import socket
import sys
import uuid
from typing import Any, Dict

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 55557
DEFAULT_CONNECT_TIMEOUT = 2.0  # seconds
DEFAULT_RECV_TIMEOUT = 30.0    # seconds
DEFAULT_MAX_BYTES = 2 * 1024 * 1024  # 2MB safety cap


def _build_request(action: str, params: Dict[str, Any] | None, request_id: str | None) -> Dict[str, Any]:
    rid = request_id or str(uuid.uuid4())
    return {
        "tool": "bpgen",
        "action": action,
        "request_id": rid,
        "params": params or {},
    }


def _default_error(action: str, request_id: str, message: str) -> Dict[str, Any]:
    return {
        "ok": False,
        "action": action,
        "request_id": request_id,
        "result": {},
        "errors": [message],
        "warnings": [],
    }


def bpgen_call(
    action: str,
    params: Dict[str, Any] | None = None,
    *,
    host: str = DEFAULT_HOST,
    port: int = DEFAULT_PORT,
    connect_timeout: float = DEFAULT_CONNECT_TIMEOUT,
    recv_timeout: float = DEFAULT_RECV_TIMEOUT,
    max_bytes: int = DEFAULT_MAX_BYTES,
    request_id: str | None = None,
) -> Dict[str, Any]:
    final_params = dict(params or {})
    auth_token = os.environ.get("SOTS_BPGEN_AUTH_TOKEN", "")
    if auth_token and "auth_token" not in final_params:
        final_params["auth_token"] = auth_token

    req = _build_request(action, final_params, request_id)
    request_id_final = req["request_id"]

    try:
        with socket.create_connection((host, port), timeout=connect_timeout) as sock:
            sock.settimeout(recv_timeout)
            payload = json.dumps(req) + "\n"
            sock.sendall(payload.encode("utf-8"))

            chunks: list[bytes] = []
            total = 0
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                chunks.append(chunk)
                total += len(chunk)
                if total > max_bytes:
                    return _default_error(action, request_id_final, "Response exceeded max_bytes guard")
            data = b"".join(chunks)
    except Exception as exc:  # socket or connection failure
        return _default_error(action, request_id_final, f"Bridge connection failed: {exc}")

    try:
        text = data.decode("utf-8", errors="replace")
        text_stripped = text.strip()
        try:
            return json.loads(text_stripped)
        except Exception:
            # Try progressive parsing to handle pretty-printed JSON arriving in chunks
            # without relying on newline boundaries.
            try:
                parsed = json.loads(text_stripped.split("\n", 1)[0])
                return parsed
            except Exception:
                return _default_error(action, request_id_final, f"Bridge response parse failed: {exc}")
    except Exception as exc:  # decode/parse failure
        return _default_error(action, request_id_final, f"Bridge response parse failed: {exc}")


def bpgen_ping(host: str = DEFAULT_HOST, port: int = DEFAULT_PORT) -> Dict[str, Any]:
    return bpgen_call("ping", {}, host=host, port=port)


if __name__ == "__main__":
    resp = bpgen_ping()
    json.dump(resp, sys.stdout, indent=2)
    sys.stdout.write("\n")
