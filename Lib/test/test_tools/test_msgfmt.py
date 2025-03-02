"""Tests for the Tools/i18n/msgfmt.py tool."""
import filecmp
import os
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


def compile_messages(po_file, mo_file):
    assert_python_ok(msgfmt, '-o', mo_file, po_file)


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
                    compile_messages(po_file, tmp_mo_file)
                    with open(tmp_mo_file, 'rb') as f:
                        actual = GNUTranslations(f)

                    self.assertDictEqual(actual._catalog, expected._catalog)

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


class Test_multi_input(unittest.TestCase):
    """Tests for the issue https://github.com/python/cpython/issues/79516
        msgfmt.py shall accept multiple input files and when imported
        make shall be callable multiple times
    """

    script = os.path.join(toolsdir, 'i18n', 'msgfmt.py')

    def imp(self):
        i18ndir = os.path.join(toolsdir, 'i18n')
        sys.path.append(i18ndir)
        import msgfmt
        sys.path.remove(i18ndir)
        return msgfmt

    def test_help(self):
        """Test option -h"""
        rc, stdout, stderr = assert_python_ok(self.script, '-h')
        self.assertEqual(0, rc)
        self.assertTrue(stderr.startswith(
            b'Generate binary message catalog from textual'
            b' translation description.'
        ))

    def test_wrong(self):
        """Test wrong option"""
        rc, stdout, stderr = assert_python_failure(self.script, '-x')
        self.assertEqual(1, rc)
        self.assertTrue(stderr.startswith(
            b'Generate binary message catalog from textual'
            b' translation description.'
        ))

    def test_outputfile(self):
        """Test script with -o option - 1 single file, Windows EOL"""
        with temp_cwd(None):
            assert_python_ok(self.script, '-o', 'file1.mo',
                             data_dir / 'file1_fr.po')
            self.assertTrue(
                filecmp.cmp(data_dir / 'file1_fr.mo', 'file1.mo'),
                'Wrong compiled file1_fr.mo')

    def test_no_outputfile(self):
        """Test script without -o option - 1 single file, Unix EOL"""
        with temp_cwd(None):
            shutil.copy(data_dir / 'file2_fr.po', '.')
            assert_python_ok(self.script, 'file2_fr.po')
            self.assertTrue(
                filecmp.cmp(data_dir / 'file2_fr.mo', 'file2_fr.mo'),
                'Wrong compiled file2_fr.mo')

    def test_both_with_outputfile(self):
        """Test script with -o option and 2 input files"""
        # msgfmt.py version 1.2 behaviour is to correctly merge the input
        # files and to keep last entry when same entry occurs in more than
        # one file
        with temp_cwd(None):
            assert_python_ok(self.script, '-o', 'file12.mo',
                             data_dir / 'file1_fr.po',
                             data_dir / 'file2_fr.po')
            self.assertTrue(
                filecmp.cmp(data_dir / 'file12_fr.mo', 'file12.mo'),
                'Wrong compiled file12.mo')

    def test_both_without_outputfile(self):
        """Test script without -o option and 2 input files"""
        # msgfmt.py version 1.2 behaviour was that second mo file
        # also merged previous po files
        with temp_cwd(None):
            shutil.copy(data_dir /'file1_fr.po', '.')
            shutil.copy(data_dir /'file2_fr.po', '.')
            assert_python_ok(self.script, 'file1_fr.po', 'file2_fr.po')
            self.assertTrue(
                filecmp.cmp(data_dir / 'file1_fr.mo', 'file1_fr.mo'),
                'Wrong compiled file1_fr.mo')
            self.assertTrue(
                filecmp.cmp(data_dir / 'file2_fr.mo', 'file2_fr.mo'),
                'Wrong compiled file2_fr.mo')

    def test_consecutive_make_calls(self):
        """Directly calls make twice to prove bpo-9741 is fixed"""
        sys.path.append(os.path.join(toolsdir, 'i18n'))
        from msgfmt import make
        with temp_cwd(None):
            with open("file1_fr.po", "wb") as out:
                out.write(self.file1_fr_po)
            with open("file2_fr.po", "wb") as out:
                out.write(self.file2_fr_po)
            make("file1_fr.po", "file1_fr.mo")
            make("file2_fr.po", "file2_fr.mo")
            with open('file1_fr.mo', 'rb') as fin:
                self.assertEqual(self.file1_fr_mo, fin.read())
            with open('file2_fr.mo', 'rb') as fin:
                self.assertEqual(self.file2_fr_mo, fin.read())
        sys.path.pop()


def update_catalog_snapshots():
    for po_file in data_dir.glob('*.po'):
        mo_file = po_file.with_suffix('.mo')
        compile_messages(po_file, mo_file)


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_catalog_snapshots()
        sys.exit(0)
    unittest.main()
