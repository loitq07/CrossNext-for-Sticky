"""Validate embedded web pages after the firmware's include/minify pipeline."""
import ast
from html.parser import HTMLParser
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class Scripts(HTMLParser):
    def __init__(self):
        super().__init__()
        self.active = None
        self.scripts = []
    def handle_starttag(self, tag, attrs):
        if tag == 'script':
            self.active = ''
    def handle_data(self, data):
        if self.active is not None:
            self.active += data
    def handle_endtag(self, tag):
        if tag == 'script':
            self.scripts.append(self.active)
            self.active = None

class EmbeddedPagesTest(unittest.TestCase):
    def test_packed_scripts_are_complete_and_parseable(self):
        tree = ast.parse((ROOT / 'scripts/build_html.py').read_text(encoding='utf-8'))
        # Import helpers without running the generator's filesystem loop.
        definitions = [n for n in tree.body if isinstance(n, (ast.Import, ast.ImportFrom, ast.FunctionDef))]
        scope = {}
        exec(compile(ast.Module(body=definitions, type_ignores=[]), 'build_html.py', 'exec'), scope)
        node = shutil.which('node')
        self.assertIsNotNone(node, 'Node.js is required for JavaScript syntax validation')
        with tempfile.TemporaryDirectory() as directory:
            for name in ('FilesPage', 'SettingsPage', 'FontsPage'):
                with self.subTest(page=name):
                    source = ROOT / 'src/network/html' / (name + '.html')
                    html = scope['minify_html'](scope['resolve_includes'](source.read_text(encoding='utf-8'), str(source.parent)))
                    parser = Scripts()
                    parser.feed(html)
                    parser.close()
                    self.assertIsNone(parser.active, 'Unclosed script prevents page initialization')
                    self.assertTrue(any('toggleMobileSidebar' in s for s in parser.scripts))
                    self.assertTrue(html.endswith('</html>'))
                    for i, script in enumerate(parser.scripts):
                        path = Path(directory) / f'{name}-{i}.js'
                        path.write_text(script, encoding='utf-8')
                        result = subprocess.run([node, '--check', str(path)], capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)

if __name__ == '__main__':
    unittest.main()
