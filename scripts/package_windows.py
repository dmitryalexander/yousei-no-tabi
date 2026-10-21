"""Package explicit clean-build inputs; never copy a player Run directory."""
from pathlib import Path
import argparse, json, shutil, tempfile, zipfile

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--port-archive',type=Path,required=True)
    p.add_argument('--dll',type=Path,action='append',default=[])
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.output.exists():p.error('Refusing to overwrite an existing package.')
    if args.exe.name!='yousei-no-tabi.exe':p.error('Use the newly built lighting-only executable.')
    with zipfile.ZipFile(args.port_archive) as archive:
        names=archive.namelist()
        if 'portVersion' not in names or any('chuchu' in n.lower() or n.startswith(('alt/','audio/')) for n in names):
            p.error('Port archive contains private test/game/mod resources.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        archive.write(args.exe,'yousei-no-tabi.exe')
        archive.write(args.port_archive,'soh.o2r')
        for dll in args.dll:
            if dll.suffix.lower()!='.dll':p.error('DLL inputs must have .dll extension.')
            archive.write(dll,dll.name)
        for item in (ROOT/'runtime').rglob('*'):
            if item.is_file():archive.write(item,item.relative_to(ROOT/'runtime').as_posix())
        for item in (ROOT/'soh/assets/yml').rglob('*'):
            if item.is_file():archive.write(item,'assets/'+item.relative_to(ROOT/'soh/assets/yml').as_posix())
        for name in ('README.md','docs/ASSET_PROVENANCE.md','docs/RELEASE_AUDIT.md','docs/UPSTREAM.md','docs/VALIDATION.md'):
            archive.write(ROOT/name,name)
        for item in (ROOT/'licenses').rglob('*'):
            if item.is_file():archive.write(item,item.relative_to(ROOT).as_posix())
        for component in ('libultraship','torch'):
            for item in (ROOT/component).rglob('*'):
                if item.is_file() and item.name.lower().startswith(('license','copying','copyright')):
                    archive.write(item,'licenses/'+item.relative_to(ROOT).as_posix())
    print(args.output)

if __name__=='__main__':main()
