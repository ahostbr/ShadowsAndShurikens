from __future__ import annotations

import dataclasses
import json

from nicegui import ui

from ..services.plugin_view_model import PluginViewModel
from ..services.report_store import ReportStore
from .layout import page_layout


def render(plugin_name: str, store: ReportStore, state, on_refresh) -> None:
    payload = PluginViewModel(store).detail_payload(plugin_name)
    payload_json = json.dumps(dataclasses.asdict(payload), ensure_ascii=False)

    with page_layout(f"/plugin/{plugin_name}", state, on_refresh):
        with ui.row().classes("items-center gap-2"):
            pin_button = ui.button().props("flat color=primary")

            def _sync_pin_button() -> None:
                if plugin_name in state.pins:
                    pin_button.set_text("Unpin")
                    pin_button.props("icon=push_pin")
                else:
                    pin_button.set_text("Pin")
                    pin_button.props("icon=push_pin")

            def _toggle_pin() -> None:
                state.toggle_pin(plugin_name)
                _sync_pin_button()
                ui.notify(f"{'Pinned' if plugin_name in state.pins else 'Unpinned'}: {plugin_name}")

            pin_button.on_click(_toggle_pin)
            _sync_pin_button()

        ui.html(_plugin_html(), sanitize=False).classes("w-full")
        ui.run_javascript(_plugin_script(payload_json))

        def _plugin_timer() -> None:
            changed = state.check_for_updates()
            if changed:
                new_payload = PluginViewModel(store).detail_payload(plugin_name)
                js_payload = json.dumps(dataclasses.asdict(new_payload), ensure_ascii=False)
                ui.run_javascript(f"window.updatePluginDetail({js_payload})")

        ui.timer(2.0, _plugin_timer)




def _plugin_html() -> str:
    return """
<style>
.plugin-page {
    width: 100%;
    display: flex;
    flex-direction: column;
    gap: 1rem;
}
.plugin-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
}
.plugin-stats {
    display: flex;
    flex-wrap: wrap;
    gap: 0.5rem;
}
.stat-pill {
    background: #1e40af;
    color: #f8fafc;
    padding: 0.35rem 0.75rem;
    border-radius: 999px;
    font-size: 0.85rem;
}
.section {
    border: 1px solid #cbd5f5;
    border-radius: 12px;
    padding: 1rem;
    background: #f8fafc;
}
.dep-list {
    display: flex;
    flex-wrap: wrap;
    gap: 0.5rem;
}
.dep-chip {
    padding: 0.3rem 0.6rem;
    background: #e0f2fe;
    border-radius: 999px;
    font-size: 0.85rem;
}
.table-wrapper {
    width: 100%;
    overflow-x: auto;
}
.detail-table {
    width: 100%;
    border-collapse: collapse;
}
.detail-table th,
.detail-table td {
    border-bottom: 1px solid #cbd5f5;
    padding: 0.3rem 0.6rem;
    text-align: left;
    font-size: 0.85rem;
    font-family: "JetBrains Mono", "Consolas", monospace;
}
.detail-table th {
    font-weight: 600;
    background: #e0e7ff;
}
.api-key {
    font-weight: 600;
}
.btn-link {
    background: transparent;
    border: none;
    padding: 0;
    color: #2563eb;
    cursor: pointer;
    font-size: 0.85rem;
}
.api-section {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}
.section-title {
    font-size: 1.1rem;
    font-weight: 600;
}
.copy-button {
    background: #1e3a8a;
    color: #fff;
    border: none;
    border-radius: 8px;
    padding: 0.4rem 0.9rem;
    cursor: pointer;
    font-size: 0.85rem;
}
.focus-badge {
    display: inline-flex;
    align-items: center;
    gap: 0.35rem;
    background: #fef3c7;
    color: #92400e;
    padding: 0.25rem 0.6rem;
    border-radius: 999px;
    font-size: 0.8rem;
    width: fit-content;
    margin-top: 0.35rem;
}
.focused-row {
    background: #fffbeb;
    outline: 2px solid #f59e0b;
}
@media (max-width: 768px) {
    .plugin-header {
        flex-direction: column;
        align-items: flex-start;
        gap: 0.5rem;
    }
}
</style>
<div class="plugin-page">
  <div class="plugin-header">
    <div>
      <h2 id="plugin-name"></h2>
      <p id="plugin-note" class="text-sm text-slate-600"></p>
      <div id="focus-badge" class="focus-badge" style="display: none;"></div>
    </div>
    <button class="copy-button" onclick="navigator.clipboard.writeText(window.pluginDetail.name)">Copy plugin name</button>
  </div>
  <div class="plugin-stats" id="plugin-stats"></div>
  <div class="section">
    <div class="section-title">Dependencies</div>
    <div class="dep-group">
      <div><strong>Outbound</strong></div>
      <div id="outbound-list" class="dep-list"></div>
    </div>
    <div class="dep-group">
      <div><strong>Inbound</strong></div>
      <div id="inbound-list" class="dep-list"></div>
    </div>
  </div>
  <div class="section">
    <div class="section-title">TODO backlog</div>
    <button id="todo-toggle" class="btn-link" onclick="toggleTodoLimit()">Show more</button>
    <div class="table-wrapper" id="todo-table"></div>
  </div>
  <div class="section">
    <div class="section-title">Tag usage</div>
    <button id="tag-toggle" class="btn-link" onclick="toggleTagLimit()">Show more</button>
    <div class="table-wrapper" id="tag-table"></div>
  </div>
  <div class="section api-section" id="api-section"></div>
</div>
"""


def _plugin_script(payload_json: str) -> str:
    template = """
window.pluginDetail = __PAYLOAD_JSON__;
window.todoLimit = 200;
window.todoShowAll = false;
window.tagLimit = 200;
window.tagShowAll = false;
window.focusContext = null;

function escapeHtml(text) {
  const div = document.createElement("div");
  div.textContent = String(text ?? "");
  return div.innerHTML;
}

function parseFocus() {
  const params = new URLSearchParams(window.location.search || "");
  const focusType = (params.get("focusType") || "").toLowerCase().trim();
  const focusFile = (params.get("focusFile") || "").trim();
  const focusLineRaw = (params.get("focusLine") || "").trim();
  const focusLine = focusLineRaw ? Number(focusLineRaw) : null;
  if (!focusType && !focusFile) {
    window.focusContext = null;
    return;
  }
  window.focusContext = {
    type: focusType || null,
    file: focusFile || null,
    line: Number.isFinite(focusLine) ? focusLine : null,
  };
}

function renderFocusBadge() {
  const badge = document.getElementById("focus-badge");
  if (!badge) return;
  const focus = window.focusContext;
  if (!focus || (!focus.type && !focus.file)) {
    badge.style.display = "none";
    badge.textContent = "";
    return;
  }
  const parts = [];
  if (focus.type) parts.push(focus.type.toUpperCase());
  if (focus.file) parts.push(focus.file);
  if (focus.line) parts.push("L" + focus.line);
  badge.textContent = "Focused result: " + parts.join(" · ");
  badge.style.display = "inline-flex";
}

function applyFocus() {
  const focus = window.focusContext;
  if (!focus || !focus.file) return;

  if (focus.type === "api") {
    const api = document.getElementById("api-section");
    if (api) api.scrollIntoView({ behavior: "smooth", block: "start" });
    return;
  }

  const type = focus.type || "";
  const rows = document.querySelectorAll(`tr[data-type="${type}"]`);
  for (const row of rows) {
    if (String(row.dataset.file || "") !== focus.file) continue;
    if (focus.line && Number(row.dataset.line || 0) !== focus.line) continue;
    row.classList.add("focused-row");
    row.scrollIntoView({ behavior: "smooth", block: "center" });
    break;
  }
}

function renderPluginDetail(data) {
  parseFocus();
  document.getElementById("plugin-name").textContent = data.name;
  document.getElementById("plugin-note").textContent = data.api.note || "";
  renderFocusBadge();
  const statsEl = document.getElementById("plugin-stats");
  statsEl.innerHTML = `
    <span class="stat-pill">Outbound deps: ${data.outbound.length}</span>
    <span class="stat-pill">Inbound deps: ${data.inbound.length}</span>
    <span class="stat-pill">TODOs: ${data.todo_count}</span>
    <span class="stat-pill">Tags: ${data.tag_count}</span>
  `;
  renderDeps("outbound-list", data.outbound);
  renderDeps("inbound-list", data.inbound);
  renderTodoTable(data.todo_entries);
  renderTagTable(data.tag_entries);
  renderAPISection(data.api);
  applyFocus();
}

function renderDeps(containerId, items) {
  const container = document.getElementById(containerId);
  if (!items.length) {
    container.innerHTML = "<span class='text-sm text-slate-500'>None</span>";
    return;
  }
  container.innerHTML = items
    .map(
      item => `<a class="dep-chip" href="/plugin/${encodeURIComponent(item)}">${item}</a>`
    )
    .join("");
}

function renderTodoTable(entries) {
  const container = document.getElementById("todo-table");
  const toggle = document.getElementById("todo-toggle");
  if (!entries.length) {
    container.innerHTML = "<p class='text-sm text-slate-500'>No TODO entries.</p>";
    toggle.style.display = "none";
    return;
  }
  toggle.style.display = entries.length > window.todoLimit ? "inline-flex" : "none";
  const display = window.todoShowAll ? entries : entries.slice(0, window.todoLimit);
  const rows = display
    .map(
      (entry, idx) => {
        const line = entry.line || "";
        return `
        <tr data-type="todo" data-file="${escapeHtml(entry.file)}" data-line="${line}">
          <td>${escapeHtml(entry.file)}</td>
          <td>${escapeHtml(line || "-")}</td>
          <td>${escapeHtml(entry.text)}</td>
        </tr>`;
      }
    )
    .join("");
  container.innerHTML = `
    <table class="detail-table">
      <thead>
        <tr><th>File</th><th>Line</th><th>Text</th></tr>
      </thead>
      <tbody>${rows}</tbody>
    </table>`;
  toggle.textContent = window.todoShowAll ? "Show less" : "Show more";
}

function toggleTodoLimit() {
  window.todoShowAll = !window.todoShowAll;
  renderTodoTable(window.pluginDetail.todo_entries);
}

function renderTagTable(entries) {
  const container = document.getElementById("tag-table");
  const toggle = document.getElementById("tag-toggle");
  if (!entries.length) {
    container.innerHTML = "<p class='text-sm text-slate-500'>No tag matches.</p>";
    toggle.style.display = "none";
    return;
  }
  toggle.style.display = entries.length > window.tagLimit ? "inline-flex" : "none";
  const display = window.tagShowAll ? entries : entries.slice(0, window.tagLimit);
  const rows = display
    .map(
      entry => {
        const line = entry.line || "";
        return `
        <tr data-type="tag" data-file="${escapeHtml(entry.file)}" data-line="${line}">
          <td>${escapeHtml(entry.file)}</td>
          <td>${escapeHtml(line || "-")}</td>
          <td>${escapeHtml(entry.text)}</td>
        </tr>`;
      }
    )
    .join("");
  container.innerHTML = `
    <table class="detail-table">
      <thead>
        <tr><th>File</th><th>Line</th><th>Text</th></tr>
      </thead>
      <tbody>${rows}</tbody>
    </table>`;
  toggle.textContent = window.tagShowAll ? "Show less" : "Show more";
}

function toggleTagLimit() {
  window.tagShowAll = !window.tagShowAll;
  renderTagTable(window.pluginDetail.tag_entries);
}

function renderAPISection(api) {
  const container = document.getElementById("api-section");
  if (!api) {
    container.innerHTML = "<p class='text-sm text-slate-500'>API data not available.</p>";
    return;
  }
  if (!api.available) {
    container.innerHTML = `<p class='text-sm text-slate-500'>${api.note}</p>`;
    return;
  }
  const typeRows = Object.entries(api.types || {})
    .map(([kind, amount]) => `<div><span class="api-key">${kind}:</span> ${amount}</div>`)
    .join("");
  container.innerHTML = `
    <div class="section-title">API surface</div>
    <div>Plugin: ${api.plugin}</div>
    <div>Headers scanned: ${api.headers}</div>
    <div>Functions: ${api.functions}</div>
    <div>Properties: ${api.properties}</div>
    <div>${typeRows}</div>
  `;
}

window.updatePluginDetail = data => {
  window.pluginDetail = data;
  renderPluginDetail(data);
};

document.addEventListener("DOMContentLoaded", () => renderPluginDetail(window.pluginDetail));
"""
    return template.replace("__PAYLOAD_JSON__", payload_json)
