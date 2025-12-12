# bpgen_coverage_report.py
#
# Scans all BPGen snippet packs, builds a coverage map of engine function usage,
# and reports duplicates where the same engine function is wrapped by multiple
# packs/snippets. Also writes a JSON report for future tooling.
#
# Suggested location:
#   DevTools/python/bpgen_coverage_report.py
#
# Usage (from project root, adjust path as needed):
#   py DevTools/python/bpgen_coverage_report.py

import os
import json
from collections import defaultdict

# --- CONFIG -------------------------------------------------------------

PACKS_ROOT = os.path.join(
    os.path.dirname(__file__),
    "..", "bpgen_snippets", "packs"
)  # DevTools/python/../bpgen_snippets/packs

REPORT_JSON = os.path.join(
    os.path.dirname(__file__),
    "bpgen_coverage_report.json"
)

# --- HELPERS ------------------------------------------------------------

def read_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def find_first_call_function_node(graphspec):
    """
    Given a GraphSpec dict, return the first node dict whose NodeType is
    'K2Node_CallFunction'. Returns None if not found.
    """
    nodes = graphspec.get("Nodes", [])
    for node in nodes:
        if node.get("NodeType") == "K2Node_CallFunction":
            return node
    return None

# --- MAIN SCAN ----------------------------------------------------------

def main():
    if not os.path.isdir(PACKS_ROOT):
        print(f"[BPGenCoverage] Packs root not found: {PACKS_ROOT}")
        return

    coverage = {}  # engine_function_path -> list of entries
    # Each entry: {
    #   "pack_name": str,
    #   "snippet_name": str,
    #   "bpgen_function_name": str,
    #   "job_file": str,
    #   "graphspec_file": str
    # }

    total_packs = 0
    total_snippets = 0

    for pack_dir_name in os.listdir(PACKS_ROOT):
        pack_dir = os.path.join(PACKS_ROOT, pack_dir_name)
        if not os.path.isdir(pack_dir):
            continue

        meta_path = os.path.join(pack_dir, "pack_meta.json")
        if not os.path.isfile(meta_path):
            continue

        try:
            meta = read_json(meta_path)
        except Exception as e:
            print(f"[BPGenCoverage] ERROR reading pack_meta: {meta_path} :: {e}")
            continue

        pack_name = meta.get("pack_name", pack_dir_name)
        snippets = meta.get("snippets", [])
        total_packs += 1

        for snippet in snippets:
            snippet_name = snippet.get("snippet_name", "UnknownSnippet")
            job_rel = snippet.get("job_file")
            graph_rel = snippet.get("graphspec_file")

            if not job_rel or not graph_rel:
                print(f"[BPGenCoverage] WARNING snippet missing job/graphspec in pack {pack_name}: {snippet_name}")
                continue

            job_path = os.path.join(pack_dir, job_rel)
            graph_path = os.path.join(pack_dir, graph_rel)

            if not os.path.isfile(job_path) or not os.path.isfile(graph_path):
                print(f"[BPGenCoverage] WARNING missing files for {pack_name}.{snippet_name}")
                continue

            try:
                job = read_json(job_path)
                graphspec = read_json(graph_path)
            except Exception as e:
                print(f"[BPGenCoverage] ERROR reading job/graphspec for {pack_name}.{snippet_name}: {e}")
                continue

            fn_def = job.get("Function", {})
            bpgen_func_name = fn_def.get("FunctionName", "UnknownBPGenFunc")

            call_node = find_first_call_function_node(graphspec)
            if not call_node:
                # Some snippets might be pure logic, no direct engine call; still track them
                engine_path = "NO_CALL_FUNCTION_NODE"
            else:
                engine_path = call_node.get("FunctionPath", "UNKNOWN_FUNCTION_PATH")

            entry = {
                "pack_name": pack_name,
                "snippet_name": snippet_name,
                "bpgen_function_name": bpgen_func_name,
                "job_file": os.path.relpath(job_path, PACKS_ROOT),
                "graphspec_file": os.path.relpath(graph_path, PACKS_ROOT),
            }

            coverage.setdefault(engine_path, []).append(entry)
            total_snippets += 1

    # --- Build duplicate summary ----------------------------------------

    duplicates = {
        fn_path: entries
        for fn_path, entries in coverage.items()
        if fn_path not in ("NO_CALL_FUNCTION_NODE", "UNKNOWN_FUNCTION_PATH")
        and len(entries) > 1
    }

    # --- Write JSON report ----------------------------------------------

    report = {
        "packs_root": os.path.abspath(PACKS_ROOT),
        "total_packs_scanned": total_packs,
        "total_snippets_scanned": total_snippets,
        "total_engine_functions": len(coverage),
        "duplicate_engine_functions": len(duplicates),
        "coverage": coverage,
        "duplicates": duplicates,
    }

    with open(REPORT_JSON, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)

    # --- Console summary -------------------------------------------------

    print("=== BPGen Coverage Report ===")
    print(f"Packs scanned:        {total_packs}")
    print(f"Snippets scanned:     {total_snippets}")
    print(f"Engine functions:     {len(coverage)}")
    print(f"Duplicate functions:  {len(duplicates)}")
    print()
    if duplicates:
        print("Some duplicate engine functions found:")
        for fn_path, entries in list(duplicates.items())[:15]:
            print(f"  {fn_path}:")
            for e in entries:
                print(f"    - {e['pack_name']} :: {e['snippet_name']} ({e['bpgen_function_name']})")
        if len(duplicates) > 15:
            print(f"  ...and {len(duplicates) - 15} more.")
    else:
        print("No duplicates detected (per engine function path).")
    print()
    print(f"Full JSON report written to: {os.path.abspath(REPORT_JSON)}")


if __name__ == "__main__":
    main()
