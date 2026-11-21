"""Pack nine original painted icons into paired Assassin HD/low-end frames.

The original atlas is kept as input so regeneration never accumulates edits.
Only the nine replacement skills' normal/dim frames change; table IDs and
every other Assassin icon remain byte-for-byte intact.
"""
from pathlib import Path
import csv
import struct
from PIL import Image, ImageEnhance

HERE = Path(__file__).resolve().parent
SKILLS = {263:'afterimage',278:'death-mark',256:'gravity-trap',276:'overclock',265:'expose-weakness',257:'spiked-ground',266:'atmg',277:'queen-of-blades',275:'omnislash'}

def install(root):
    root = Path(root)
    excel = root / 'data/global/excel'
    def rows(name):
        with (excel / (name + '.txt')).open(encoding='utf-8-sig') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))
    skills = {int(r['*Id']): r for r in rows('skills') if r['*Id'].isdigit()}
    descs = {r['skilldesc']: r for r in rows('skilldesc')}
    frames = {int(descs[skills[s]['skilldesc']]['IconCel']): n for s, n in SKILLS.items()}
    assert len(frames) == 9
    relative = Path('data/hd/global/ui/spells/assassin')
    for source in (HERE / 'upstream' / relative).glob('*.sprite'):
        original = source.read_bytes()
        version, fw, width, height, _, count = struct.unpack_from('<HH4I', original, 4)
        assert original[:4] in (b'SPa1', b'SpA1') and version == 31
        assert width % count == 0 and len(original) == 40 + width * height * 4
        stride = width // count
        assert stride >= fw  # Native Assassin atlases keep two padding pixels per frame.
        packed = bytearray(original)
        changed = set()
        source_frame=int(descs[skills[275]['skilldesc']]['IconCel'])
        destination_frame=int(descs[skills[260]['skilldesc']]['IconCel'])
        for offset in (0,1):
            changed.add(destination_frame+offset)
            for y in range(height):
                a=40+(y*width+(source_frame+offset)*stride)*4
                b=40+(y*width+(destination_frame+offset)*stride)*4
                packed[b:b+fw*4]=original[a:a+fw*4]
        for frame, name in frames.items():
            with Image.open(HERE / 'icons' / 'assassin' / (name + '.png')) as image:
                normal = image.convert('RGBA').resize((fw, height), Image.Resampling.LANCZOS)
            for offset, art in enumerate((normal, ImageEnhance.Brightness(normal).enhance(0.55))):
                assert frame + offset < count
                changed.add(frame + offset)
                raw = art.tobytes()
                for y in range(height):
                    dest = 40 + (y * width + (frame + offset) * stride) * 4
                    packed[dest:dest + fw * 4] = raw[y * fw * 4:(y + 1) * fw * 4]
        # Protect the rest of the atlas, including all ordinary aura icons.
        for frame in set(range(count)) - changed:
            for y in range(height):
                start = 40 + (y * width + frame * stride) * 4
                assert packed[start:start + fw * 4] == original[start:start + fw * 4]
        target = root / relative / source.name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(packed)
    import json
    manifest = HERE / 'assets.json'
    assets = json.loads(manifest.read_text(encoding='utf8'))
    assets.extend((relative / name).as_posix() for name in ('asskillicon.sprite', 'asskillicon.lowend.sprite'))
    manifest.write_text(json.dumps(list(dict.fromkeys(assets)), indent=2), encoding='utf8')
    print('Packed nine distinct Assassin icons, paired display states, HD and low-end; other frames preserved.')

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
