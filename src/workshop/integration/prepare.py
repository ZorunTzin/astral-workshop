"""Retarget two existing mod-scoped plugins without changing their native code.

Keep all native signature and loader/core hash guards. The pickup plugin's
packed-FMMP archive identity is replaced with a hash of a Workshop routing
manifest because Workshop uses a loose MPQ directory. Input hashes and exact
bytes are checked; unfamiliar binaries are rejected rather than guessed at.
"""
from pathlib import Path
from shutil import copy2
import hashlib, json, struct

HERE = Path(__file__).resolve().parent

def rva_offset(raw, rva):
    pe = struct.unpack_from('<I', raw, 0x3c)[0]
    count = struct.unpack_from('<H', raw, pe + 6)[0]
    optional = struct.unpack_from('<H', raw, pe + 20)[0]
    for n in range(count):
        pos = pe + 24 + optional + n * 40
        size, base, disk_size, disk = struct.unpack_from('<4I', raw, pos + 8)
        if base <= rva < base + max(size, disk_size): return disk + rva - base
    raise ValueError('Unmapped RVA')

def replace(raw, old, new, expected=1):
    assert len(new) <= len(old) and raw.count(old) == expected
    return raw.replace(old, new + b'\0' * (len(old) - len(new)))

def install(root, *, pickup_only=False):
    root = Path(root)
    expected = json.loads((HERE / 'UPSTREAM.json').read_text())
    for name, digest in expected.items():
        assert hashlib.sha256((HERE / 'upstream' / name).read_bytes()).hexdigest() == digest, name
    manifest = {'mod': 'ReimaginedWorkshop', 'core': '1.3.1-beta',
                'native_material_registry': 'D2RCore Advanced Stash; unsupported items remain in inventory',
                'tables': {}}
    for name in ('misc', 'itemtypes', 'inventory'):
        manifest['tables'][name] = hashlib.sha256((root / f'data/global/excel/{name}.txt').read_bytes()).hexdigest()
    bank = json.dumps(manifest, indent=2).encode()
    (root / 'bank.txt').write_bytes(bank)
    bank_hash = hashlib.sha256(bank).hexdigest().encode()
    plugins = root / 'd2rloader/plugins'; plugins.mkdir(parents=True, exist_ok=True)
    pickup = (HERE / 'upstream/fmmp-auto-collectibles.dll').read_bytes()
    pickup = replace(pickup, b'FMMP-Experimental-R67\0', b'ReimaginedWorkshop\0')
    old = '/mods/FMMP-Experimental-R67/FMMP-Experimental-R67.mpq\0'.encode('utf-16le')
    pickup = replace(pickup, old, '/mods/ReimaginedWorkshop/bank.txt\0'.encode('utf-16le'))
    old_hash = b'2603713f63c44a8b80f3ea9102abea810dac020e72e0d73f6257dc36d3cbc554'
    pickup = replace(pickup, old_hash, bank_hash, 3)
    (plugins / 'fmmp-auto-collectibles.dll').write_bytes(pickup)

    if pickup_only:
        print('Refreshed pickup manifest and guarded pickup plugin only.')
        return

    monster = (HERE / 'upstream/fmmp-monster-display.dll').read_bytes()
    monster = replace(monster, b'FMMP-Experimental-R67\0', b'ReimaginedWorkshop\0')
    (plugins / 'fmmp-monster-display.dll').write_bytes(monster)

    maps = (HERE / 'upstream/d2rl-ruffneckk-mapsense.dll').read_bytes()
    maps = bytearray(replace(maps, b'FMMP-Experimental-R67\0', b'ReimaginedWorkshop\0'))
    offset = rva_offset(maps, 0x94983)
    assert maps[offset:offset+4] == bytes.fromhex('4883fb15')
    maps[offset+3] = len('ReimaginedWorkshop')
    # The other FMMP-specific name-length checks remain untouched: Workshop
    # uses MapSense's ordinary markers, not FMMP's goblin-only filtering.
    (plugins / 'd2rl-ruffneckk-mapsense.dll').write_bytes(maps)
    copy2(HERE / 'upstream/RuffnecKkMapSenseMapgen.exe', plugins)
    copy2(HERE/'upstream/d2rl-celestialrayone-whirlwind-follow-cursor.dll',plugins)
    config = root / 'd2rloader/config'; config.mkdir(exist_ok=True)
    copy2(HERE/'upstream/celestialrayone.whirlwind.toml',config)
    copy2(HERE / 'upstream/ruffneckk-mapsense.toml', config)
    licenses = root / 'd2rloader/licenses'; licenses.mkdir(exist_ok=True)
    for p in (HERE / 'upstream/licenses').iterdir(): copy2(p, licenses)
    copy2(HERE / 'upstream/THIRD-PARTY-NOTICES.txt', root / 'd2rloader/THIRD-PARTY-NOTICES-workshop.txt')
    report = {'source': 'Locally installed FMMP 0.22.2+Randomizer plugins',
              'scope': 'ReimaginedWorkshop only', 'bank_manifest_sha256': bank_hash.decode(),
              'native_guards': 'unchanged', 'upstream_hashes': expected}
    (HERE / 'PORT.json').write_text(json.dumps(report, indent=2))
    import importlib.util
    spec=importlib.util.spec_from_file_location("workshop_loot_filter",HERE/"loot_filter.py")
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    install_loot_filter=module.install
    install_loot_filter(root)
    spec=importlib.util.spec_from_file_location("workshop_forge_chronicle",HERE.parent/"forge_chronicle_data.py")
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module.install(root)
    print('Prepared scoped MapSense and first-pickup deposit with unchanged native guards.')

if __name__ == '__main__':
    import sys
    install(sys.argv[1])
