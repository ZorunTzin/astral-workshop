"""Create a versioned, checksum-verified Workshop payload for GitHub Releases."""
import argparse, hashlib, json, pathlib, zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest().upper()
def package(version):
    out=ROOT/'build'/'launcher'/'release';out.mkdir(parents=True,exist_ok=True)
    entries=[]
    for folder in ['data','d2rloader']:
        for path in sorted((ROOT/folder).rglob('*')):
            if path.is_symlink(): raise ValueError(f'Linked payload: {path}')
            if not path.is_file() or path.suffix.lower() in {'.exp','.lib','.log','.pdb','.disabled'}: continue
            rel=path.relative_to(ROOT).as_posix()
            if folder=='data': rel='ReimaginedWorkshop.mpq/'+rel
            entries.append((path,rel))
    entries.extend([(ROOT/'modinfo.json','ReimaginedWorkshop.mpq/modinfo.json'),(ROOT/'bank.txt','bank.txt'),(ROOT/'LICENSE.txt','ReimaginedWorkshop.mpq/LICENSE.txt')])
    archive=out/'workshop-mod.zip'
    files=[]
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for path,rel in entries:
            z.write(path,rel);files.append({'Path':rel,'Sha256':digest(path)})
    manifest={'Version':version,'Archive':archive.name,'Sha256':digest(archive),'Files':files}
    (out/'workshop-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(f'Packaged {len(files)} files, {archive.stat().st_size/1048576:.1f} MB: {archive}',flush=True)
    source=out/'workshop-source.zip'
    folders=['src','tables','scripts','launcher','launcher-tests','launcher-setup','docs','data','d2rloader']
    with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for folder in folders:
            for path in sorted((ROOT/folder).rglob('*')):
                if not path.is_file() or path.is_symlink() or any(p in {'bin','obj','__pycache__','node_modules'} for p in path.relative_to(ROOT).parts):continue
                if path.suffix.lower() in {'.log','.bak','.pdb','.obj','.exp','.lib'}:continue
                z.write(path,path.relative_to(ROOT).as_posix())
        for name in ['LICENSE.txt','README.md','modinfo.json','bank.txt','.gitignore']:
            z.write(ROOT/name,name)
    print(f'Source archive: {source.stat().st_size/1048576:.1f} MB',flush=True)
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('version');package(parser.parse_args().version)
