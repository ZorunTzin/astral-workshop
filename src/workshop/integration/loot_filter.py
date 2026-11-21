"""Expose D2R native Loot Filter and Chronicle; retire the ESR overlay."""
from pathlib import Path
import json,re,shutil
HERE=Path(__file__).parent
LAYOUTS=["data/global/ui/layouts/"+p for p in ("pauselayouthd.json","pauselayoutgardenhd.json","controller/pauselayoutgardenhd.json")]
def install(root):
 root=Path(root)
 for rel in LAYOUTS[:2]:
  p=root/rel
  if not p.exists():continue
  text=re.sub(r"//[^\n]*","",p.read_text(encoding="utf-8-sig"))
  j=json.loads(re.sub(r",\s*([}\]])",r"\1",text))
  j["children"]=[c for c in j["children"] if c["name"] not in ("LootChron Separator","PauseTableExtra")]
  table=next(c for c in j["children"] if c["name"]=="PauseTable")
  table["children"]=[c for c in table["children"] if c["name"]!="Spacer"]
  table["fields"]["rowHeight"]=140
  table["fields"]["rect"]["y"]=-340
  p.write_text(json.dumps(j,indent=2)+"\n",encoding="utf8")
 p=root/LAYOUTS[2];p.parent.mkdir(parents=True,exist_ok=True)
 shutil.copy2(HERE/"native-panels/controller.json",p)
 config=root/"d2rloader/config/celestialrayone.loot-filter.toml"
 config.parent.mkdir(parents=True,exist_ok=True)
 text=config.read_text(encoding="utf-8-sig") if config.exists() else "[loot_filter]\nenabled = false\n"
 config.write_text(re.sub(r"(?m)^enabled\s*=\s*true", "enabled = false",text),encoding="utf8")
 dll=root/"d2rloader/plugins/d2rl-celestialrayone-loot-filter.dll"
 if dll.exists():
  retired=dll.with_suffix(".dll.disabled")
  if retired.exists():
   assert retired.read_bytes()==dll.read_bytes(),"Different retired plugin: preserve both before migration"
   dll.unlink()
  else:dll.rename(retired)
 print("Native Loot Filter and Chronicle menu entries installed; ESR disabled; profiles preserved.")
if __name__=="__main__":
 import sys
 install(sys.argv[1])
