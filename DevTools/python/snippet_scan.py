import pathlib, re, json

root = pathlib.Path('BEP_EXPORT_CGF_ParkourComp/BlueprintFlows/Snippets')
plugin = pathlib.Path('Plugins/SOTS_Parkour')

texts = []
for p in plugin.rglob('*.cpp'):
    texts.append(p.read_text(errors='ignore'))
for p in plugin.rglob('*.h'):
    texts.append(p.read_text(errors='ignore'))
blob = '\n'.join(texts)

prefixes = (
    'ParkourComponent_',
    'ParkourFunctionLibrary_',
    'ParkourABPInterface_',
    'ParkourInterface_',
    'ParkourStatsWidget_Interface_',
)
stop = {
    'input','output','return','true','false','event','function','call','branch','sequence','cast','set','get','target','self','index','array','bool','float','int','vector','rotator','timeline','macro','for','each','break','continue','loop','success','failure','valid','cast','casted','struct','class','enum','switch','case'
}

report = []
for f in sorted(root.iterdir()):
    stem = f.stem
    if not stem.startswith(prefixes):
        continue
    content = f.read_text(errors='ignore')
    cand = set(re.findall(r'[A-Za-z_][A-Za-z0-9_]{3,}', content))
    cand = {c for c in cand if c.lower() not in stop and len(c) > 3}
    hits = sorted([c for c in cand if c in blob])
    misses = sorted([c for c in cand if c not in blob])
    report.append({
        'snippet': stem,
        'hits': hits,
        'misses': misses,
        'hit_count': len(hits),
        'miss_count': len(misses),
    })

report.sort(key=lambda r: r['miss_count'], reverse=True)

out_path = pathlib.Path('WORKING/snippet_token_coverage.json')
out_path.parent.mkdir(parents=True, exist_ok=True)
out_path.write_text(json.dumps(report, indent=2))

print('Wrote', out_path)
print('Top 5 by misses:')
for item in report[:5]:
    print(f"{item['snippet']}: hits={item['hit_count']}, misses={item['miss_count']}")
