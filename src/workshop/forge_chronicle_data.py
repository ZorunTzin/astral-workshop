"""Preserve forge save IDs; use a distinct Chronicle namespace for every forged recipe."""
from pathlib import Path
import csv,io,json
HERE=Path(__file__).parent
ALIASES={row:61000+row for row in range(220)}
def install(root):
 root=Path(root);words=json.loads((HERE/"FORGING.json").read_text(encoding="utf8"))
 table=root/"data/global/excel/runes.txt"
 with table.open(encoding="utf-8-sig",newline="") as f:reader=csv.DictReader(f,delimiter="\t");fields=reader.fieldnames;rows=list(reader)
 assert len(rows)==len(words)==220
 mapping=[]
 for w in words:
  candidates=[i for i,row in enumerate(rows) if (row["Name"]==w["sourceName"] or row["Name"]=="WsForgeVariant"+str(i)) and [row["Rune"+str(k)] for k in range(1,7) if row["Rune"+str(k)]]==w["runeCodes"]]
  if len(candidates)>1:candidates=[i for i in candidates if set(rows[i]["itype"+str(k)] for k in range(1,7) if rows[i]["itype"+str(k)])==set(w["includeTypes"])]
  assert len(candidates)==1,(w["name"],candidates)
  mapping.append(candidates[0])
 assert len(set(mapping))==220
 strings=root/"data/local/lng/strings/item-runes.json";translations=json.loads(strings.read_text(encoding="utf-8-sig"))
 for row_id,string_id in ALIASES.items():
  key="WsForgeVariant"+str(row_id);word=words[mapping.index(row_id)]
  assert not any(x["id"]==string_id and x["Key"]!=key for x in translations)
  entry=next((x for x in translations if x["Key"]==key),None)
  if entry is None:
   entry={"id":string_id,"Key":key};translations.append(entry)
  entry["id"]=string_id
  for lang in ("enUS","zhTW","deDE","esES","frFR","itIT","koKR","plPL","esMX","jaJP","ptBR","ruRU","zhCN"):entry[lang]=word["name"]+" ("+word["base"]+")"
  rows[row_id]["Name"]=key
 strings.write_text(json.dumps(translations,ensure_ascii=False,indent=2),encoding="utf8")
 assert len({row["Name"] for row in rows})==220
 assert all(row["complete"]=="0" for row in rows)
 buf=io.StringIO(newline="");writer=csv.DictWriter(buf,fieldnames=fields,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows)
 for prefix in ("","base/"):(root/("data/global/excel/"+prefix+"runes.txt")).write_text(buf.getvalue(),encoding="utf8",newline="")
 bindings=root/"workshop/forge_chronicle_ids.inc"
 if not bindings.parent.exists():bindings=HERE/"forge_chronicle_ids.inc"
 bindings.write_text("// Generated forge identity -> native Runes row; saved forge IDs are unchanged.\nconstexpr ForgeChronicleRow forgeChronicleRows[]={\n"+",\n".join("{"+str(row_id)+","+json.dumps(rows[row_id]["Name"])+"}" for row_id in mapping)+"\n};\n",encoding="utf8")
 print("Chronicle: 220 distinct recipe IDs; all native runewords remain disabled for socket crafting")
if __name__=="__main__":
 import sys
 install(sys.argv[1])
