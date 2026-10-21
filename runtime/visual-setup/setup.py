"""Local visual setup worker. Assets are downloaded from authors, never shipped here."""
from pathlib import Path
import argparse, ctypes, hashlib, json, os, re, shutil, subprocess, sys, time, urllib.request, zipfile

HERE=Path(__file__).resolve().parent
MANIFEST=json.loads((HERE/'manifest.json').read_text(encoding='utf-8-sig'))
ALLOWED_HOSTS={'evilgames.eu','gamebanana.com','files.gamebanana.com'}

def report(message):
    print(json.dumps({'message':message}),flush=True)

def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def verified(path,item):
    return path.is_file() and path.stat().st_size==item['size'] and digest(path)==item['sha256']

def atomic_json(path,value):
    temp=path.with_name(path.name+'.new')
    temp.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
    os.replace(temp,path)

def reject_links(path):
    # Junctions/reparse points can redirect writes out of the selected installation.
    for p in (path,*path.parents):
        if p.exists() and getattr(p.lstat(),'st_file_attributes',0)&0x400:
            raise ValueError('Use a normal installation folder, without junctions or symbolic links.')

def game_stopped():
    if os.name!='nt':return
    # Reject every running lighting-only instance, including one launched after the wizard.
    class PROCESSENTRY32W(ctypes.Structure):
        _fields_=[('dwSize',ctypes.c_uint32),('cntUsage',ctypes.c_uint32),('th32ProcessID',ctypes.c_uint32),
                  ('th32DefaultHeapID',ctypes.c_size_t),('th32ModuleID',ctypes.c_uint32),('cntThreads',ctypes.c_uint32),
                  ('th32ParentProcessID',ctypes.c_uint32),('pcPriClassBase',ctypes.c_long),('dwFlags',ctypes.c_uint32),('szExeFile',ctypes.c_wchar*260)]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateToolhelp32Snapshot.restype=ctypes.c_void_p
    kernel.Process32FirstW.argtypes=[ctypes.c_void_p,ctypes.POINTER(PROCESSENTRY32W)]
    kernel.Process32NextW.argtypes=kernel.Process32FirstW.argtypes
    kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    handle=kernel.CreateToolhelp32Snapshot(2,0)
    if handle==ctypes.c_void_p(-1).value:raise OSError('Cannot check running games.')
    try:
        entry=PROCESSENTRY32W();entry.dwSize=ctypes.sizeof(entry)
        found=kernel.Process32FirstW(handle,ctypes.byref(entry))
        while found:
            if entry.szExeFile.lower()=='yousei-no-tabi.exe':raise ValueError('Save and close Yousei no Tabi, then try setup again.')
            found=kernel.Process32NextW(handle,ctypes.byref(entry))
    finally:kernel.CloseHandle(handle)

class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,req,fp,code,msg,headers,newurl):
        check_url(newurl)
        return super().redirect_request(req,fp,code,msg,headers,newurl)

def check_url(url):
    u=urllib.parse.urlsplit(url)
    distribution_host=u.hostname in ALLOWED_HOSTS or re.fullmatch(r'filecache[0-9]+\.gamebanana\.com',u.hostname or '')
    if u.scheme!='https' or not distribution_host or u.username or u.password:
        raise ValueError('The author redirected this download to an unapproved location. Use the author page instead.')

def obtain(item,cache,offline=False):
    target=cache/item['filename'];reject_links(target)
    if verified(target,item):
        report('Verified cached '+item['title']);return target
    if offline:raise ValueError('A verified offline download is missing: '+item['filename'])
    check_url(item['url']);part=target.with_suffix(target.suffix+'.partial')
    report('Downloading '+item['title'])
    try:
        request=urllib.request.Request(item['url'],headers={'User-Agent':'Yousei-no-Tabi-Visual-Setup/1.0'})
        with urllib.request.build_opener(SafeRedirect()).open(request,timeout=45) as response,part.open('wb') as output:
            check_url(response.url);received=0;last=time.monotonic()
            while chunk:=response.read(1024*1024):
                received+=len(chunk)
                if received>item['size']:raise ValueError('Download size changed; update the setup recipe.')
                output.write(chunk)
                if time.monotonic()-last>0.4:
                    report(f"{item['title']}: {received*100//item['size']}% ({received//1048576} MiB)");last=time.monotonic()
        report('Checking SHA-256: '+item['title'])
        if not verified(part,item):raise ValueError('Download checksum changed. No files installed. Please check the author page or update Yousei no Tabi.')
        os.replace(part,target)
        return target
    finally:
        if part.exists():part.unlink()

def extract_pack(download,item,dest):
    report('Installing '+item['title'])
    if any(Path(o['name']).name!=o['name'] or '\\' in o['name'] for o in item['outputs']):
        raise ValueError('Invalid output filename in setup recipe')
    if item['format']=='zip':
        with zipfile.ZipFile(download) as archive:
            for output in item['outputs']:
                # Extract ONLY exact selected names; no archive-controlled destination paths.
                matches=[i for i in archive.infolist() if i.filename.replace('\\','/').split('/')[-1]==output['name']]
                if len(matches)!=1 or matches[0].file_size!=output['size']:raise ValueError('Unexpected pack contents')
                with archive.open(matches[0]) as src,(dest/output['name']).open('xb') as dst:shutil.copyfileobj(src,dst,1024*1024)
    elif item['format']=='7z':
        tar=Path(os.environ.get('SystemRoot',r'C:\Windows'))/'System32/tar.exe'
        if not tar.is_file():raise ValueError('Windows tar.exe is unavailable. Update Windows, or import the extracted O2R from the author download.')
        for output in item['outputs']:
            with (dest/output['name']).open('xb') as stream:
                result=subprocess.run([str(tar),'-xOf',str(download),output['name']],stdout=stream,stderr=subprocess.PIPE,
                                      creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),timeout=300)
            if result.returncode:raise ValueError('Windows could not extract the 7z pack: '+result.stderr.decode(errors='replace')[:400])
    else:raise ValueError('Unsupported pack format')
    for output in item['outputs']:
        if Path(output['name']).name!=output['name'] or not verified(dest/output['name'],output):raise ValueError('Installed texture pack checksum mismatch')

def lookup(config,key):
    current=config
    for part in ('CVars.'+key).split('.'):
        if not isinstance(current,dict) or part not in current:return {'present':False}
        current=current[part]
    return {'present':True,'value':current}

def assign(config,key,value,present=True):
    parts=('CVars.'+key).split('.');current=config
    for part in parts[:-1]:
        if part not in current:current[part]={}
        if not isinstance(current[part],dict):raise ValueError('Unexpected settings format: '+key)
        current=current[part]
    if present:current[parts[-1]]=value
    else:current.pop(parts[-1],None)

def recover(root,work):
    pending=work/'pending.json'
    if not pending.exists():return
    report('Restoring interrupted setup')
    state=json.loads(pending.read_text());backup=work/'backups'/state['id']
    reject_links(backup)
    mods=root/'mods'
    # Move the interrupted generated directory aside, never delete player files.
    if (backup/'mods').exists():
        if mods.exists():mods.rename(backup/('interrupted-mods-'+str(time.time_ns())))
        (backup/'mods').rename(mods)
    elif state['had_mods'] is False and mods.exists():
        mods.rename(backup/('interrupted-mods-'+str(time.time_ns())))
    config=root/'yousei-no-tabi.json'
    if state['had_config']:shutil.copy2(backup/'config.json',config)
    elif config.exists():config.rename(backup/'interrupted-config.json')
    receipt=work/'installed.json'
    if state.get('previous_receipt') is not None:atomic_json(receipt,state['previous_receipt'])
    elif receipt.exists():receipt.unlink()
    pending.unlink()

def undo(root,work):
    game_stopped();recover(root,work)
    receipt=work/'installed.json'
    if not receipt.exists():raise ValueError('There is no completed visual setup to undo.')
    state=json.loads(receipt.read_text());backup=work/'backups'/state['id'];reject_links(backup)
    config_path=root/'yousei-no-tabi.json'
    config=json.loads(config_path.read_text(encoding='utf-8-sig')) if config_path.exists() else {}
    # Only revert owned settings. Later controller/audio/save options remain untouched.
    for key,value in state['previous'].items():assign(config,key,value.get('value'),value['present'])
    destination=backup/('undone-mods-'+str(time.time_ns()))
    if (root/'mods').exists():(root/'mods').rename(destination)
    try:
        if (backup/'mods').exists():(backup/'mods').rename(root/'mods')
        atomic_json(config_path,config)
    except BaseException:
        if (root/'mods').exists():(root/'mods').rename(backup/'mods')
        if destination.exists():destination.rename(root/'mods')
        raise
    if state.get('previous_receipt') is not None:atomic_json(receipt,state['previous_receipt'])
    else:receipt.unlink()
    report('Previous mods and visual settings restored. Saves and other settings preserved.')

def install(root,cache,offline=False):
    work=root/'.visual-setup';game_stopped();recover(root,work)
    config_path=root/'yousei-no-tabi.json'
    config=json.loads(config_path.read_text(encoding='utf-8-sig')) if config_path.exists() else {}
    # Honour a configured main archive folder, but never search unrelated/private directories.
    game=config.get('Game',{}).get('Main Archive',str(root))
    game=Path(game)
    if not game.is_absolute():game=root/game
    candidates=[game] if game.is_file() else [game/'oot.o2r',game/'oot-mq.o2r']
    game_archive=next((p for p in candidates if p.is_file()),None)
    if game_archive is None:raise ValueError('Run the game and extract your supported ROM first. No extracted game archive was found.')
    if config.get('Game',{}).get('Patches Archive',str(root/'mods')) not in (str(root/'mods'),'./mods','mods'):
        raise ValueError('A custom mod directory is configured. Restore the default mods directory before using this profile.')
    if shutil.disk_usage(root).free<12*1024**3:raise ValueError('Please free at least 12 GiB on the game drive before setup.')
    # Failed downloads, extraction, or repair generation occur before any player files change.
    ident=time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())+'-'+str(time.time_ns())
    stage=work/('stage-'+ident);stage.mkdir()
    try:
        for item in MANIFEST['packs']:
            # Older Windows tar versions may not support 7z. Accept the exact
            # extracted author O2R as a verified manual fallback, without Python packages.
            raw=cache/item['outputs'][0]['name']
            if item['format']=='7z' and verified(raw,item['outputs'][0]):
                report('Installing verified extracted '+item['title'])
                shutil.copy2(raw,stage/raw.name)
            else:extract_pack(obtain(item,cache,offline),item,stage)
        report('Building town and shop repairs from your local game data')
        result=subprocess.run([sys.executable,str(HERE/'build_backgrounds.py'),'--mods',str(stage),'--game-archive',str(game_archive),'--output',str(stage)],
                              capture_output=True,text=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),timeout=180)
        if result.returncode:raise ValueError('Background repair generation failed; no changes installed. '+result.stderr[-1200:])
        shutil.rmtree(stage/'geometry-work')
        # The blank shop dialogue resources are repaired using this player's original archive.
        with zipfile.ZipFile(game_archive) as base,zipfile.ZipFile(stage/'ZZZ - Kokiri Shop Dialogue Background Fix.o2r','x',zipfile.ZIP_DEFLATED) as patch:
            for name in ('textures/vr_KSVR_static/gKokiriShopBgTex','textures/vr_KSVR_static/gKokiriShop2BgTex',
                         'textures/vr_KSVR_pal_static/gKokiriShopBgTLUT','textures/vr_KSVR_pal_static/gKokiriShopBg2TLUT'):
                patch.writestr('alt/'+name,base.read(name))
        names=[Path(o['name']).stem for pack in MANIFEST['packs'] for o in pack['outputs']]
        names+=['ZZ-LightingLab-Architecture','ZZZ - Kokiri Shop Dialogue Background Fix']
        settings=dict(MANIFEST['settings']);settings['gSettings.EnabledMods']='|'.join(names)
        previous={k:lookup(config,k) for k in settings}
        for key,value in settings.items():assign(config,key,value)
        # Keep supplied author credits next to installed files.
        shutil.copy2(HERE/'CREDITS.txt',stage/'CREDITS.txt')
        shutil.copy2(HERE/'RELOADED-CREDITS.txt',stage/'RELOADED-CREDITS.txt')
        shutil.copy2(HERE/'MPYQ-LICENSE.txt',stage/'MPYQ-LICENSE.txt')
        game_stopped()
        backup=work/'backups'/ident;backup.mkdir(parents=True)
        mods=root/'mods';reject_links(mods);reject_links(config_path)
        receipt=work/'installed.json'
        state=dict(id=ident,had_mods=mods.exists(),had_config=config_path.exists(),previous=previous,
                   previous_receipt=json.loads(receipt.read_text()) if receipt.exists() else None)
        if state['had_config']:shutil.copy2(config_path,backup/'config.json')
        atomic_json(work/'pending.json',state)
        try:
            if mods.exists():mods.rename(backup/'mods')
            stage.rename(mods)
            atomic_json(config_path,config)
            atomic_json(receipt,state)
            (work/'pending.json').unlink()
        except BaseException:
            recover(root,work);raise
        report('Recommended visuals installed. You can start the game. Your previous mods and settings are backed up.')
    finally:
        if stage.exists():shutil.rmtree(stage)

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True)
    p.add_argument('--cache',type=Path);p.add_argument('--offline',action='store_true');p.add_argument('--undo',action='store_true')
    args=p.parse_args();root=args.root.absolute();reject_links(root)
    if not root.is_dir():raise ValueError('Game directory is missing')
    work=root/'.visual-setup';work.mkdir(exist_ok=True);reject_links(work)
    cache=(args.cache or work/'downloads').absolute();reject_links(cache);cache.mkdir(parents=True,exist_ok=True)
    # Exclusive lock is released by the OS even if the process is interrupted.
    import msvcrt
    with (work/'setup.lock').open('a+b') as lock:
        lock.seek(0);lock.write(b'0');lock.flush();lock.seek(0)
        try:msvcrt.locking(lock.fileno(),msvcrt.LK_NBLCK,1)
        except OSError:raise ValueError('Another visual setup is already running.')
        if args.undo:undo(root,work)
        else:install(root,cache,args.offline)

if __name__=='__main__':
    try:main()
    except Exception as error:
        report('Setup stopped: '+str(error));sys.exit(1)
