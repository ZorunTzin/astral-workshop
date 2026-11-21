"""Pack five original painted icons into paired Paladin HD/low-end frames.

The original atlas is kept as input so regeneration never accumulates edits.
Only the five replacement skills' normal/dim frames change; table IDs and
every other Paladin icon remain byte-for-byte intact.
"""
from pathlib import Path
import csv
import struct
from PIL import Image, ImageEnhance

HERE = Path(__file__).resolve().parent
SKILLS = {100: 'efficiency', 105: 'mastery', 110: 'guardian',
          103: 'retribution', 109: 'ground'}

def install(root):
    root = Path(root)
    excel = root / 'data/global/excel'
    def rows(name):
        with (excel / (name + '.txt')).open(encoding='utf-8-sig') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))
    skills = {int(r['*Id']): r for r in rows('skills') if r['*Id'].isdigit()}
    descs = {r['skilldesc']: r for r in rows('skilldesc')}
    frames = {int(descs[skills[s]['skilldesc']]['IconCel']): n for s, n in SKILLS.items()}
    assert len(frames) == 5
    relative = Path('data/hd/global/ui/spells/paladin')
    for source in (HERE / 'upstream' / relative).glob('*.sprite'):
        original = source.read_bytes()
        version, fw, width, height, _, count = struct.unpack_from('<HH4I', original, 4)
        assert original[:4] in (b'SPa1', b'SpA1') and version == 31
        assert width == fw * count and len(original) == 40 + width * height * 4
        packed = bytearray(original)
        changed = set()
        for frame, name in frames.items():
            with Image.open(HERE / 'icons' / (name + '.png')) as image:
                normal = image.convert('RGBA').resize((fw, height), Image.Resampling.LANCZOS)
            for offset, art in enumerate((normal, ImageEnhance.Brightness(normal).enhance(0.55))):
                assert frame + offset < count
                changed.add(frame + offset)
                raw = art.tobytes()
                for y in range(height):
                    dest = 40 + (y * width + (frame + offset) * fw) * 4
                    packed[dest:dest + fw * 4] = raw[y * fw * 4:(y + 1) * fw * 4]
        # Protect the rest of the atlas, including all ordinary aura icons.
        for frame in set(range(count)) - changed:
            for y in range(height):
                start = 40 + (y * width + frame * fw) * 4
                assert packed[start:start + fw * 4] == original[start:start + fw * 4]
        target = root / relative / source.name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(packed)
    print('Packed five distinct Paladin icons, paired display states, HD and low-end; other frames preserved.')

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
