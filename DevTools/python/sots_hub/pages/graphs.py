from __future__ import annotations

import json

from nicegui import ui

from ..services.report_store import ReportStore
from ..report_loader import ReportBundle
from .layout import page_layout


def render(bundle: ReportBundle, store: ReportStore, state, on_refresh) -> None:
    payload = store.graph_payload()
    filter_state = {
        "showSots": False,
        "contains": "",
        "hideIsolated": False,
        "maxNodes": 200,
    }

    def _apply_filters() -> None:
        ui.run_javascript(f"window.applyGraphFilters({json.dumps(filter_state)})")

    with page_layout("/graphs", state, on_refresh):
        ui.label("Graphs").classes("text-lg font-semibold")
        ui.label("Top inbound dependencies (interactive)").classes("text-sm text-slate-600")

        with ui.row().classes("gap-4 flex-wrap"):
            show_sots = ui.checkbox("Show only SOTS_*", value=False, on_change=lambda _: _refresh_filters())
            contains_input = ui.input("Contains…", on_change=lambda _: _refresh_filters()).props("clearable")
            hide_isolated = ui.checkbox("Hide isolated nodes", value=False, on_change=lambda _: _refresh_filters())
            max_nodes = ui.slider(
                value=200,
                min=50,
                max=500,
                step=10,
                on_change=lambda _: _refresh_filters(),
            ).props("label-visible")
            ui.button("Reset view", on_click=lambda _: _reset_filters()).props("flat color=primary")

        ui.html(_graph_html(payload), sanitize=False).classes("w-full").style("height: 620px")
        ui.run_javascript(_graph_script(payload))

        def _refresh_filters() -> None:
            filter_state["showSots"] = show_sots.value
            filter_state["contains"] = (contains_input.value or "").strip()
            filter_state["hideIsolated"] = hide_isolated.value
            filter_state["maxNodes"] = max_nodes.value
            _apply_filters()

        def _reset_filters() -> None:
            filter_state.update({"showSots": False, "contains": "", "hideIsolated": False, "maxNodes": 200})
            show_sots.value = False
            contains_input.value = ""
            hide_isolated.value = False
            max_nodes.value = 200
            _apply_filters()

        def _graph_timer() -> None:
            changed = state.check_for_updates()
            if "depmap" in changed:
                new_payload = state.report_store.graph_payload()
                ui.run_javascript(f"window.setGraphData({json.dumps(new_payload)})")

        ui.timer(2.0, _graph_timer)


def _graph_html(payload: dict[str, list]) -> str:
    return """
<style>
#cy-container {
    position: relative;
    width: 100%;
}
#cy {
    width: 100%;
    height: 560px;
    border-radius: 12px;
    border: 1px solid #CBD5F5;
}
#cy-tooltip {
    position: absolute;
    top: 16px;
    left: 16px;
    background: rgba(15,23,42,0.85);
    color: #F8FAFC;
    padding: 8px 12px;
    border-radius: 6px;
    font-size: 12px;
    display: none;
    pointer-events: none;
    z-index: 10;
}
</style>
<div id="cy-container">
    <div id="cy"></div>
    <div id="cy-tooltip"></div>
</div>
"""


def _graph_script(payload: dict[str, list]) -> str:
    safe_payload = json.dumps(payload)
    template = """
window.graphPayload = __GRAPH_PAYLOAD__;
window.filterConfig = {
    showSots: false,
    contains: "",
    hideIsolated: false,
    maxNodes: 200,
};

function filterNodes(payload) {
    let nodes = payload.nodes.slice();
    if (window.filterConfig.showSots) {
        nodes = nodes.filter(n => n.id.startsWith("SOTS_"));
    }
    if (window.filterConfig.contains) {
        const needle = window.filterConfig.contains.toLowerCase();
        nodes = nodes.filter(n => n.id.toLowerCase().includes(needle));
    }
    nodes.sort((a, b) => {
        const aScore = (a.meta.inbound || 0) + (a.meta.outbound || 0);
        const bScore = (b.meta.inbound || 0) + (b.meta.outbound || 0);
        return bScore - aScore;
    });
    return nodes.slice(0, window.filterConfig.maxNodes || nodes.length);
}

function filterEdges(payload, nodeSet) {
    return payload.edges.filter(e => nodeSet.has(e.source) && nodeSet.has(e.target));
}

function applyFilters() {
    const nodes = filterNodes(window.graphPayload);
    const nodeIds = new Set(nodes.map(n => n.id));
    let edges = filterEdges(window.graphPayload, nodeIds);
    if (window.filterConfig.hideIsolated) {
        const connected = new Set(edges.flatMap(e => [e.source, e.target]));
        const filteredNodes = nodes.filter(n => connected.has(n.id));
        nodes.length = 0;
        nodes.push(...filteredNodes);
        nodeIds.clear();
        filteredNodes.forEach(n => nodeIds.add(n.id));
        edges = filterEdges(window.graphPayload, nodeIds);
    }
    const elements = [
        ...nodes.map(n => ({ data: { id: n.id, label: n.label, meta: n.meta } })),
        ...edges.map(e => ({ data: { source: e.source, target: e.target } }))
    ];
    window.cy.elements().remove();
    window.cy.add(elements);
    window.cy.layout({ name: "cose", animate: true, animationDuration: 300 }).run();
}

function initGraph() {
    window.cy = cytoscape({
        container: document.getElementById("cy"),
        elements: [],
        style: [
            {
                selector: "node",
                style: {
                    "background-color": "#2563EB",
                    label: "data(label)",
                    color: "#F8FAFC",
                    "text-valign": "center",
                    "text-halign": "center",
                    "font-size": "10px",
                    "text-outline-width": 2,
                    "text-outline-color": "#1E3A8A"
                }
            },
            {
                selector: "edge",
                style: {
                    width: 2,
                    "line-color": "#A5B4FC",
                    "target-arrow-color": "#A5B4FC",
                    "target-arrow-shape": "triangle"
                }
            }
        ]
    });

    const tooltip = document.getElementById("cy-tooltip");

    window.cy.on("tap", "node", evt => {
        const node = evt.target;
        window.location.href = "/plugin/" + encodeURIComponent(node.id());
    });

    window.cy.on("mouseover", "node", evt => {
        const node = evt.target;
        const meta = node.data("meta") || {};
        tooltip.innerHTML = `
            <strong>${node.id()}</strong><br>
            Outbound: ${meta.outbound || 0}<br>
            Inbound: ${meta.inbound || 0}<br>
            TODOs: ${meta.todo || 0}<br>
            Tags: ${meta.tag || 0}
        `;
        tooltip.style.display = "block";
    });

    window.cy.on("mouseout", "node", () => {
        tooltip.style.display = "none";
    });

    applyFilters();
}

window.applyGraphFilters = config => {
    window.filterConfig = config;
    applyFilters();
};

window.resetGraphFilters = () => {
    window.applyGraphFilters({
        showSots: false,
        contains: "",
        hideIsolated: false,
        maxNodes: 200,
    });
};

window.setGraphData = payload => {
    window.graphPayload = payload;
    applyFilters();
};

document.addEventListener("DOMContentLoaded", initGraph);
"""
    return template.replace("__GRAPH_PAYLOAD__", safe_payload)
