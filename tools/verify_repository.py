"""Check documentation links and archive newline integrity using only stdlib."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import unittest

from historical_integrity import archive_form

ROOT = Path(__file__).resolve().parents[1]


class RepositoryChecks(unittest.TestCase):
    def test_documentation_links(self):
        for document in [ROOT / 'README.md', *sorted((ROOT / 'docs').glob('*.md'))]:
            for target in re.findall(r'\]\(([^)]+)\)', document.read_text(encoding='utf-8')):
                if target.startswith(('https://', 'http://', '#')):
                    continue
                with self.subTest(document=document.name, target=target):
                    self.assertTrue((document.parent / target.split('#')[0]).is_file())

    def test_readme_identity_and_required_resource(self):
        self.assertTrue((ROOT / 'README.md').read_text(encoding='utf-8').startswith(
            '# CS2 Trade-Up Engine\n**Author:** Rareș Constantinescu\n'))
        self.assertTrue((ROOT / 'src/historical_cells.inc').is_file())

    def test_archive_newline_forms_and_tampering(self):
        entries = json.loads((ROOT / 'tests/fixtures/historical_manifest.json').read_text(encoding='utf-8'))['files']
        for entry in entries:
            if not entry['tracked']:
                continue
            with self.subTest(path=entry['path']):
                raw = (ROOT / entry['path']).read_bytes()
                archived = archive_form(raw, entry)
                self.assertEqual(hashlib.sha256(archived).hexdigest(), entry['sha256'])
                self.assertEqual(archive_form(archived, entry, allow_git_lf=False), archived)
                with self.assertRaises(ValueError):
                    archive_form(raw + b'X', entry)
                if Path(entry['path']).suffix in ('.cpp', '.txt'):
                    lf = archived.replace(b'\r\n', b'\n')
                    self.assertEqual(archive_form(lf, entry), archived)
                    if lf != archived:
                        with self.assertRaises(ValueError):
                            archive_form(lf, entry, allow_git_lf=False)
                        mixed = archived.replace(b'\r\n', b'\n', 1)
                        with self.assertRaises(ValueError):
                            archive_form(mixed, entry)
        # An optional archive is never granted Git newline conversion privileges.
        data = b'optional\r\n'
        optional = {'path': 'historical/optional.txt', 'tracked': False,
                    'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        with self.assertRaises(ValueError):
            archive_form(data.replace(b'\r\n', b'\n'), optional)


if __name__ == '__main__':
    unittest.main(verbosity=2)
