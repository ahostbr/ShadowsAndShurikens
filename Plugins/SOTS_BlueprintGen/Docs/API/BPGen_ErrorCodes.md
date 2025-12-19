# BPGen Error Codes (SPINE_P)

Bridge and API return `error` (string) plus optional `errors` array and `error_code` for machine handling. Stable codes:

| Code | When | Notes |
| --- | --- | --- |
| `ERR_PROTOCOL_MISMATCH` | Client protocol major differs from server. | Upgrade client or server; protocol_version currently 1.0. |
| `ERR_SAFE_MODE_ACTIVE` | Dangerous op attempted while safe_mode=true. | Supply `dangerous_ok=true` or disable safe mode (not recommended). |
| `ERR_DANGEROUS_OP_REQUIRES_OPT_IN` | Delete/replace attempted without `dangerous_ok`. | Add `dangerous_ok=true` in params. |
| `ERR_INVALID_PARAMS` | Missing/invalid fields (e.g., canonicalize_spec missing graph_spec). | Validate request payload. |
| `ERR_RATE_LIMIT` | Bridge rate limits exceeded. | Lower request rate or raise limits in config. |
| `ERR_AUTH_REQUIRED` | Auth token required but missing/invalid. | Provide matching token via params.auth_token. |
| `ERR_TOO_LARGE` | Request exceeds MaxRequestBytes. | Reduce payload size. |

Non-coded errors still appear in `error`/`errors` (e.g., `APPLY blocked: set SOTS_ALLOW_APPLY=1`). Treat any `ok=false` as failure even if code is empty.
