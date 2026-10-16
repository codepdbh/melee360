"""Regression checks for the dependency slicer used by the XDK menu build."""
import importlib.util
from pathlib import Path
import unittest
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
spec = importlib.util.spec_from_file_location('menu_sources', ROOT/'tools/generate_menu_sources.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FunctionSlices(unittest.TestCase):
    def test_comments_literals_and_nested_blocks(self):
        source = '''// int bogus(void) { }
int first(void) {
    const char* text="} { \\\"";
    /* } */ if (text) { return '{'; }
    return 0;
}
static unsigned second(int x) { return x; }
'''
        found = module.functions(source)
        self.assertEqual(set(found), {'first', 'second'})
        self.assertTrue(found['first'][1].endswith('return 0;\n}'))
        self.assertEqual(source[found['second'][0]:], found['second'][1]+'\n')

    def test_unclosed_function_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unclosed function: broken'):
            module.functions('int broken(void) {')

    def test_original_menu_dependencies_present(self):
        base=ROOT/'upstream/melee-pc/src'
        gm=module.functions((base/'melee/gm/gm_1601.c').read_text(encoding='utf-8'))
        for name in ('gm_SetupAllPlayerDefaults','gm_SetupRulesDefaults','gm_SelKindToCKind','gm_80167BC8','gm_80167FC4'):
            self.assertIn(name, gm)
        css=module.functions((base/'melee/mn/mncharsel.c').read_text(encoding='utf-8'))
        for name in ('mnCharSel_Scene_OnEnter','mnCharSel_Scene_OnFrame','mnCharSel_Scene_OnExit'):
            self.assertIn(name, css)

    def test_reviewed_backports_match_the_pinned_sources(self):
        from apply_upstream_backports import MANIFEST,apply_backport
        import json
        for name,patch in json.loads(MANIFEST.read_text()).items():
            source=(ROOT/'upstream/melee-pc'/patch['source']).read_text(encoding='utf-8')
            adapted=apply_backport(name,source)
            for _,new in patch['edits']:
                self.assertIn(new,adapted)
            with self.assertRaises(ValueError):
                apply_backport(name,adapted)


if __name__ == '__main__':
    unittest.main()
