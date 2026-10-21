"""Transaction and preservation regressions; no ROM or third-party packs needed."""
from pathlib import Path
import importlib.util,json,tempfile,unittest
from unittest.mock import patch

SOURCE=Path(__file__).resolve().parents[1]/'runtime/visual-setup/setup.py'
spec=importlib.util.spec_from_file_location('visual_setup',SOURCE)
setup=importlib.util.module_from_spec(spec);spec.loader.exec_module(setup)

class SetupTests(unittest.TestCase):
    def test_profile_only_changes_owned_settings(self):
        data={'CVars':{'gSettings':{'Controller':'keep','GraphicsTargetFPS':120},'gEnhancements':{'Autosave':1}}}
        for key,value in setup.MANIFEST['settings'].items():setup.assign(data,key,value)
        self.assertEqual(data['CVars']['gSettings']['Controller'],'keep')
        self.assertEqual(data['CVars']['gSettings']['GraphicsTargetFPS'],120)
        self.assertEqual(data['CVars']['gEnhancements']['Autosave'],1)
        self.assertNotIn('NativeTest',data['CVars']['gEnhancements']['LightingLab'])

    def test_interruption_restores_mods_config_and_receipt(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);work=root/'.visual-setup';backup=work/'backups'/'first';backup.mkdir(parents=True)
            (backup/'mods').mkdir();(backup/'mods/old.otr').write_bytes(b'old')
            (root/'mods').mkdir();(root/'mods/new.otr').write_bytes(b'new')
            original={'CVars':{'controller':'keep'}}
            setup.atomic_json(backup/'config.json',original)
            setup.atomic_json(root/'yousei-no-tabi.json',{'changed':True})
            setup.atomic_json(work/'installed.json',{'id':'first'})
            setup.atomic_json(work/'pending.json',dict(id='first',had_mods=True,had_config=True,previous_receipt=None))
            setup.recover(root,work)
            self.assertEqual((root/'mods/old.otr').read_bytes(),b'old')
            self.assertEqual(json.loads((root/'yousei-no-tabi.json').read_text()),original)
            self.assertFalse((work/'installed.json').exists())
            self.assertFalse((work/'pending.json').exists())

    def test_undo_preserves_later_controller_changes_and_saves(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);work=root/'.visual-setup';backup=work/'backups'/'first';backup.mkdir(parents=True)
            (backup/'mods').mkdir();(backup/'mods/original.otr').write_bytes(b'prior mod')
            (root/'mods').mkdir();(root/'mods/installed.otr').write_bytes(b'new')
            (root/'Save').mkdir();(root/'Save/player.sav').write_bytes(b'never touch')
            setup.atomic_json(root/'yousei-no-tabi.json',{'CVars':{'gSettings':{'Controller':'later','AltAssets':1}}})
            setup.atomic_json(work/'installed.json',dict(id='first',previous={'gSettings.AltAssets':{'present':True,'value':0}},previous_receipt=None))
            with patch.object(setup,'game_stopped'):setup.undo(root,work)
            data=json.loads((root/'yousei-no-tabi.json').read_text())
            self.assertEqual(data['CVars']['gSettings'],{'Controller':'later','AltAssets':0})
            self.assertEqual((root/'Save/player.sav').read_bytes(),b'never touch')
            self.assertTrue((root/'mods/original.otr').exists())

    def test_wrong_download_rejected_before_installation(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);p=root/'pack.zip';p.write_bytes(b'bad')
            item=dict(filename='pack.zip',size=3,sha256='0'*64,title='fixture')
            with self.assertRaises(ValueError):setup.obtain(item,root,offline=True)

    def test_download_redirect_allowlist(self):
        for url in ('http://evilgames.eu/file','https://evilgames.eu.attacker.test/file','https://user@evilgames.eu/file','file:///etc/passwd'):
            with self.assertRaises(ValueError):setup.check_url(url)
        setup.check_url('https://gamebanana.com/dl/1810733')
        setup.check_url('https://filecache44.gamebanana.com/mods/file.zip')
        with self.assertRaises(ValueError):setup.check_url('https://filecache44.gamebanana.com.attacker.test/mods/file.zip')

    def test_duplicate_zip_names_are_rejected(self):
        import zipfile,hashlib
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);dest=root/'out';dest.mkdir()
            archive=root/'source.zip'
            with zipfile.ZipFile(archive,'w') as z:
                z.writestr('../fixture.otr',b'abc');z.writestr('folder/fixture.otr',b'abc')
            item=dict(title='fixture',format='zip',outputs=[dict(name='fixture.otr',size=3,sha256=hashlib.sha256(b'abc').hexdigest())])
            with self.assertRaises(ValueError):setup.extract_pack(archive,item,dest)
            self.assertFalse((root/'fixture.otr').exists())

if __name__=='__main__':unittest.main()
