import argparse
import re
import sys
from pathlib import Path
from datetime import datetime

PROJECT = Path(__file__).resolve().parent.parent.parent
DEFAULT_INI = PROJECT / 'Config' / 'DefaultGameplayTags.ini'
PLUGINS = PROJECT / 'Plugins'
BEP = PROJECT / 'BEP_EXPORTS'

now = datetime.now().strftime('%Y%m%d_%H%M')

pattern = re.compile(r'\b(?:SOTS|SAS|Input)\.[A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)+\b')
allowed_exts = {'.h', '.hpp', '.hh', '.cpp', '.cc', '.cxx', '.inl', '.ini', '.json', '.txt', '.csv', '.md'}
forbidden_suffixes = {'txt', 'json', 'csv', 'md', 'ini', 'h', 'hpp', 'cpp', 'cc', 'cxx', 'uasset', 'umap'}

for root in (DEFAULT_INI, PLUGINS, BEP):
    if not root.exists():
        print(f'[ERROR] Missing path: {root}')
        sys.exit(1)

parser = argparse.ArgumentParser(description='Scan for missing gameplay tags and emit patch/worklog.')
parser.add_argument('--apply-default', action='store_true', help='Append missing tags to Config/DefaultGameplayTags.ini with a marker header.')
args = parser.parse_args()

def read_text_safe(p: Path):
    try:
        return p.read_text(encoding='utf-8', errors='ignore')
    except Exception:
        return ''

def collect_used(paths):
    used = set()
    for root in paths:
        for p in root.rglob('*'):
            if not p.is_file():
                continue
            if p.suffix.lower() not in allowed_exts:
                continue
            txt = read_text_safe(p)
            for m in pattern.finditer(txt):
                tag = m.group(0)
                parts = tag.split('.')
                if parts and parts[-1].lower() in forbidden_suffixes:
                    continue
                used.add(tag)
    return used

def collect_existing(path: Path):
    existing = set()
    txt = read_text_safe(path)
    for m in re.finditer(r'Tag\s*=\s*"([^"]+)"', txt):
        existing.add(m.group(1).strip())
    return existing

used = collect_used([PLUGINS, BEP])
existing = collect_existing(DEFAULT_INI)
missing = sorted(t for t in used if t not in existing)

print(f'[STATS] used={len(used)} existing={len(existing)} missing={len(missing)}')

OUT_DIR = PROJECT / 'Plugins' / 'SOTS_TagManager' / 'Docs' / 'TagAudits'
OUT_DIR.mkdir(parents=True, exist_ok=True)
patch_path = OUT_DIR / f'MissingGameplayTags_Patch_{now}.ini'
with patch_path.open('w', encoding='utf-8') as f:
    for tag in missing:
        f.write(f'+GameplayTagList=(Tag="{tag}",DevComment="AUTO: missing (ref: scan)")\n')

WORKLOG_DIR = PROJECT / 'Plugins' / 'SOTS_TagManager' / 'Docs' / 'Worklogs'
WORKLOG_DIR.mkdir(parents=True, exist_ok=True)
worklog_path = WORKLOG_DIR / f'BuddyWorklog_{now}_MissingTagsAudit.md'
with worklog_path.open('w', encoding='utf-8') as f:
    f.write('# Missing GameplayTags Audit (BRIDGE missing-only)\n')
    f.write(f'- Timestamp: {now}\n')
    f.write('- Scanned: Plugins/** and BEP_EXPORTS/** for SOTS./SAS./Input.* tags\n')
    f.write(f'- Stats: Used={len(used)} Existing={len(existing)} Missing={len(missing)}\n')
    f.write(f'- Patch file: {patch_path} \n')
    f.write('\n## Missing Tags (add-only)\n')
    for tag in missing:
        f.write(f'+GameplayTagList=(Tag="{tag}",DevComment="AUTO: missing (ref: scan)")\n')

print(f'[OUT] patch: {patch_path}')
print(f'[OUT] worklog: {worklog_path}')

if args.apply_default and missing:
    marker = [
        '',
        '; === Missing GameplayTags Auto-Append ===',
        f'; Generated: {now}',
        f'; Source: {patch_path.name}',
    ]
    with DEFAULT_INI.open('a', encoding='utf-8') as f:
        f.write('\n'.join(marker) + '\n')
        for tag in missing:
            f.write(f'+GameplayTagList=(Tag="{tag}",DevComment="AUTO: missing (ref: scan)")\n')
    print(f'[APPLY] appended {len(missing)} tags to {DEFAULT_INI} (bottom of file)')
elif args.apply_default and not missing:
    print('[APPLY] no missing tags; DefaultGameplayTags.ini left unchanged')
