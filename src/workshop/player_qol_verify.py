"""Release regressions: starter skill, standalone display and guarded death patch."""
from pathlib import Path
import csv, hashlib, json, struct
ROOT=Path(__file__).resolve().parents[2]

def verify(root):
 root=Path(root)
 for suffix in ('','base/'):
  with (root/('data/global/excel/'+suffix+'charstats.txt')).open(encoding='utf8',newline='') as f:rows=list(csv.DictReader(f,delimiter='\t'))
  sorc=next(r for r in rows if r['class']=='Sorceress')
  assert sorc['StartSkill'].lower()=='fire ball' and sorc['item1']=='ob1'
 raw=(root/'d2rloader/plugins/fmmp-monster-display.dll').read_bytes()
 assert '/mods/FMMP-Experimental-R67/'.encode('utf-16le') not in raw
 assert '/mods/ReimaginedWorkshop/bank.txt'.encode('utf-16le') in raw
 assert raw.count(hashlib.sha256((root/'bank.txt').read_bytes()).hexdigest().encode())==3
 # The loader/core identity gates must remain byte-for-byte the upstream hashes.
 source=(ROOT/'src/workshop/integration/upstream/fmmp-monster-display.dll').read_bytes()
 import re
 for h in re.findall(rb'[a-f0-9]{64}',source):
  if h!=b'2603713f63c44a8b80f3ea9102abea810dac020e72e0d73f6257dc36d3cbc554':assert h in raw
 patch=json.loads((root/'d2rloader/patches/workshop-keep-equipment.json').read_text())['patches']
 assert len(patch)==1 and patch[0]['op']=='bytes'
 p=patch[0];start=int(p['rva'],16);before=bytes.fromhex(p['expected']);after=bytes.fromhex(p['bytes'])
 assert start==0x42c240 and len(before)==len(after)==163
 for address,target,size in [(0x42c240,0x42c2a4,6),(0x42c2da,0x42c531,9)]:
  i=address-start
  assert after[i]==0xe9 and address+5+struct.unpack_from('<i',after,i+1)[0]==target
  assert after[i+5:i+size]==b'\x90'*(size-5)
 assert before[6:154]==after[6:154]
 print('PASS: Fire Ball starter; standalone monster display with native guards; atomic equipment/belt death patch')
if __name__=='__main__':
 import sys
 verify(sys.argv[1] if len(sys.argv)>1 else ROOT/'build/workshop-clusters')
