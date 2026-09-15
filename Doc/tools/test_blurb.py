"""Regression tests for the documentation build's private blurb backport.

Run after ``make venv`` with the virtual environment's Python:
    python tools/test_blurb.py [path/to/patched/blurb.py]
"""

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


BLURB_FILE = Path(__file__).resolve().parents[1] / 'venv' / 'blurb.py'


class BlurbTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location('patched_blurb', str(BLURB_FILE))
        cls.blurb = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.blurb)

    def test_next_filenames(self):
        for kind in ('bpo', 'gh-issue'):
            with self.subTest(kind=kind):
                filename = 'Security/2025-05-09-20-22-54.{}-133767.nonce.rst'.format(kind)
                metadata = self.blurb.Blurbs._parse_next_filename(filename)
                self.assertEqual(metadata[kind], '133767')
                self.assertEqual(metadata['section'], 'Security')
                entries = self.blurb.Blurbs()
                entries.parse('Fix a security issue.', metadata=metadata)
                self.assertEqual(entries[0][0][kind], '133767')

    def test_invalid_issue_number(self):
        for kind in ('bpo', 'gh-issue'):
            for value in ('invalid', None):
                with self.subTest(kind=kind, value=value):
                    entries = self.blurb.Blurbs()
                    with self.assertRaises(self.blurb.BlurbError):
                        entries.parse('Fix an issue.',
                                      metadata={'section': 'Library', kind: value})

    def test_merge_preserves_issue_trackers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            security = root / 'Security'
            security.mkdir()
            fragments = {
                security / '2025-05-09.gh-issue-133767.abc.rst': 'GitHub fix.',
                security / '2020-05-09.bpo-12345.def.rst': 'Roundup fix.',
                security / '2020-05-08.bpo-0.ghi.rst': 'Unnumbered fix.',
            }
            release = root / '3.6.15.rst'
            fragments[release] = (
                '.. release date: 2021-09-04\n'
                '.. section: Library\n.. bpo: 23456\n\nHistorical fix.\n'
                '\n..\n\n.. section: Library\n'
                '.. bpo: 34567\n.. gh-issue: 123456\n\nMigrated fix.\n'
            )
            for filename, text in fragments.items():
                filename.write_text(text, encoding='utf-8')
            files = {'next': [str(f) for f in fragments if f != release],
                     '3.6.15': [str(release)]}
            with mock.patch.object(self.blurb, 'original_dir', directory), \
                 mock.patch.object(self.blurb, 'glob_versions', return_value=['next', '3.6.15']), \
                 mock.patch.object(self.blurb, 'glob_blurbs', side_effect=files.__getitem__):
                self.blurb.merge('NEWS', forced=True)
            news = (root / 'NEWS').read_text(encoding='utf-8')
            for entry in ('gh-issue-133767: GitHub fix.',
                          'bpo-12345: Roundup fix.',
                          '- Unnumbered fix.',
                          'bpo-23456: Historical fix.',
                          'gh-issue-123456: Migrated fix.'):
                self.assertIn(entry, news)
            self.assertNotIn('bpo-0:', news)
            self.assertNotIn('bpo-34567:', news)
            for filename, text in fragments.items():
                self.assertEqual(filename.read_text(encoding='utf-8'), text)


if __name__ == '__main__':
    if len(sys.argv) > 1 and not sys.argv[1].startswith('-'):
        BLURB_FILE = Path(sys.argv.pop(1)).resolve()
    unittest.main()
