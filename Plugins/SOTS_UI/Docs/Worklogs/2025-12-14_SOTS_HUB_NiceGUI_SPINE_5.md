# SOTS Hub (NiceGUI) — SPINE_5

Date: 2025-12-14

SPINE_5 is the “daily-driver” pass: the Hub becomes a navigation and workflow tool (not just visualization).

## What’s New

### Command Palette (Ctrl+K)

- Global keybind: press `Ctrl+K` on any page to open the Command Palette.
- Fuzzy-ish matching over common actions:
  - `Go: Home / Runs / Plugins / Graphs / Search / Hotspots`
  - `Plugin: <PluginName>` (opens `/plugin/<name>`)
  - Copy helpers:
    - `Copy: Depmap command`
    - `Copy: TODO backlog command`
    - `Copy: Tag usage command`
    - `Copy: API surface command (example)`

### Deep Search (linked navigation)

- `/search` is now a navigation tool, not a text dump.
- Results are shown as a table with:
  - Type (`TODO` / `TAG` / `API`)
  - Plugin
  - File
  - Line (when available)
  - Snippet
  - Actions (`Open Plugin`, `Copy file path`, `Copy snippet`)

#### Focus navigation (`/plugin/<name>?focus...`)

Deep Search uses query params to open the plugin detail page with context:

`/plugin/<name>?focusType=todo&focusFile=...&focusLine=...`

On load, the plugin detail page:

- Shows a “Focused result” badge near the title.
- Best-effort highlights the matching TODO/tag row (file + line when present) and scrolls it into view.

### Hotspots dashboard (`/hotspots`)

- New page that ranks “problem areas”:
  - Top plugins/files by a simple tunable score:
    - `score = (todo_count * 3) + (tag_count * 1)`
- Includes:
  - ECharts bar chart for top plugins
  - Table for top files with `Open Plugin` + `Copy path`
  - Filters:
    - `SOTS_* only`
    - `Contains…` substring
    - `Sort: Score / TODO / Tags`
- Designed to load even if only some reports exist.

### Pinned plugins

- Plugins page and plugin detail page include a Pin/Unpin toggle.
- Pinned plugins show up at the top of:
  - Plugins page
  - Command Palette suggestions

Implementation note: pins are persisted to `DevTools/reports/sots_hub_pins.json`.

## Policy reminders

- The Hub is read-only by default: it does not execute DevTools scripts automatically.
- No builds are triggered by the Hub.
- The Hub prints and writes a log file on every run (see `DevTools/logs/`).

