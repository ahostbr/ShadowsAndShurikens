# TL;DR (chosen plan)
QuickCodeEditor (QCE) can talk to a **local OpenAI-compatible HTTP gateway** that implements `POST /v1/chat/completions` and returns `choices[0].message.content`.

- QCE’s ChatGPT provider already emits an OpenAI-style request body (`model`, `messages[]`) and parses the OpenAI-style response (`choices[0].message.content`).
- QCE always sends `Authorization: Bearer <key>` and it **will not send requests unless the key is non-empty**.
- SOTS already has a capable MCP stdio server (`DevTools/python/sots_mcp_server/server.py`) with search, reports, capture image access, BPGen, and Agents runner hooks.
- Current gap: there are **no first-class RAG tools exposed via MCP**, and the Agents runner path currently **hard-requires `OPENAI_API_KEY`**.

Decision: implement a tiny gateway that QCE points at, which forwards the prompt into SOTS Agents (preferred) and (optionally) injects RAG snippets and/or VisualDigest presence notes.

---

## Scope
This case study answers:
- What QCE actually sends/expects when using its “ChatGPT” provider.
- What SOTS MCP server already provides (actual tool surface).
- What’s missing for clean “QCE ↔ SOTS toolchain” integration.
- The thinnest viable “local endpoint” strategy.

Constraints honored in this PASS:
- Docs-only, add-only.
- No Unreal build/run.
- No code edits.

---

## QCE integration seams (what it sends / expects)

### Provider + endpoint + auth behavior
In QCE’s `ChatGPT` provider configuration:
- Endpoint default is OpenAI Chat Completions: `https://api.openai.com/v1/chat/completions`.
- Auth header:
  - Header name: `Authorization`
  - Prefix: `Bearer `
  - Value: QCE’s configured API key.

Important behavioral note:
- QCE treats the provider as unavailable if the API key is empty, and it also blocks both chat and completion requests when the key is empty.
  - This means a local gateway must accept **any non-empty placeholder key**, even if it ignores it.

### Request body shape (OpenAI-style)
For ChatGPT, QCE emits:
- `model`: a string (default configured as `gpt-4o` in settings).
- `messages`: an array of objects like `{ "role": "user", "content": "..." }`.
- `max_tokens`: set from settings (simple vs regular conversation).
- `temperature`: set when supported.

For inline completions (QCE “completion” call), it builds a single `messages` entry:
- `role: "user"`
- `content`: a prompt containing code context and instructions.

### Response parse contract
For ChatGPT, QCE reads:
- Error case: `error.type` and `error.message` (it surfaces `error.message` to the user).
- Success case: `choices[0].message.content` as the assistant text.

This makes a minimal local gateway straightforward: it only needs to return an OpenAI-compatible JSON envelope and ensure `choices[0].message.content` contains the text output.

---

## What SOTS MCP already provides (actual tool list)
SOTS exposes a single unified MCP server: `DevTools/python/sots_mcp_server/server.py` (stdio).

At minimum, the current canonical tools include:

### Help / Info
- `sots_help`
- `sots_server_info`
- `sots_smoketest_all`

### Search
- `sots_search`
- `sots_search_workspace`
- `sots_search_exports`

### Files / Dirs
- `sots_list_dir`
- `sots_read_file`

### Reports
- `sots_list_reports`
- `sots_read_report`

### Capture
- `sots_latest_digest_image`
- `sots_read_image`

### Agents
- `sots_agents_run`
- `sots_agents_help`
- `agents_server_info` (alias path)
- `sots_agents_health`

### BPGen
- `bpgen_ping`
- `bpgen_call` / `manage_bpgen` (forwarder)

### VibeUE compat layer
- `manage_blueprint`
- `manage_blueprint_function`

Note: `sots_help` also lists additional BPGen/UMG-related and parity-layer tools beyond the minimum groups above.

---

## Gap analysis (current code state)

### 1) No first-class RAG tools in MCP
- RAG indexing/query exists in DevTools (`DevTools/python/sots_rag/...`), but the MCP server does not expose dedicated `sots_rag_*` tools today.

### 2) Devtool allowlist doesn’t include RAG scripts
- MCP’s `sots_run_devtool` uses an allowlist (`ALLOWED_SCRIPTS`).
- `run_rag_index.py` / `run_rag_query.py` are not currently listed.

### 3) No write/apply_patch tools exposed to MCP
- The MCP server is read-only by default; it does not expose a “write to repo” tool surface (even though `SOTS_ALLOW_APPLY` exists for other guarded mutation pathways).

### 4) Agents runner currently hard-requires `OPENAI_API_KEY`
- The Agents runner entrypoint used by MCP rejects requests when `OPENAI_API_KEY` is missing.
- The MCP tool `sots_agents_run` also blocks early when `OPENAI_API_KEY` is missing.

---

## Integration decision: the thinnest local endpoint strategy

### Recommended strategy
Implement a tiny local HTTP gateway that speaks the OpenAI Chat Completions shape:
- `POST /v1/chat/completions`
- `GET /health`

Gateway responsibilities:
1) Accept QCE requests
- Read request `model` and `messages[]`.
- Join/convert messages into a single prompt string suitable for SOTS.
- Accept any non-empty `Authorization: Bearer ...` value (QCE requires non-empty).

2) Call SOTS Agents runner (preferred)
- Use the same Python entrypoint/function that the MCP server uses for `sots_agents_run`.

3) Fallback behavior
- If Agents cannot run (e.g., missing `OPENAI_API_KEY` in the current code), return a structured OpenAI-style error:
  - `{ "error": { "type": "qce_error", "message": "..." } }`

4) Optional RAG injection (toggle)
- If enabled, run a RAG query and inject top snippets into the system prompt as **READ-ONLY CONTEXT**.

5) Optional VisualDigest hook (toggle)
- If enabled, call `sots_latest_digest_image` (or just check the image path exists) and include a small note like:
  - “Latest VisualDigest exists at: …; use if needed.”
- Avoid inlining base64 into the prompt by default.

### Why a gateway (instead of having QCE speak MCP directly)
- QCE is already wired for OpenAI/Claude HTTP APIs.
- MCP is stdio JSON-RPC and not what QCE expects.
- A gateway avoids modifying QCE and keeps SOTS MCP’s “stdio safety / no stdout” rules intact.

---

## Minimal config recipe for Ryan (QCE)
In QCE Editor Settings:
- Provider: `ChatGPT`
- Endpoint: `http://127.0.0.1:<PORT>/v1/chat/completions`
- API key: any non-empty placeholder, e.g. `local-dev`

---

## Where to start the gateway from (command line)
Planned location (next PASS): `DevTools/python/sots_mcp_server/run_qce_gateway.py`

Planned start command (example):
- `python DevTools/python/sots_mcp_server/run_qce_gateway.py`

(Exact command + env vars will be documented in `Docs/Tools/QuickCodeEditor_LocalBuddy_Setup.md` in the gateway PASS.)
