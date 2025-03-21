"""Tests for the Tools/i18n/msgfmt.py tool.

These tests use data files (po and mo) in the msgfmt_data folder.
The mo files can be generated (if the po file changes, or if msgfmt.py
slightly changes its output format) by using the --snapshot-update flag
with this script:

    python test_msgfmt.py --snapshot-update
"""

import filecmp
import json
import os.path
import shutil
import sys
import unittest
from gettext import GNUTranslations
from pathlib import Path

from test.support.os_helper import temp_cwd
from test.support.script_helper import assert_python_failure, assert_python_ok
from test.test_tools import skip_if_missing, toolsdir

skip_if_missing('i18n')

data_dir = (Path(__file__).parent / 'msgfmt_data').resolve()
script_dir = Path(toolsdir) / 'i18n'
msgfmt = script_dir / 'msgfmt.py'


def compile_messages(mo_file, *po_files):
    assert_python_ok(msgfmt, '-o', mo_file, *po_files)


class CompilationTest(unittest.TestCase):

    def test_compilation(self):
        self.maxDiff = None
        with temp_cwd():
            for po_file in data_dir.glob('*.po'):
                with self.subTest(po_file=po_file):
                    mo_file = po_file.with_suffix('.mo')
                    with open(mo_file, 'rb') as f:
                        expected = GNUTranslations(f)

                    tmp_mo_file = mo_file.name
                    compile_messages(tmp_mo_file, po_file)
                    with open(tmp_mo_file, 'rb') as f:
                        actual = GNUTranslations(f)

                    self.assertDictEqual(actual._catalog, expected._catalog)

    def test_translations(self):
        with open(data_dir / 'general.mo', 'rb') as f:
            t = GNUTranslations(f)

        self.assertEqual(t.gettext('foo'), 'foo')
        self.assertEqual(t.gettext('bar'), 'baz')
        self.assertEqual(t.pgettext('abc', 'foo'), 'bar')
        self.assertEqual(t.pgettext('xyz', 'foo'), 'bar')
        self.assertEqual(t.gettext('Multilinestring'), 'Multilinetranslation')
        self.assertEqual(t.gettext('"escapes"'), '"translated"')
        self.assertEqual(t.gettext('\n newlines \n'), '\n translated \n')
        self.assertEqual(t.ngettext('One email sent.', '%d emails sent.', 1),
                         'One email sent.')
        self.assertEqual(t.ngettext('One email sent.', '%d emails sent.', 2),
                         '%d emails sent.')
        self.assertEqual(t.npgettext('abc', 'One email sent.',
                                     '%d emails sent.', 1),
                         'One email sent.')
        self.assertEqual(t.npgettext('abc', 'One email sent.',
                                     '%d emails sent.', 2),
                         '%d emails sent.')

    def test_po_with_bom(self):
        with temp_cwd():
            Path('bom.po').write_bytes(b'\xef\xbb\xbfmsgid "Python"\nmsgstr "Pioton"\n')

            res = assert_python_failure(msgfmt, 'bom.po')
            err = res.err.decode('utf-8')
            self.assertIn('The file bom.po starts with a UTF-8 BOM', err)

    def test_invalid_msgid_plural(self):
        with temp_cwd():
            Path('invalid.po').write_text('''\
msgid_plural "plural"
msgstr[0] "singular"
''')

            res = assert_python_failure(msgfmt, 'invalid.po')
            err = res.err.decode('utf-8')
            self.assertIn('msgid_plural not preceded by msgid', err)

    def test_plural_without_msgid_plural(self):
        with temp_cwd():
            Path('invalid.po').write_text('''\
msgid "foo"
msgstr[0] "bar"
''')

            res = assert_python_failure(msgfmt, 'invalid.po')
            err = res.err.decode('utf-8')
            self.assertIn('plural without msgid_plural', err)

    def test_indexed_msgstr_without_msgid_plural(self):
        with temp_cwd():
            Path('invalid.po').write_text('''\
msgid "foo"
msgid_plural "foos"
msgstr "bar"
''')

            res = assert_python_failure(msgfmt, 'invalid.po')
            err = res.err.decode('utf-8')
            self.assertIn('indexed msgstr required for plural', err)

    def test_generic_syntax_error(self):
        with temp_cwd():
            Path('invalid.po').write_text('''\
"foo"
''')

            res = assert_python_failure(msgfmt, 'invalid.po')
            err = res.err.decode('utf-8')
            self.assertIn('Syntax error', err)


class CLITest(unittest.TestCase):

    def test_help(self):
        for option in ('--help', '-h'):
            res = assert_python_ok(msgfmt, option)
            err = res.err.decode('utf-8')
            self.assertIn('Generate binary message catalog from textual translation description.', err)

    def test_version(self):
        for option in ('--version', '-V'):
            res = assert_python_ok(msgfmt, option)
            out = res.out.decode('utf-8').strip()
            self.assertEqual('msgfmt.py 1.2', out)

    def test_invalid_option(self):
        res = assert_python_failure(msgfmt, '--invalid-option')
        err = res.err.decode('utf-8')
        self.assertIn('Generate binary message catalog from textual translation description.', err)
        self.assertIn('option --invalid-option not recognized', err)

    def test_no_input_file(self):
        res = assert_python_ok(msgfmt)
        err = res.err.decode('utf-8').replace('\r\n', '\n')
        self.assertIn('No input file given\n'
                      "Try `msgfmt --help' for more information.", err)

    def test_nonexistent_file(self):
        assert_python_failure(msgfmt, 'nonexistent.po')


class MultiInputTest(unittest.TestCase):
    """Tests for multiple input files
    """

    def test_both_with_outputfile(self):
        """Test script with -o option and 2 input files

        The current behaviour is to merge entries having distinct ids
        and keep last one if the same id occurs in multiple files.

        Here the first file has Windows endings (cflr) while second has
        Unix endings (lf)
        """
        with temp_cwd(None):
            assert_python_ok(msgfmt, '-o', 'file12.mo',
                             data_dir / 'file1_fr_crlf.po',
                             data_dir / 'file2_fr_lf.po')
            self.assertTrue(filecmp.cmp(data_dir / 'file12_fr.mo',
                                        'file12.mo'))

    def test_both_without_outputfile(self):
        """Test script without -o option and 2 input files"""

        with temp_cwd(None):
            shutil.copy(data_dir / 'file1_fr_crlf.po', '.')
            shutil.copy(data_dir / 'file2_fr_lf.po', '.')
            assert_python_ok(msgfmt, 'file1_fr_crlf.po', 'file2_fr_lf.po')
            self.assertTrue(filecmp.cmp(data_dir / 'file1_fr_crlf.mo',
                                        'file1_fr_crlf.mo'))
            self.assertTrue(filecmp.cmp(data_dir / 'file2_fr_lf.mo',
                                        'file2_fr_lf.mo'))


class PONamesTest(unittest.TestCase):
    def test_no_extension(self):
        with temp_cwd(None):
            shutil.copy(data_dir / 'file1_fr_crlf.po', 'file1.fr.po')
            assert_python_ok(msgfmt, 'file1.fr')
            self.assertTrue(os.path.exists('file1.fr.mo'))

    def test_wrong_extension(self):
        with temp_cwd(None):
            shutil.copy(data_dir / 'file1_fr_crlf.po', 'file1_fr.pox')
            assert_python_failure(msgfmt, 'file1_fr.pox')
            self.assertFalse(os.path.exists('file1_fr.mo'))
            self.assertFalse(os.path.exists('file1_fr.pox.mo'))

    @unittest.skipUnless(sys.platform.startswith("win"), "uppercase on Windows")
    def test_MAJ_on_Windows(self):
        with temp_cwd(None):
            shutil.copy(data_dir / 'file1_fr_crlf.po', 'File1.PO')
            assert_python_ok(msgfmt, 'FIle1.Po')
            self.assertTrue(os.path.exists('file1.mo'))


def make_message_files(mo_file, *po_files):
    compile_messages(mo_file, *po_files)
    # Create a human-readable JSON file which is
    # easier to review than the binary .mo file.
    with open(mo_file, 'rb') as f:
        translations = GNUTranslations(f)
    catalog_file = mo_file.with_suffix('.json')
    with open(catalog_file, 'w') as f:
        data = translations._catalog.items()
        data = sorted(data, key=lambda x: (isinstance(x[0], tuple), x[0]))
        json.dump(data, f, indent=4)
        f.write('\n')


def update_catalog_snapshots():
    for po_file in data_dir.glob('*.po'):
        mo_file = po_file.with_suffix('.mo')
        make_message_files(mo_file, po_file)
    # special processing for file12_fr.mo which results from 2 input files
    make_message_files(data_dir / 'file12_fr.mo',
                       data_dir / 'file1_fr_crlf.po',
                       data_dir / 'file2_fr_lf.po')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_catalog_snapshots()
        sys.exit(0)
    unittest.main()
