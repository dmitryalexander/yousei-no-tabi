"""Reject accidentally bundled game data, private content and build artifacts."""
from pathlib import Path
import argparse, hashlib, json, re, sys, zipfile
from concurrent.futures import ThreadPoolExecutor

BAD_SUFFIXES={'.z64','.n64','.v64','.nes','.sfc','.gba','.nds','.otr','.sav','.sra','.pdb','.mp3','.ogg','.wav'}
BAD_DIRS={'.git','mods','mods-unused','learning-data','save','saves','cache','logs','records','checkpoints'}
SECRETS=re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|\bgh[pousr]_[A-Za-z0-9]{30,}\b|\bgithub_pat_[A-Za-z0-9_]{40,}\b')
# StormLib intentionally embeds Blizzard's publicly documented weak MPQ signing
# key. Allow only the byte-exact pinned upstream file, never a general key match.
PUBLIC_KEY_FIXTURES={'torch/lib/StormLib/src/SFileVerify.cpp':'4bb7f69607f671e9bceebc2807187d1d8069996f86d57c3b6bf92d8033f2bb09'}
PUBLIC_MPQ_KEY_SHA256='ef1fd1f89eeca070622ee7f57ca4fa00c884494e851316aa7df6713f8bca1013'

def has_secret(name,data):
    if PUBLIC_KEY_FIXTURES.get(name)==hashlib.sha256(data.replace(b'\r\n',b'\n')).hexdigest():return False
    # The same public MPQ signing key is embedded in a linked StormLib binary.
    # Mask only that exact PEM value; continue scanning the rest of the file.
    private_header=b'-----BEGIN '+b'PRIVATE KEY-----'
    for match in re.finditer(re.escape(private_header),data):
        end=data.find(b'-----END PRIVATE KEY-----',match.start())
        if end<0:continue
        pem=data[match.start():end+len(b'-----END PRIVATE KEY-----')]
        if hashlib.sha256(pem.replace(b'\r',b'').replace(b'\n',b'')).hexdigest()==PUBLIC_MPQ_KEY_SHA256:
            data=data.replace(pem,b'')
    return bool(SECRETS.search(data))

def check(name,data,source):
    p=Path(name)
    parts={x.lower() for x in p.parts}
    bad=[]
    if parts & BAD_DIRS:bad.append('private/runtime directory')
    if p.suffix.lower() in BAD_SUFFIXES:bad.append('excluded asset/artifact type')
    if p.suffix.lower()=='.o2r' and (source or p.name!='soh.o2r'):bad.append('game/archive data')
    if source and p.suffix.lower() in {'.exe','.dll','.lib','.a','.obj'}:bad.append('compiled artifact')
    if p.name in {'ChuchuMesh.inc','dialogue.json','japanese-learning.json','learning-build.marker'}:bad.append('private edition content')
    if len(data)>50*1024*1024:bad.append('file over 50 MiB')
    if has_secret(name,data):bad.append('possible credential/private key')
    return [{'path':name,'reason':x} for x in bad]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('target',type=Path)
    parser.add_argument('--package',action='store_true')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    findings=[];manifest=[]
    if args.target.is_file():
        with zipfile.ZipFile(args.target) as archive:
            entries=[(i.filename,archive.read(i)) for i in archive.infolist() if not i.is_dir()]
    else:
        paths=[]
        for path in args.target.rglob('*'):
            relative=path.relative_to(args.target)
            if not path.is_file() or '.git' in relative.parts or '__pycache__' in relative.parts:continue
            paths.append(path)
        def read(path):return (path.relative_to(args.target).as_posix(),path.read_bytes())
        with ThreadPoolExecutor(max_workers=8) as pool:
            entries=list(pool.map(read,paths))
    for name,data in entries:
        findings.extend(check(name,data,not args.package))
        manifest.append({'path':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    result={'passed':not findings,'files':len(manifest),'bytes':sum(x['bytes'] for x in manifest),'findings':findings,'manifest':manifest}
    if args.output:args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({k:result[k] for k in ('passed','files','bytes','findings')},indent=2))
    return 0 if result['passed'] else 1

if __name__=='__main__':sys.exit(main())
