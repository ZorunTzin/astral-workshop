"""Generate the C++ catalogue from the authoritative explicit tier values."""
import json
from pathlib import Path
def generate(root):
 p=Path(root);c=json.loads((p/'modifiers.json').read_text())
 h='#pragma once\nnamespace Arena {\nenum class Mod {'+','.join(m['key'] for m in c)+'};\ninline constexpr int ModCount='+str(len(c))+';\n'
 for n,k in [('Names','name'),('EffectNames','effect'),('Units','unit')]:h+='inline constexpr const char* '+n+'[]={'+','.join(json.dumps(m[k]) for m in c)+'};\n'
 for n,fn in [('Base',lambda m:m['base']),('Caps',lambda m:m['cap']),('Positive',lambda m:m['positive']),('EnabledMods',lambda m:m.get('enabled',True)),('Attributes',lambda m:m['kind']=='attribute')]:h+='inline constexpr int '+n+'[]={'+','.join(str(int(fn(m))) for m in c)+'};\n'
 h+='inline constexpr int TierValues[][5]={'+','.join('{'+','.join(map(str,m['tiers']))+'}' for m in c)+'};\n'
 h+='struct StatBonus{int mod,stat,mult;};\ninline constexpr StatBonus StatBonuses[]={'+','.join('{%d,%d,%d}'%(i,s,v) for i,m in enumerate(c) for s,v in m['stats'])+'};\n}\n';(p/'modifiers.h').write_text(h)
if __name__=='__main__':generate(Path(__file__).parent)
