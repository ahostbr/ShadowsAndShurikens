
from __future__ import annotations

import datetime as dt
import fnmatch
import hashlib
import json
import os
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from schemas import OUTPUT_FILES, REPORT_FILE, WORKLOG_FILE, SCHEMA_VERSION

CACHE_VERSION = 2
CACHE_FILE = "repo_index_cache.json"

SCAN_EXTS = {".h", ".hpp", ".cpp", ".inl", ".cs", ".uplugin", ".ini", ".md"}
CODE_EXTS = {".h", ".hpp", ".cpp", ".inl"}

RE_UCLASS = re.compile(r"\bUCLASS\b")
RE_USTRUCT = re.compile(r"\bUSTRUCT\b")
RE_UENUM = re.compile(r"\bUENUM\b")
RE_UINTERFACE = re.compile(r"\bUINTERFACE\b")
RE_UFUNCTION = re.compile(r"\bUFUNCTION\b")
RE_UPROPERTY = re.compile(r"\bUPROPERTY\b")
RE_EXPORT = re.compile(r"\b([A-Z0-9_]+_API)\b")
RE_ENUM = re.compile(r"^\s*enum\b")
RE_CLASS = re.compile(r"^\s*(class|struct)\s+")
RE_FUNC_NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
RE_DELEGATE = re.compile(r"DECLARE_[A-Z0-9_]+_DELEGATE[^\(]*\(\s*([A-Za-z_][A-Za-z0-9_]*)")
RE_TAG_LITERAL = re.compile(r"\"([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)+)\"")
RE_REQUEST_TAG = re.compile(r"RequestGameplayTag\s*\(\s*(?:TEXT\(|FName\()?\"([^\"]+)\"")
RE_TAG_INI = re.compile(r"Tag\s*=\s*\"([^\"]+)\"")
RE_DEFINE_TAG = re.compile(r"UE_DEFINE_GAMEPLAY_TAG(?:_COMMENT)?\s*\(\s*[^,]+,\s*\"([^\"]+)\"")
RE_ADD_NATIVE_TAG = re.compile(r"AddNativeGameplayTag\s*\(\s*(?:TEXT\()?\"([^\"]+)\"")
RE_TAG_SHAPE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z0-9_]+)+$")
RE_IPV4 = re.compile(r"^\d+\.\d+\.\d+\.\d+$")

TAG_SUFFIX_BLACKLIST = {
    "h",
    "hpp",
    "cpp",
    "inl",
    "cs",
    "ini",
    "md",
    "json",
    "txt",
    "csv",
    "uplugin",
    "uasset",
    "umap",
    "png",
    "jpg",
    "jpeg",
    "webp",
    "wav",
    "mp3",
    "ogg",
}

DEFAULT_TAG_ROOTS = {
    "SOTS",
    "SAS",
    "Input",
    "Interaction",
    "Player",
    "AI",
    "GSM",
    "MMSS",
    "KEM",
    "Mission",
    "SkillTree",
    "INV",
    "UI",
    "FX",
}


def detect_project_root(start: Optional[Path] = None) -> Path:
    if start is None:
        start = Path(__file__).resolve()
    path = start if start.is_dir() else start.parent
    for parent in [path] + list(path.parents):
        if (parent / "Plugins").is_dir() and (parent / "DevTools" / "python").is_dir():
            return parent
    raise RuntimeError("Could not auto-detect project root. Use --project_root.")


def _sha1_file(path: Path) -> str:
    h = hashlib.sha1()
    with path.open("rb") as fh:
        while True:
            chunk = fh.read(1024 * 1024)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def _is_likely_gameplay_tag(tag: str) -> bool:
    if not tag:
        return False
    if len(tag) > 128:
        return False
    if RE_IPV4.match(tag):
        return False
    if not RE_TAG_SHAPE.match(tag):
        return False
    parts = tag.split(".")
    if any(part.isdigit() for part in parts):
        return False
    if parts[-1].lower() in TAG_SUFFIX_BLACKLIST:
        return False
    return True


class RepoIndexer:
    def __init__(
        self,
        project_root: Path,
        plugins_dir: Path,
        reports_dir: Path,
        changed_only: bool = True,
        full: bool = False,
        plugin_filter: str = "",
        verbose: bool = False,
    ) -> None:
        self.project_root = Path(project_root).resolve()
        self.plugins_dir = Path(plugins_dir).resolve()
        self.reports_dir = Path(reports_dir).resolve()
        self.tool_root = Path(__file__).resolve().parent
        self.cache_dir = self.tool_root / "_cache"
        self.logs_dir = self.tool_root / "_logs"
        self.cache_path = self.cache_dir / CACHE_FILE
        self.changed_only = bool(changed_only)
        self.full = bool(full)
        self.plugin_filter = (plugin_filter or "").strip()
        self.verbose = bool(verbose)
        self._log_lines: List[str] = []

    def log(self, msg: str) -> None:
        line = f"[repo_index] {msg}"
        print(line)
        ts = dt.datetime.now().isoformat(timespec="seconds")
        self._log_lines.append(f"{ts} {msg}")

    def _flush_log(self) -> None:
        if not self._log_lines:
            return
        self.reports_dir.mkdir(parents=True, exist_ok=True)
        log_path = self.reports_dir / "repo_index_run.log"
        with log_path.open("a", encoding="utf-8") as fh:
            for line in self._log_lines:
                fh.write(line + "\n")
        self._log_lines.clear()

    def _ensure_dirs(self) -> None:
        self.reports_dir.mkdir(parents=True, exist_ok=True)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self.logs_dir.mkdir(parents=True, exist_ok=True)

    def _load_cache(self) -> Dict[str, object]:
        if not self.cache_path.exists():
            return {}
        try:
            data = json.loads(self.cache_path.read_text(encoding="utf-8"))
        except Exception:
            return {}
        if data.get("version") != CACHE_VERSION:
            return {}
        return data

    def _save_cache(self, cache: Dict[str, object]) -> None:
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self.cache_path.write_text(json.dumps(cache, indent=2, sort_keys=True), encoding="utf-8")

    def _rel_path(self, path: Path) -> str:
        return path.resolve().relative_to(self.project_root).as_posix()

    def _category_for_path(self, path: Path) -> str:
        ext = path.suffix.lower()
        if ext in {".h", ".hpp", ".inl"}:
            return "header"
        if ext == ".cpp":
            return "source"
        if ext == ".cs":
            return "build"
        if ext == ".uplugin":
            return "uplugin"
        if ext == ".ini":
            return "ini"
        if ext == ".md":
            return "md"
        return "other"

    def _collect_plugins(self) -> List[Tuple[str, Path]]:
        plugins: List[Tuple[str, Path]] = []
        if not self.plugins_dir.is_dir():
            return plugins
        patterns = [p.strip() for p in self.plugin_filter.split(",") if p.strip()]
        for child in sorted(self.plugins_dir.iterdir(), key=lambda p: p.name.lower()):
            if not child.is_dir():
                continue
            name = child.name
            if patterns:
                if not any(fnmatch.fnmatch(name, pat) for pat in patterns):
                    continue
            plugins.append((name, child))
        if patterns:
            return plugins
        sots = [p for p in plugins if fnmatch.fnmatch(p[0], "SOTS_*")]
        others = [p for p in plugins if not fnmatch.fnmatch(p[0], "SOTS_*")]
        return sots + others

    def _collect_files(self) -> List[Path]:
        files: List[Path] = []
        skip_dirs = {"Binaries", "Intermediate", "Saved", "DerivedDataCache", ".git", ".vs", ".idea"}

        def walk_dir(root: Path) -> None:
            for dirpath, dirnames, filenames in os.walk(root):
                dirnames[:] = [d for d in dirnames if d not in skip_dirs]
                for name in filenames:
                    path = Path(dirpath) / name
                    if path.suffix.lower() in SCAN_EXTS:
                        files.append(path)

        config_dir = self.project_root / "Config"
        if config_dir.is_dir():
            walk_dir(config_dir)

        for plugin_name, plugin_dir in self._collect_plugins():
            uplugin = plugin_dir / f"{plugin_name}.uplugin"
            if uplugin.exists():
                files.append(uplugin)
            src_dir = plugin_dir / "Source"
            if src_dir.is_dir():
                walk_dir(src_dir)
            cfg_dir = plugin_dir / "Config"
            if cfg_dir.is_dir():
                walk_dir(cfg_dir)

        return files
    def _build_manifest(
        self,
        files: List[Path],
        cache_manifest: Dict[str, Dict[str, object]],
    ) -> Tuple[Dict[str, Dict[str, object]], List[str], int]:
        manifest: Dict[str, Dict[str, object]] = {}
        changed: List[str] = []
        cache_hits = 0

        for path in files:
            rel = self._rel_path(path)
            stat = path.stat()
            size = int(stat.st_size)
            mtime = int(stat.st_mtime)
            category = self._category_for_path(path)
            cached = cache_manifest.get(rel)
            if self.changed_only and cached:
                if cached.get("size") == size and cached.get("mtime") == mtime:
                    manifest[rel] = cached
                    cache_hits += 1
                    continue
            sha1 = _sha1_file(path)
            record = {"path": rel, "size": size, "mtime": mtime, "sha1": sha1, "category": category}
            manifest[rel] = record
            changed.append(rel)

        return manifest, changed, cache_hits

    def _read_text(self, path: Path) -> str:
        try:
            return path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            return path.read_text(encoding="utf-8", errors="ignore")

    def _extract_plugin_module(self, rel_path: str) -> Tuple[str, str]:
        parts = Path(rel_path).parts
        plugin = ""
        module = ""
        if "Plugins" in parts:
            idx = parts.index("Plugins")
            if idx + 1 < len(parts):
                plugin = parts[idx + 1]
        if "Source" in parts:
            idx = parts.index("Source")
            if idx + 1 < len(parts):
                module = parts[idx + 1]
        return plugin, module

    def _collect_macro_block(self, lines: List[str], start_idx: int) -> Tuple[str, int]:
        text = lines[start_idx].strip()
        idx = start_idx
        while ")" not in text and idx + 1 < len(lines):
            idx += 1
            text = text + " " + lines[idx].strip()
        return text, idx

    def _next_code_line(self, lines: List[str], start_idx: int) -> Tuple[str, int]:
        for idx in range(start_idx, len(lines)):
            s = lines[idx].strip()
            if not s or s.startswith("//"):
                continue
            return s, idx
        return "", -1

    def _extract_function_name(self, signature: str) -> str:
        matches = RE_FUNC_NAME.findall(signature)
        return matches[-1] if matches else ""

    def _extract_export_macro(self, line: str) -> str:
        m = RE_EXPORT.search(line)
        return m.group(1) if m else ""

    def _parse_macro_args(self, macro_text: str) -> str:
        start = macro_text.find("(")
        end = macro_text.rfind(")")
        if start == -1 or end == -1 or end <= start:
            return ""
        return macro_text[start + 1 : end]

    def _parse_blueprint_meta(self, macro_args: str) -> Tuple[List[str], str]:
        keywords: List[str] = []
        category = ""
        for part in [p.strip() for p in macro_args.split(",") if p.strip()]:
            if "=" in part:
                key, val = part.split("=", 1)
                if key.strip() == "Category":
                    category = val.strip().strip("\"")
            else:
                keywords.append(part)
        return keywords, category

    def _is_forward_decl(self, line: str) -> bool:
        if ";" in line and "{" not in line and ":" not in line:
            return True
        return False

    def _parse_class_decl(self, line: str) -> Tuple[str, str, str]:
        m = re.match(r"^\s*(class|struct)\s+(?:(\w+_API)\s+)?(\w+)", line)
        if not m:
            return "", "", ""
        exports = m.group(2) or ""
        name = m.group(3) or ""
        kind = "CLASS" if m.group(1) == "class" else "STRUCT"
        return kind, name, exports

    def _parse_enum_decl(self, line: str) -> str:
        m = re.match(r"^\s*enum\s+(?:class\s+)?(\w+)", line)
        return m.group(1) if m else ""

    def _extract_tags_from_line(self, line: str, allow_roots: set[str]) -> List[str]:
        tags = set()

        def maybe_add(tag: str, require_root: bool) -> None:
            if not _is_likely_gameplay_tag(tag):
                return
            if require_root:
                root = tag.split(".")[0].lower()
                if root not in allow_roots:
                    return
            tags.add(tag)

        for match in RE_DEFINE_TAG.finditer(line):
            maybe_add(match.group(1), False)
        for match in RE_REQUEST_TAG.finditer(line):
            maybe_add(match.group(1), False)
        for match in RE_ADD_NATIVE_TAG.finditer(line):
            maybe_add(match.group(1), False)
        for match in RE_TAG_LITERAL.finditer(line):
            maybe_add(match.group(1), True)

        return sorted(tags)

    def _build_tag_occ(self, rel_path: str, line_num: int, plugin: str, module: str, line: str) -> Dict[str, object]:
        snippet = line.strip().replace("\t", " ")
        if len(snippet) > 120:
            snippet = snippet[:120]
        return {
            "file": rel_path,
            "line": line_num,
            "plugin": plugin,
            "module": module,
            "context_snippet": snippet,
        }

    def _build_tag_root_allowlist(self, tags_defined_in_ini: List[str]) -> set[str]:
        allow = {root.lower() for root in DEFAULT_TAG_ROOTS}
        for tag in tags_defined_in_ini:
            root = tag.split(".")[0].strip()
            if root:
                allow.add(root.lower())
        return allow
    def _parse_code_file(
        self,
        rel_path: str,
        text: str,
        plugin: str,
        module: str,
        allow_roots: set[str],
    ) -> Tuple[List[Dict[str, object]], Dict[str, List[Dict[str, object]]], Dict[str, object]]:
        lines = text.splitlines()
        symbols: List[Dict[str, object]] = []
        api: Dict[str, List[Dict[str, object]]] = {
            "functions": [],
            "properties": [],
            "subsystems": [],
            "components": [],
        }
        tag_occ: Dict[str, List[Dict[str, object]]] = defaultdict(list)
        tags_seen: set[str] = set()

        current_class = ""
        pending_macro = ""

        i = 0
        while i < len(lines):
            line = lines[i]
            stripped = line.strip()
            if stripped:
                for tag in self._extract_tags_from_line(stripped, allow_roots):
                    tags_seen.add(tag)
                    tag_occ[tag].append(self._build_tag_occ(rel_path, i + 1, plugin, module, line))

            if not stripped or stripped.startswith("//"):
                i += 1
                continue

            if pending_macro:
                if self._is_forward_decl(stripped):
                    pending_macro = ""
                else:
                    if pending_macro in {"UCLASS", "USTRUCT", "UINTERFACE"}:
                        if RE_CLASS.search(stripped):
                            kind, name, exports = self._parse_class_decl(stripped)
                            if name:
                                symbols.append(
                                    {
                                        "kind": pending_macro,
                                        "name": name,
                                        "plugin": plugin,
                                        "module": module,
                                        "header": rel_path,
                                        "source": "",
                                        "macro_context": {"has_" + pending_macro.lower(): True},
                                        "line": i + 1,
                                        "signature": stripped,
                                        "exports": exports,
                                        "confidence": "high",
                                    }
                                )
                                current_class = name
                                if "UGameInstanceSubsystem" in stripped:
                                    api["subsystems"].append({"class": name, "type": "UGameInstanceSubsystem", "header": rel_path})
                                if "UWorldSubsystem" in stripped:
                                    api["subsystems"].append({"class": name, "type": "UWorldSubsystem", "header": rel_path})
                                if "UEngineSubsystem" in stripped:
                                    api["subsystems"].append({"class": name, "type": "UEngineSubsystem", "header": rel_path})
                                if "UActorComponent" in stripped:
                                    api["components"].append({"class": name, "header": rel_path})
                                pending_macro = ""
                                i += 1
                                continue
                    if pending_macro == "UENUM" and RE_ENUM.search(stripped):
                        enum_name = self._parse_enum_decl(stripped)
                        if enum_name:
                            symbols.append(
                                {
                                    "kind": "UENUM",
                                    "name": enum_name,
                                    "plugin": plugin,
                                    "module": module,
                                    "header": rel_path,
                                    "source": "",
                                    "macro_context": {"has_uenum": True},
                                    "line": i + 1,
                                    "signature": stripped,
                                    "exports": "",
                                    "confidence": "high",
                                }
                            )
                            pending_macro = ""
                            i += 1
                            continue

            if RE_UCLASS.search(stripped):
                pending_macro = "UCLASS"
                i += 1
                continue
            if RE_USTRUCT.search(stripped):
                pending_macro = "USTRUCT"
                i += 1
                continue
            if RE_UINTERFACE.search(stripped):
                pending_macro = "UINTERFACE"
                i += 1
                continue
            if RE_UENUM.search(stripped):
                pending_macro = "UENUM"
                i += 1
                continue

            if RE_UFUNCTION.search(stripped):
                macro_text, end_idx = self._collect_macro_block(lines, i)
                macro_args = self._parse_macro_args(macro_text)
                blueprint_meta, category = self._parse_blueprint_meta(macro_args)
                sig_line, sig_idx = self._next_code_line(lines, end_idx + 1)
                func_name = self._extract_function_name(sig_line)
                if func_name:
                    symbols.append(
                        {
                            "kind": "FUNCTION",
                            "name": func_name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {"has_ufunction": True},
                            "line": sig_idx + 1 if sig_idx >= 0 else i + 1,
                            "signature": sig_line or stripped,
                            "exports": "",
                            "confidence": "high",
                        }
                    )
                    if "BlueprintCallable" in blueprint_meta or "BlueprintPure" in blueprint_meta:
                        api["functions"].append(
                            {
                                "owning_class": current_class,
                                "function_name": func_name,
                                "header": rel_path,
                                "line": sig_idx + 1 if sig_idx >= 0 else i + 1,
                                "blueprint_meta": blueprint_meta,
                                "category": category,
                            }
                        )
                i = end_idx + 1
                continue

            if RE_UPROPERTY.search(stripped):
                macro_text, end_idx = self._collect_macro_block(lines, i)
                macro_args = self._parse_macro_args(macro_text)
                if "BlueprintReadWrite" in macro_args or "BlueprintReadOnly" in macro_args:
                    prop_line, prop_idx = self._next_code_line(lines, end_idx + 1)
                    if prop_line:
                        tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", prop_line)
                        prop_name = tokens[-1] if tokens else ""
                        if prop_name:
                            api["properties"].append(
                                {
                                    "owning_class": current_class,
                                    "property_name": prop_name,
                                    "header": rel_path,
                                    "line": prop_idx + 1 if prop_idx >= 0 else i + 1,
                                }
                            )
                i = end_idx + 1
                continue

            if "DECLARE_" in stripped and "DELEGATE" in stripped:
                match = RE_DELEGATE.search(stripped)
                if match:
                    name = match.group(1)
                    symbols.append(
                        {
                            "kind": "DELEGATE",
                            "name": name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {"has_delegate": True},
                            "line": i + 1,
                            "signature": stripped,
                            "exports": "",
                            "confidence": "high",
                        }
                    )

            if RE_CLASS.search(stripped) and not self._is_forward_decl(stripped):
                kind, name, exports = self._parse_class_decl(stripped)
                if name:
                    symbols.append(
                        {
                            "kind": kind,
                            "name": name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {},
                            "line": i + 1,
                            "signature": stripped,
                            "exports": exports,
                            "confidence": "low",
                        }
                    )
                    current_class = name
                    if "UGameInstanceSubsystem" in stripped:
                        api["subsystems"].append({"class": name, "type": "UGameInstanceSubsystem", "header": rel_path})
                    if "UWorldSubsystem" in stripped:
                        api["subsystems"].append({"class": name, "type": "UWorldSubsystem", "header": rel_path})
                    if "UEngineSubsystem" in stripped:
                        api["subsystems"].append({"class": name, "type": "UEngineSubsystem", "header": rel_path})
                    if "UActorComponent" in stripped:
                        api["components"].append({"class": name, "header": rel_path})

            if RE_ENUM.search(stripped) and not self._is_forward_decl(stripped):
                enum_name = self._parse_enum_decl(stripped)
                if enum_name:
                    symbols.append(
                        {
                            "kind": "ENUM",
                            "name": enum_name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {},
                            "line": i + 1,
                            "signature": stripped,
                            "exports": "",
                            "confidence": "low",
                        }
                    )

            if stripped.startswith("typedef "):
                match = re.search(r"typedef\s+.*\s+([A-Za-z_][A-Za-z0-9_]*)\s*;", stripped)
                if match:
                    name = match.group(1)
                    symbols.append(
                        {
                            "kind": "TYPEDEF",
                            "name": name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {},
                            "line": i + 1,
                            "signature": stripped,
                            "exports": "",
                            "confidence": "low",
                        }
                    )

            if stripped.startswith("using ") and "=" in stripped:
                match = re.search(r"using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=", stripped)
                if match:
                    name = match.group(1)
                    symbols.append(
                        {
                            "kind": "TYPEDEF",
                            "name": name,
                            "plugin": plugin,
                            "module": module,
                            "header": rel_path,
                            "source": "",
                            "macro_context": {},
                            "line": i + 1,
                            "signature": stripped,
                            "exports": "",
                            "confidence": "low",
                        }
                    )

            i += 1

        tag_payload = {
            "occurrences": {k: v for k, v in tag_occ.items()},
            "tags_seen": sorted(tags_seen),
            "tags_ini": [],
        }
        return symbols, api, tag_payload

    def _parse_ini_tags(self, text: str) -> List[str]:
        tags = set()
        for match in RE_TAG_INI.finditer(text):
            tags.add(match.group(1))
        return sorted(tags)

    def _parse_uplugin(self, text: str, plugin_name: str) -> Dict[str, object]:
        try:
            data = json.loads(text.lstrip("\ufeff"))
        except Exception:
            return {"type": "uplugin", "plugin": plugin_name, "modules": [], "dependencies": []}
        modules = [m.get("Name") for m in data.get("Modules", []) if m.get("Name")]
        deps = [p.get("Name") for p in data.get("Plugins", []) if p.get("Name")]
        return {"type": "uplugin", "plugin": plugin_name, "modules": sorted(modules), "dependencies": sorted(deps)}

    def _extract_deps_block(self, text: str, key: str) -> List[str]:
        deps: List[str] = []
        for match in re.finditer(key + r".*?;", text, re.DOTALL):
            deps.extend(re.findall(r"\"([A-Za-z0-9_]+)\"", match.group(0)))
        return sorted(set(deps))

    def _parse_build_cs(self, text: str, plugin: str, module: str) -> Dict[str, object]:
        public_deps = self._extract_deps_block(text, "PublicDependencyModuleNames")
        private_deps = self._extract_deps_block(text, "PrivateDependencyModuleNames")
        public_includes = self._extract_deps_block(text, "PublicIncludePaths")
        private_includes = self._extract_deps_block(text, "PrivateIncludePaths")
        return {
            "type": "build",
            "plugin": plugin,
            "module": module,
            "public_deps": public_deps,
            "private_deps": private_deps,
            "public_includes": public_includes,
            "private_includes": private_includes,
        }

    def _assign_source_paths(self, symbols: List[Dict[str, object]], all_files: set[str]) -> None:
        for entry in symbols:
            header = entry.get("header", "")
            if not header:
                continue
            if entry.get("source"):
                continue
            if header.endswith(".cpp"):
                entry["source"] = header
                continue
            base = Path(header)
            for ext in [".cpp", ".inl"]:
                candidate = base.with_suffix(ext).as_posix()
                if candidate in all_files:
                    entry["source"] = candidate
                    break

    def _write_json(self, path: Path, payload: object) -> None:
        path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

    def _build_module_graph(self, module_entries: Dict[str, Dict[str, object]]) -> Dict[str, object]:
        plugins: Dict[str, Dict[str, object]] = {}
        modules: Dict[str, Dict[str, object]] = {}
        edges: List[Dict[str, str]] = []

        for entry in module_entries.values():
            if entry.get("type") == "uplugin":
                name = entry.get("plugin", "")
                if not name:
                    continue
                plugins[name] = {
                    "name": name,
                    "modules": entry.get("modules", []),
                    "dependencies": entry.get("dependencies", []),
                }

        for entry in module_entries.values():
            if entry.get("type") != "build":
                continue
            module = entry.get("module", "")
            if not module:
                continue
            modules[module] = {
                "name": module,
                "plugin": entry.get("plugin", ""),
                "public_deps": entry.get("public_deps", []),
                "private_deps": entry.get("private_deps", []),
                "public_includes": entry.get("public_includes", []),
                "private_includes": entry.get("private_includes", []),
            }
            for dep in entry.get("public_deps", []):
                edges.append({"from": module, "to": dep, "visibility": "public"})
            for dep in entry.get("private_deps", []):
                edges.append({"from": module, "to": dep, "visibility": "private"})

        plugin_list = sorted(plugins.values(), key=lambda p: p["name"].lower())
        module_list = sorted(modules.values(), key=lambda m: m["name"].lower())
        edges = sorted(edges, key=lambda e: (e["from"].lower(), e["to"].lower(), e["visibility"]))
        return {"plugins": plugin_list, "modules": module_list, "dependency_edges": edges}

    def _combine_api(self, api_by_file: Dict[str, Dict[str, List[Dict[str, object]]]]) -> Dict[str, List[Dict[str, object]]]:
        combined = {"functions": [], "properties": [], "subsystems": [], "components": []}
        for payload in api_by_file.values():
            for key in combined.keys():
                combined[key].extend(payload.get(key, []))
        combined["functions"] = sorted(combined["functions"], key=lambda f: (f.get("owning_class", ""), f.get("function_name", "")))
        combined["properties"] = sorted(combined["properties"], key=lambda p: (p.get("owning_class", ""), p.get("property_name", "")))
        combined["subsystems"] = sorted(combined["subsystems"], key=lambda s: (s.get("class", ""), s.get("type", "")))
        combined["components"] = sorted(combined["components"], key=lambda c: c.get("class", ""))
        return combined

    def _combine_tags(self, tags_by_file: Dict[str, Dict[str, object]]) -> Dict[str, object]:
        tag_map: Dict[str, List[Dict[str, object]]] = defaultdict(list)
        tags_defined: set[str] = set()
        tags_seen: set[str] = set()

        for payload in tags_by_file.values():
            for tag, occ_list in payload.get("occurrences", {}).items():
                tag_map[tag].extend(occ_list)
            tags_seen.update(payload.get("tags_seen", []))
            tags_defined.update(payload.get("tags_ini", []))

        tags_missing = sorted(tags_seen - tags_defined)
        return {
            "tags": {k: v for k, v in sorted(tag_map.items(), key=lambda i: i[0].lower())},
            "tags_defined_in_ini": sorted(tags_defined),
            "tags_seen_in_code": sorted(tags_seen),
            "tags_missing_from_ini": tags_missing,
        }

    def _build_manifest_list(self, manifest: Dict[str, Dict[str, object]]) -> List[Dict[str, object]]:
        return [manifest[k] for k in sorted(manifest.keys())]

    def _build_refcounts(
        self,
        symbol_names: set[str],
        code_files: List[Path],
        cache: Dict[str, object],
        changed_files: set[str],
    ) -> Dict[str, int]:
        refcounts_by_file: Dict[str, Dict[str, int]] = cache.get("refcounts_by_file", {}) if isinstance(cache.get("refcounts_by_file"), dict) else {}
        cached_hash = cache.get("symbol_set_hash", "")
        symbol_hash = hashlib.sha1("\n".join(sorted(symbol_names)).encode("utf-8")).hexdigest()

        if cached_hash != symbol_hash:
            refcounts_by_file = {}
            files_to_scan = code_files
            self.log("Symbol set changed; recomputing reference counts for all code files.")
        else:
            files_to_scan = [p for p in code_files if self._rel_path(p) in changed_files or self._rel_path(p) not in refcounts_by_file]

        token_re = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
        for path in files_to_scan:
            rel = self._rel_path(path)
            text = self._read_text(path)
            counts: Dict[str, int] = {}
            for token in token_re.findall(text):
                if token in symbol_names:
                    counts[token] = counts.get(token, 0) + 1
            refcounts_by_file[rel] = counts

        cache["refcounts_by_file"] = refcounts_by_file
        cache["symbol_set_hash"] = symbol_hash

        total: Dict[str, int] = {}
        for counts in refcounts_by_file.values():
            for name, count in counts.items():
                total[name] = total.get(name, 0) + count
        return total

    def _write_report(
        self,
        symbol_map: List[Dict[str, object]],
        tag_usage: Dict[str, object],
        module_graph: Dict[str, object],
        refcounts: Dict[str, int],
        manifest: Dict[str, Dict[str, object]],
        cache_hits: int,
        rescans: int,
    ) -> None:
        kinds = Counter([s.get("kind", "") for s in symbol_map])
        lines: List[str] = []
        lines.append("Repo Index Report")
        lines.append(f"Generated: {dt.datetime.now().isoformat(timespec='seconds')}")
        lines.append(f"Project root: {self.project_root}")
        lines.append("")
        lines.append(f"Files scanned: {len(manifest)} (rescanned: {rescans}, cache hits: {cache_hits})")
        lines.append("Symbol counts by kind:")
        for key in sorted(kinds.keys()):
            lines.append(f"  {key}: {kinds[key]}")
        lines.append("")

        top_symbols = sorted(refcounts.items(), key=lambda i: i[1], reverse=True)[:20]
        lines.append("Top 20 symbols by reference count:")
        for name, count in top_symbols:
            lines.append(f"  {name}: {count}")
        if not top_symbols:
            lines.append("  (none)")
        lines.append("")

        missing_tags = tag_usage.get("tags_missing_from_ini", [])
        lines.append("Top 20 missing tags (seen in code, missing in ini):")
        for tag in missing_tags[:20]:
            lines.append(f"  {tag}")
        if not missing_tags:
            lines.append("  (none)")
        lines.append("")

        plugin_count = len(module_graph.get("plugins", []))
        module_count = len(module_graph.get("modules", []))
        edge_count = len(module_graph.get("dependency_edges", []))
        lines.append("Module dependency summary:")
        lines.append(f"  Plugins: {plugin_count}")
        lines.append(f"  Modules: {module_count}")
        lines.append(f"  Dependency edges: {edge_count}")

        report_path = self.reports_dir / REPORT_FILE
        report_path.write_text("\n".join(lines), encoding="utf-8")

    def _write_worklog(self, cache_hits: int, rescans: int, outputs: Dict[str, Path]) -> None:
        lines = []
        lines.append("# Repo Index Worklog")
        lines.append("")
        lines.append(f"- Generated: {dt.datetime.now().isoformat(timespec='seconds')}")
        lines.append(f"- Project root: {self.project_root}")
        lines.append(f"- Cache hits: {cache_hits}")
        lines.append(f"- Files rescanned: {rescans}")
        lines.append("- Outputs:")
        for key, path in outputs.items():
            lines.append(f"  - {key}: {path}")
        (self.reports_dir / WORKLOG_FILE).write_text("\n".join(lines), encoding="utf-8")
    def run(self) -> None:
        self._ensure_dirs()
        cache = {} if self.full else self._load_cache()
        cache_manifest = cache.get("file_manifest", {}) if isinstance(cache.get("file_manifest"), dict) else {}
        symbols_by_file = cache.get("symbols_by_file", {}) if isinstance(cache.get("symbols_by_file"), dict) else {}
        api_by_file = cache.get("api_by_file", {}) if isinstance(cache.get("api_by_file"), dict) else {}
        tags_by_file = cache.get("tags_by_file", {}) if isinstance(cache.get("tags_by_file"), dict) else {}
        module_by_file = cache.get("module_by_file", {}) if isinstance(cache.get("module_by_file"), dict) else {}

        plugins = self._collect_plugins()
        files = self._collect_files()
        manifest, changed_files, cache_hits = self._build_manifest(files, cache_manifest)
        changed_set = set(changed_files)
        rescans = len(changed_files)

        self.log(f"Plugins scanned: {len(plugins)}")
        self.log(f"Files enumerated: {len(files)}")
        self.log(f"Cache hits: {cache_hits}")
        self.log(f"Files rescanned: {rescans}")

        deleted = set(cache_manifest.keys()) - set(manifest.keys())
        for rel in deleted:
            symbols_by_file.pop(rel, None)
            api_by_file.pop(rel, None)
            tags_by_file.pop(rel, None)
            module_by_file.pop(rel, None)

        for rel in changed_files:
            path = self.project_root / rel
            category = manifest[rel]["category"]
            if category != "ini":
                continue
            tags = self._parse_ini_tags(self._read_text(path))
            tags_by_file[rel] = {
                "occurrences": {},
                "tags_seen": [],
                "tags_ini": tags,
            }

        ini_tags: List[str] = []
        for payload in tags_by_file.values():
            ini_tags.extend(payload.get("tags_ini", []))
        allow_roots = self._build_tag_root_allowlist(ini_tags)

        for rel in changed_files:
            path = self.project_root / rel
            category = manifest[rel]["category"]
            plugin, module = self._extract_plugin_module(rel)
            text = self._read_text(path)

            if category in {"header", "source"}:
                symbols, api_payload, tag_payload = self._parse_code_file(rel, text, plugin, module, allow_roots)
                symbols_by_file[rel] = symbols
                api_by_file[rel] = api_payload
                tags_by_file[rel] = tag_payload
            elif category == "uplugin":
                module_by_file[rel] = self._parse_uplugin(text, plugin or Path(rel).stem)
            elif category == "build":
                module_by_file[rel] = self._parse_build_cs(text, plugin, module)

        symbol_entries: List[Dict[str, object]] = []
        for payload in symbols_by_file.values():
            symbol_entries.extend(payload)

        all_files_set = set(manifest.keys())
        self._assign_source_paths(symbol_entries, all_files_set)
        symbol_entries = sorted(symbol_entries, key=lambda s: (s.get("name", ""), s.get("kind", "")))
        kinds = Counter([s.get("kind", "") for s in symbol_entries])
        self.log("Symbol counts by kind:")
        for key in sorted(kinds.keys()):
            self.log(f"  {key}: {kinds[key]}")

        public_api_map = self._combine_api(api_by_file)
        module_graph = self._build_module_graph(module_by_file)
        self.log("Module dependency summary:")
        self.log(f"  Plugins: {len(module_graph.get('plugins', []))}")
        self.log(f"  Modules: {len(module_graph.get('modules', []))}")
        self.log(f"  Dependency edges: {len(module_graph.get('dependency_edges', []))}")
        tag_usage = self._combine_tags(tags_by_file)
        file_manifest = self._build_manifest_list(manifest)

        symbol_names = {s.get("name", "") for s in symbol_entries if s.get("name", "")}
        code_files = [self.project_root / rel for rel, rec in manifest.items() if rec.get("category") in {"header", "source"}]
        refcounts = self._build_refcounts(symbol_names, code_files, cache, changed_set)

        outputs = {
            "symbol_map": self.reports_dir / OUTPUT_FILES["symbol_map"],
            "public_api_map": self.reports_dir / OUTPUT_FILES["public_api_map"],
            "module_graph": self.reports_dir / OUTPUT_FILES["module_graph"],
            "tag_usage": self.reports_dir / OUTPUT_FILES["tag_usage"],
            "file_manifest": self.reports_dir / OUTPUT_FILES["file_manifest"],
            "report": self.reports_dir / REPORT_FILE,
            "worklog": self.reports_dir / WORKLOG_FILE,
        }

        self._write_json(outputs["symbol_map"], symbol_entries)
        self._write_json(outputs["public_api_map"], public_api_map)
        self._write_json(outputs["module_graph"], module_graph)
        self._write_json(outputs["tag_usage"], tag_usage)
        self._write_json(outputs["file_manifest"], file_manifest)
        self._write_report(symbol_entries, tag_usage, module_graph, refcounts, manifest, cache_hits, rescans)
        self._write_worklog(cache_hits, rescans, outputs)

        cache["version"] = CACHE_VERSION
        cache["schema_version"] = SCHEMA_VERSION
        cache["file_manifest"] = manifest
        cache["symbols_by_file"] = symbols_by_file
        cache["api_by_file"] = api_by_file
        cache["tags_by_file"] = tags_by_file
        cache["module_by_file"] = module_by_file

        self._save_cache(cache)
        self._flush_log()

        self.log("Reports written:")
        for key, path in outputs.items():
            self.log(f"  {key}: {path}")
