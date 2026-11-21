"""Replace obsolete passive-skill procs and reject future invalid proc targets."""
from pathlib import Path
import csv,io,json,re
PROC_CODES={'hit-skill','att-skill','gethit-skill','death-skill','kill-skill','levelup-skill'}
REPLACEMENTS={'Dim Vision':'Decrepify','Weaken':'Decrepify'}
def read(p):
 with p.open(encoding='utf-8-sig',newline='') as f:
  reader=csv.DictReader(f,delimiter='\t');return reader.fieldnames,list(reader)
def install(root,source=None):
 root=Path(root);source=Path(source) if source else root/'workshop'
 if not source.exists():source=Path(__file__).parent
 _,skills=read(root/'data/global/excel/skills.txt')
 by={r['skill'].casefold():r for r in skills};by.update({r['*Id']:r for r in skills})
 changes=[]
 def target(value,where):
  skill=by.get(str(value).casefold());assert skill is not None,(where,value)
  if skill.get('passive')=='1':
   replacement=REPLACEMENTS.get(skill['skill']);assert replacement,(where,'Unmapped passive proc',skill['skill'])
   active=by[replacement.casefold()];assert active.get('passive')!='1' and active.get('srvdofunc'),replacement
   changes.append((where,skill['skill'],replacement))
   return active['*Id'] if str(value).isdigit() else replacement
  return value
 for p in (root/'data/global/excel').rglob('*.txt'):
  if p.name=='properties.txt':continue
  fields,rows=read(p);dirty=False
  for i,row in enumerate(rows):
   for key,value in list(row.items()):
    if not isinstance(value,str) or value not in PROC_CODES:continue
    candidates={key.replace('code','param'),key.replace('Code','Param'),key.replace('prop','par'),key+' param',key.replace('Prop','ParMin'),key.replace('Prop','ParMax')}-{key}
    params=[k for k in candidates if k in row];assert params,(p,key,params)
    for k in params:
     v=target(row[k],f'{p.name}:{i}:{key}')
     if v!=row[k]:row[k]=v;dirty=True
  if dirty:
   buf=io.StringIO(newline='');w=csv.DictWriter(buf,fields,delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows);p.write_text(buf.getvalue(),encoding='utf8',newline='')
 words_path=source/'FORGING.json';words=json.loads(words_path.read_text(encoding='utf8'))
 for word in words:
  for effect in word['terms']:
   if effect['code'] in PROC_CODES:effect['param']=target(effect['param'],word['name'])
  # Human-readable legacy summaries must agree with the actual recipe terms.
  for key in ('original','magic','rare','unique','core'):
   for old,new in REPLACEMENTS.items():
    if any(e['code'] in PROC_CODES and e['param']==new for e in word['terms']):word[key]=word[key].replace(old,new)
 words_path.write_text(json.dumps(words,ensure_ascii=False,indent=2),encoding='utf8')
 _,props=read(root/'data/global/excel/properties.txt');proc_ids={int(r['*Id']) for r in props if r['code'] in PROC_CODES}
 def fix(effect):
  if effect[0] in proc_ids:effect[1]=int(target(str(effect[1]),'compiled forge'))
  return effect
 audit_path=source/'FORGING-audit.json';audit=json.loads(audit_path.read_text(encoding='utf8'))
 for row in audit:row['effects']=[fix(e) for e in row['effects']]
 audit_path.write_text(json.dumps(audit,ensure_ascii=False,indent=2),encoding='utf8')
 p=source/'forge.inc';s=p.read_text();a=s.index('constexpr Items::PropertySpec forgeEffects[]={');b=s.index('};',a)
 s=s[:a]+re.sub(r'\{(-?\d+),(-?\d+),(-?\d+),(-?\d+)\}',lambda m:'{'+','.join(map(str,fix(list(map(int,m.groups())))))+'}',s[a:b])+s[b:];p.write_text(s,encoding='utf8')
 print('Proc safety: all trigger targets checked;',len(changes),'passive references replaced')
if __name__=='__main__':
 import sys
 install(*sys.argv[1:])
