from __future__ import annotations

SCHEMA_VERSION = 1

OUTPUT_FILES = {
    "symbol_map": "symbol_map.json",
    "public_api_map": "public_api_map.json",
    "module_graph": "module_graph.json",
    "tag_usage": "tag_usage.json",
    "file_manifest": "file_manifest.json",
    "report": "repo_index_report.txt",
    "worklog": "repo_index_worklog.md",
}

REPORT_FILE = OUTPUT_FILES["report"]
WORKLOG_FILE = OUTPUT_FILES["worklog"]
