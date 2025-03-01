"""Tests for the Tools/i18n/msgfmt.py tool."""

import os
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

    # binary contents of tiny po files
    # windows end of lines for first one
    file1_fr_po = b'''# French translations for python package.\r
# Copyright (C) 2018 THE python\'S COPYRIGHT HOLDER\r
# This file is distributed under the same license as the python package.\r
# s-ball <s-ball@laposte.net>, 2018.\r
#\r
msgid ""\r
msgstr ""\r
"Project-Id-Version: python 3.8\\n"\r
"Report-Msgid-Bugs-To: \\n"\r
"POT-Creation-Date: 2018-11-30 23:46+0100\\n"\r
"PO-Revision-Date: 2018-11-30 23:47+0100\\n"\r
"Last-Translator: s-ball <s-ball@laposte.net>\\n"\r
"Language-Team: French\\n"\r\n"Language: fr\\n"\r
"MIME-Version: 1.0\\n"\r
"Content-Type: text/plain; charset=UTF-8\\n"\r
"Content-Transfer-Encoding: 8bit\\n"\r
"Plural-Forms: nplurals=2; plural=(n > 1);\\n"\r
\r
#: file1.py:6\r
msgid "Hello!"\r
msgstr "Bonjour !"\r
\r
#: file1.py:7\r
#, python-brace-format\r
msgid "{n} horse"\r
msgid_plural "{n} horses"\r
msgstr[0] "{n} cheval"\r
msgstr[1] "{n} chevaux"\r
'''

    # Unix end of file and a non ascii character for second one
    file2_fr_po = rb'''# French translations for python package.
# Copyright (C) 2018 THE python'S COPYRIGHT HOLDER
# This file is distributed under the same license as the python package.
# s-ball <s-ball@laposte.net>, 2018.
#
msgid ""
msgstr ""
"Project-Id-Version: python 3.8\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: 2018-11-30 23:57+0100\n"
"PO-Revision-Date: 2018-11-30 23:57+0100\n"
"Last-Translator: s-ball <s-ball@laposte.net>\n"
"Language-Team: French\n"
"Language: fr\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n > 1);\n"

#: file2.py:6
msgid "It's over."
msgstr "C'est termin\xc3\xa9."

#: file2.py:7
msgid "Bye..."
msgstr "Au revoir ..."
'''

    # binary contents of corresponding compiled mo files
    file1_fr_mo = (
        b'\xde\x12\x04\x95\x00\x00\x00\x00\x03\x00\x00\x00\x1c'
        b'\x00\x00\x004\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        b'\x00\x00\x00\x00\x00L\x00\x00\x00\x06\x00\x00\x00M'
        b'\x00\x00\x00\x14\x00\x00\x00T\x00\x00\x00[\x01\x00'
        b'\x00i\x00\x00\x00\t\x00\x00\x00\xc5\x01\x00\x00\x16'
        b'\x00\x00\x00\xcf\x01\x00\x00\x00Hello!\x00{n} horse'
        b'\x00{n} horses\x00Project-Id-Version: python 3.8\n'
        b'Report-Msgid-Bugs-To: \nPOT-Creation-Date: 2018-11-30 '
        b'23:46+0100\nPO-Revision-Date: 2018-11-30 23:47+0100\n'
        b'Last-Translator: s-ball <s-ball@laposte.net>\n'
        b'Language-Team: French\nLanguage: fr\nMIME-Version: 1.0'
        b'\nContent-Type: text/plain; charset=UTF-8\nContent-'
        b'Transfer-Encoding: 8bit\nPlural-Forms: nplurals=2'
        b'; plural=(n > 1);\n\x00Bonjour !\x00{n} cheval\x00{n'
        b'} chevaux\x00'
    )
    file2_fr_mo = (
        b"\xde\x12\x04\x95\x00\x00\x00\x00\x03\x00\x00\x00"
        b"\x1c\x00\x00\x004\x00\x00\x00\x00\x00\x00\x00\x00"
        b"\x00\x00\x00\x00\x00\x00\x00L\x00\x00\x00\x06\x00"
        b"\x00\x00M\x00\x00\x00\n\x00\x00\x00T\x00\x00\x00["
        b"\x01\x00\x00_\x00\x00\x00\r\x00\x00\x00\xbb\x01"
        b"\x00\x00\x0f\x00\x00\x00\xc9\x01\x00\x00\x00Bye.."
        b".\x00It's over.\x00Project-Id-Version: python 3.8"
        b"\nReport-Msgid-Bugs-To: \nPOT-Creation-Date: 2018"
        b"-11-30 23:57+0100\nPO-Revision-Date: 2018-11-30 2"
        b"3:57+0100\nLast-Translator: s-ball <s-ball@lapost"
        b"e.net>\nLanguage-Team: French\nLanguage: fr\nMIME"
        b"-Version: 1.0\nContent-Type: text/plain; charset="
        b"UTF-8\nContent-Transfer-Encoding: 8bit\nPlural-Fo"
        b"rms: nplurals=2; plural=(n > 1);\n\x00Au revoir ."
        b"..\x00C'est termin\xc3\xa9.\x00"
    )

    # content of merging both po files keeping second header
    file12_fr_mo = (
        b"\xde\x12\x04\x95\x00\x00\x00\x00\x05\x00\x00\x00"
        b"\x1c\x00\x00\x00D\x00\x00\x00\x00\x00\x00\x00\x00"
        b"\x00\x00\x00\x00\x00\x00\x00l\x00\x00\x00\x06\x00"
        b"\x00\x00m\x00\x00\x00\x06\x00\x00\x00t\x00\x00\x00"
        b"\n\x00\x00\x00{\x00\x00\x00\x14\x00\x00\x00\x86"
        b"\x00\x00\x00[\x01\x00\x00\x9b\x00\x00\x00\r\x00"
        b"\x00\x00\xf7\x01\x00\x00\t\x00\x00\x00\x05\x02\x00"
        b"\x00\x0f\x00\x00\x00\x0f\x02\x00\x00\x16\x00\x00"
        b"\x00\x1f\x02\x00\x00\x00Bye...\x00Hello!\x00It's "
        b"over.\x00{n} horse\x00{n} horses\x00Project-Id-Ver"
        b"sion: python 3.8\nReport-Msgid-Bugs-To: \nPOT-Crea"
        b"tion-Date: 2018-11-30 23:57+0100\nPO-Revision-Date"
        b": 2018-11-30 23:57+0100\nLast-Translator: s-ball <"
        b"s-ball@laposte.net>\nLanguage-Team: French\nLangua"
        b"ge: fr\nMIME-Version: 1.0\nContent-Type: text/plai"
        b"n; charset=UTF-8\nContent-Transfer-Encoding: 8bit"
        b"\nPlural-Forms: nplurals=2; plural=(n > 1);\n\x00A"
        b"u revoir ...\x00Bonjour !\x00C'est termin\xc3\xa9."
        b"\x00{n} cheval\x00{n} chevaux\x00"
    )

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
            with open("file1_fr.po", "wb") as out:
                out.write(self.file1_fr_po)
            assert_python_ok(self.script, '-o', 'file1.mo', 'file1_fr.po')
            with open('file1.mo', 'rb') as fin:
                self.assertEqual(self.file1_fr_mo, fin.read())

    def test_no_outputfile(self):
        """Test script without -o option - 1 single file, Unix EOL"""
        with temp_cwd(None):
            with open("file2_fr.po", "wb") as out:
                out.write(self.file2_fr_po)
            assert_python_ok(self.script, 'file2_fr.po')
            with open('file2_fr.mo', 'rb') as fin:
                self.assertEqual(self.file2_fr_mo, fin.read())

    def test_both_with_outputfile(self):
        """Test script with -o option and 2 input files"""
        # msgfmt.py version 1.2 behaviour is to correctly merge the input
        # files and to keep last entry when same entry occurs in more than
        # one file
        with temp_cwd(None):
            with open("file1_fr.po", "wb") as out:
                out.write(self.file1_fr_po)
            with open("file2_fr.po", "wb") as out:
                out.write(self.file2_fr_po)
            assert_python_ok(self.script, '-o', 'file1.mo',
                             'file1_fr.po', 'file2_fr.po')
            with open('file1.mo', 'rb') as fin:
                self.assertEqual(self.file12_fr_mo, fin.read())

    def test_both_without_outputfile(self):
        """Test script without -o option and 2 input files"""
        # msgfmt.py version 1.2 behaviour was that second mo file
        # also merged previous po files
        with temp_cwd(None):
            with open("file1_fr.po", "wb") as out:
                out.write(self.file1_fr_po)
            with open("file2_fr.po", "wb") as out:
                out.write(self.file2_fr_po)
            assert_python_ok(self.script, 'file1_fr.po', 'file2_fr.po')
            with open('file1_fr.mo', 'rb') as fin:
                self.assertEqual(self.file1_fr_mo, fin.read())
            with open('file2_fr.mo', 'rb') as fin:
                self.assertEqual(self.file2_fr_mo, fin.read())

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
