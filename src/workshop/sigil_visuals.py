"""Give the unique Sigil an explicit, correctly sized HD inventory asset."""
from pathlib import Path
import json, struct
from PIL import Image
from combat.port import read, write
from item_visuals import add_mappings

def install(root):
    root = Path(root)
    source = Path(__file__).resolve().parent / 'integration/upstream/sigil'
    assets = []
    for suffix in ('.sprite', '.lowend.sprite'):
        raw = (source / ('relic_of_the_ancients' + suffix)).read_bytes()
        _, fw, width, height, _, count = struct.unpack_from('<HH4I', raw, 4)
        assert count == 1 and fw == width and len(raw) == 40 + width * height * 4
        # Preserve the original artifact's proportions and alpha; fit its full
        # three-slot artwork inside the Sigil's existing one-slot footprint.
        original = Image.frombytes('RGBA', (width, height), raw[40:])
        original.thumbnail((width, width), Image.Resampling.LANCZOS)
        canvas = Image.new('RGBA', (width, width))
        canvas.alpha_composite(original, ((width-original.width)//2, (width-original.height)//2))
        header = bytearray(raw[:40]); struct.pack_into('<I', header, 12, width)
        relative = 'data/hd/global/ui/items/misc/uniquecharm/workshop_astral_sigil' + suffix
        target = root / relative; target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(header + canvas.tobytes()); assets.append(relative)
    relative = 'data/hd/items/misc/uniquecharm/workshop_astral_sigil.json'
    target = root / relative; target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes((source / 'relic_of_the_ancients.json').read_bytes()); assets.append(relative)
    add_mappings(root / 'data/hd/items/uniques.json', {
        'workshop_astral_sigil': dict.fromkeys(('normal', 'uber', 'ultra'), 'uniquecharm/workshop_astral_sigil')})
    for folder in ('data/global/excel', 'data/global/excel/base'):
        p = root / folder
        h, rows = read(p / 'uniqueitems.txt')
        sigil = next(r for r in rows if r['index'] == 'WorkshopAstralSigil')
        sigil['invfile'] = 'workshop_astral_sigil'
        write(p / 'uniqueitems.txt', h, rows)
        h, rows = read(p / 'misc.txt')
        next(r for r in rows if r['code'] == 'Wsig')['uniqueinvfile'] = 'workshop_astral_sigil'
        write(p / 'misc.txt', h, rows)
    manifest = root / 'workshop/combat/assets.json'
    current = json.loads(manifest.read_text())
    manifest.write_text(json.dumps(list(dict.fromkeys(current + assets)), indent=2))

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
