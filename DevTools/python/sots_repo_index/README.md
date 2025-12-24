# SOTS Repo Index

This tool builds a local, cacheable index of the SOTS plugin suite to speed up symbol and API lookups.

## Usage

```powershell
python DevTools/python/sots_repo_index/run_repo_index.py --project_root E:\SAS\ShadowsAndShurikens --verbose
```

## Outputs

Reports are written to:

```
<PROJECT_ROOT>\Reports\RepoIndex\
  symbol_map.json
  public_api_map.json
  module_graph.json
  tag_usage.json
  file_manifest.json
  repo_index_report.txt
  repo_index_worklog.md
  repo_index_run.log
```

## Cache

Cache and logs are stored under:

```
DevTools/python/sots_repo_index/_cache/
DevTools/python/sots_repo_index/_logs/
```

The cache enables incremental updates by rescanning only changed files when possible.
