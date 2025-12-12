import argparse
import os
import sys
from datetime import datetime


def debug_print(msg: str) -> None:
    print(f"[pack_template_generator] {msg}")


def ensure_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def write_file(path: str, lines) -> None:
    with open(path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")


def make_template(name: str):
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    if name == "tag_audit":
        return [
            "[SOTS_DEVTOOLS]",
            "name: TagManager V2_11 – RequestGameplayTag scan",
            "tool: quick_search",
            "plugin: SOTS_TagManager",
            "category: tagmanager_audit",
            "mode: manual",
            "scope: Plugins",
            "wave: V2_TagManager_11",
            f"created: {ts}",
            'search: "RequestGameplayTag("',
            "exts: .cpp|.h",
            "[/SOTS_DEVTOOLS]",
            "",
            "Body: TagManager V2 audit template for RequestGameplayTag usage.",
        ]
    if name == "omnitrace_sweep":
        return [
            "[SOTS_DEVTOOLS]",
            "name: OmniTrace V2 sweep – usage scan",
            "tool: quick_search",
            "plugin: SOTS_OmniTrace",
            "category: omnitrace_sweep",
            "mode: manual",
            "scope: Plugins",
            "wave: V2_OmniTrace_01",
            f"created: {ts}",
            'search: "OmniTrace"',
            "exts: .cpp|.h",
            "[/SOTS_DEVTOOLS]",
            "",
            "Body: OmniTrace integration sweep template.",
        ]
    if name == "kem_execution_audit":
        return [
            "[SOTS_DEVTOOLS]",
            "name: KEM V2 – Execution audit sweep",
            "tool: quick_search",
            "plugin: SOTS_KillExecutionManager",
            "category: kem_audit",
            "mode: manual",
            "scope: Plugins/SOTS_KillExecutionManager",
            "wave: V2_KEM_01",
            f"created: {ts}",
            'search: "Execution"',
            "exts: .cpp|.h",
            "[/SOTS_DEVTOOLS]",
            "",
            "Body: KEM Execution audit template.",
        ]
    raise ValueError(f"Unknown template: {name}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Generate [SOTS_DEVTOOLS] pack templates.")
    parser.add_argument(
        "--template",
        required=True,
        choices=["tag_audit", "omnitrace_sweep", "kem_execution_audit"],
    )
    parser.add_argument("--output-dir", type=str, default="chatgpt_inbox")
    args = parser.parse_args(argv)

    debug_print("Starting pack_template_generator")
    debug_print(f"Template:   {args.template}")
    debug_print(f"Output dir: {args.output_dir}")

    ensure_dir(args.output_dir)
    ts_name = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{ts_name}_{args.template}.txt"
    path = os.path.join(args.output_dir, filename)
    lines = make_template(args.template)
    write_file(path, lines)
    debug_print(f"Template written to: {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())