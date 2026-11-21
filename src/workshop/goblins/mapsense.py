"""Retarget the hash-guarded MapSense special icon classifier, not game code."""
from pathlib import Path
import hashlib,json,struct
from integration.prepare import replace,rva_offset
HERE=Path(__file__).resolve().parent.parent/'integration'
RVA=0x73977
GUARD=bytes.fromhex('81faef0200007734740983fa130f8538030000')
def marker_code(ids):
 assert len(ids)==9 and ids==list(range(ids[0],ids[0]+9))
 # Existing prologue preserves xmm3 in xmm2. Accept only Workshop IDs;
 # unsigned JA also rejects values below first. Fall through to gold icon;
 # the existing false epilogue lets ordinary monster markers render normally.
 code=b'\x81\xea'+struct.pack('<I',ids[0])+b'\x83\xfa\x08'
 code+=b'\x0f\x87'+struct.pack('<i',0x73cc2-(RVA+15))
 return code+b'\x90'*4

def install(root,ids):
 expected=json.loads((HERE/'UPSTREAM.json').read_text())
 name='d2rl-ruffneckk-mapsense.dll';raw=(HERE/'upstream'/name).read_bytes()
 assert hashlib.sha256(raw).hexdigest()==expected[name]
 raw=bytearray(replace(raw,b'FMMP-Experimental-R67\0',b'ReimaginedWorkshop\0'))
 off=rva_offset(raw,0x94983);assert raw[off:off+4]==bytes.fromhex('4883fb15');raw[off+3]=len('ReimaginedWorkshop')
 off=rva_offset(raw,RVA);assert raw[off:off+len(GUARD)]==GUARD
 raw[off:off+len(GUARD)]=marker_code(ids)
 target=Path(root)/'d2rloader/plugins'/name;target.write_bytes(raw)
 print('MapSense: special goblin icon restricted to Workshop IDs',ids[0],'-',ids[-1])
