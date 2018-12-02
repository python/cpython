"""Tests to cover the Tools/i18n package"""

import os
import sys
import unittest
from textwrap import dedent

from test.support.script_helper import assert_python_ok, assert_python_failure
from test.test_tools import skip_if_missing, toolsdir
from test.support import temp_cwd, temp_dir, unload, change_cwd


skip_if_missing()


class Test_pygettext(unittest.TestCase):
    """Tests for the pygettext.py tool"""

    script = os.path.join(toolsdir,'i18n', 'pygettext.py')

    def get_header(self, data):
        """ utility: return the header of a .po file as a dictionary """
        headers = {}
        for line in data.split('\n'):
            if not line or line.startswith(('#', 'msgid','msgstr')):
                continue
            line = line.strip('"')
            key, val = line.split(':',1)
            headers[key] = val.strip()
        return headers

    def get_msgids(self, data):
        """ utility: return all msgids in .po file as a list of strings """
        msgids = []
        reading_msgid = False
        cur_msgid = []
        for line in data.split('\n'):
            if reading_msgid:
                if line.startswith('"'):
                    cur_msgid.append(line.strip('"'))
                else:
                    msgids.append('\n'.join(cur_msgid))
                    cur_msgid = []
                    reading_msgid = False
                    continue
            if line.startswith('msgid '):
                line = line[len('msgid '):]
                cur_msgid.append(line.strip('"'))
                reading_msgid = True
        else:
            if reading_msgid:
                msgids.append('\n'.join(cur_msgid))

        return msgids

    def extract_docstrings_from_str(self, module_content):
        """ utility: return all msgids extracted from module_content """
        filename = 'test_docstrings.py'
        with temp_cwd(None) as cwd:
            with open(filename, 'w') as fp:
                fp.write(module_content)
            assert_python_ok(self.script, '-D', filename)
            with open('messages.pot') as fp:
                data = fp.read()
        return self.get_msgids(data)

    def test_header(self):
        """Make sure the required fields are in the header, according to:
           http://www.gnu.org/software/gettext/manual/gettext.html#Header-Entry
        """
        with temp_cwd(None) as cwd:
            assert_python_ok(self.script)
            with open('messages.pot') as fp:
                data = fp.read()
            header = self.get_header(data)

            self.assertIn("Project-Id-Version", header)
            self.assertIn("POT-Creation-Date", header)
            self.assertIn("PO-Revision-Date", header)
            self.assertIn("Last-Translator", header)
            self.assertIn("Language-Team", header)
            self.assertIn("MIME-Version", header)
            self.assertIn("Content-Type", header)
            self.assertIn("Content-Transfer-Encoding", header)
            self.assertIn("Generated-By", header)

            # not clear if these should be required in POT (template) files
            #self.assertIn("Report-Msgid-Bugs-To", header)
            #self.assertIn("Language", header)

            #"Plural-Forms" is optional

    @unittest.skipIf(sys.platform.startswith('aix'),
                     'bpo-29972: broken test on AIX')
    def test_POT_Creation_Date(self):
        """ Match the date format from xgettext for POT-Creation-Date """
        from datetime import datetime
        with temp_cwd(None) as cwd:
            assert_python_ok(self.script)
            with open('messages.pot') as fp:
                data = fp.read()
            header = self.get_header(data)
            creationDate = header['POT-Creation-Date']

            # peel off the escaped newline at the end of string
            if creationDate.endswith('\\n'):
                creationDate = creationDate[:-len('\\n')]

            # This will raise if the date format does not exactly match.
            datetime.strptime(creationDate, '%Y-%m-%d %H:%M%z')

    def test_funcdocstring(self):
        for doc in ('"""doc"""', "r'''doc'''", "R'doc'", 'u"doc"'):
            with self.subTest(doc):
                msgids = self.extract_docstrings_from_str(dedent('''\
                def foo(bar):
                    %s
                ''' % doc))
                self.assertIn('doc', msgids)

    def test_funcdocstring_bytes(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo(bar):
            b"""doc"""
        '''))
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_funcdocstring_fstring(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo(bar):
            f"""doc"""
        '''))
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_classdocstring(self):
        for doc in ('"""doc"""', "r'''doc'''", "R'doc'", 'u"doc"'):
            with self.subTest(doc):
                msgids = self.extract_docstrings_from_str(dedent('''\
                class C:
                    %s
                ''' % doc))
                self.assertIn('doc', msgids)

    def test_classdocstring_bytes(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        class C:
            b"""doc"""
        '''))
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_classdocstring_fstring(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        class C:
            f"""doc"""
        '''))
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_msgid(self):
        msgids = self.extract_docstrings_from_str(
                '''_("""doc""" r'str' u"ing")''')
        self.assertIn('docstring', msgids)

    def test_msgid_bytes(self):
        msgids = self.extract_docstrings_from_str('_(b"""doc""")')
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_msgid_fstring(self):
        msgids = self.extract_docstrings_from_str('_(f"""doc""")')
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_funcdocstring_annotated_args(self):
        """ Test docstrings for functions with annotated args """
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo(bar: str):
            """doc"""
        '''))
        self.assertIn('doc', msgids)

    def test_funcdocstring_annotated_return(self):
        """ Test docstrings for functions with annotated return type """
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo(bar) -> str:
            """doc"""
        '''))
        self.assertIn('doc', msgids)

    def test_funcdocstring_defvalue_args(self):
        """ Test docstring for functions with default arg values """
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo(bar=()):
            """doc"""
        '''))
        self.assertIn('doc', msgids)

    def test_funcdocstring_multiple_funcs(self):
        """ Test docstring extraction for multiple functions combining
        annotated args, annotated return types and default arg values
        """
        msgids = self.extract_docstrings_from_str(dedent('''\
        def foo1(bar: tuple=()) -> str:
            """doc1"""

        def foo2(bar: List[1:2]) -> (lambda x: x):
            """doc2"""

        def foo3(bar: 'func'=lambda x: x) -> {1: 2}:
            """doc3"""
        '''))
        self.assertIn('doc1', msgids)
        self.assertIn('doc2', msgids)
        self.assertIn('doc3', msgids)

    def test_classdocstring_early_colon(self):
        """ Test docstring extraction for a class with colons occuring within
        the parentheses.
        """
        msgids = self.extract_docstrings_from_str(dedent('''\
        class D(L[1:2], F({1: 2}), metaclass=M(lambda x: x)):
            """doc"""
        '''))
        self.assertIn('doc', msgids)

    def test_files_list(self):
        """Make sure the directories are inspected for source files
           bpo-31920
        """
        text1 = 'Text to translate1'
        text2 = 'Text to translate2'
        text3 = 'Text to ignore'
        with temp_cwd(None), temp_dir(None) as sdir:
            os.mkdir(os.path.join(sdir, 'pypkg'))
            with open(os.path.join(sdir, 'pypkg', 'pymod.py'), 'w') as sfile:
                sfile.write(f'_({text1!r})')
            os.mkdir(os.path.join(sdir, 'pkg.py'))
            with open(os.path.join(sdir, 'pkg.py', 'pymod2.py'), 'w') as sfile:
                sfile.write(f'_({text2!r})')
            os.mkdir(os.path.join(sdir, 'CVS'))
            with open(os.path.join(sdir, 'CVS', 'pymod3.py'), 'w') as sfile:
                sfile.write(f'_({text3!r})')
            assert_python_ok(self.script, sdir)
            with open('messages.pot') as fp:
                data = fp.read()
            self.assertIn(f'msgid "{text1}"', data)
            self.assertIn(f'msgid "{text2}"', data)
            self.assertNotIn(text3, data)


class Test_msgfmt(unittest.TestCase):
    """Tests for the msgfmt.py tool
        bpo-35335 - bpo-9741
    """

    script = os.path.join(toolsdir,'i18n', 'msgfmt.py')

    # binary images of tiny po files
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

    # binary images of corresponding compiled mo files
    file1_fr_mo = b'\xde\x12\x04\x95\x00\x00\x00\x00\x03\x00\x00\x00\x1c' \
                  b'\x00\x00\x004\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00L\x00\x00\x00\x06\x00\x00\x00M' \
                  b'\x00\x00\x00\x14\x00\x00\x00T\x00\x00\x00[\x01\x00' \
                  b'\x00i\x00\x00\x00\t\x00\x00\x00\xc5\x01\x00\x00\x16' \
                  b'\x00\x00\x00\xcf\x01\x00\x00\x00Hello!\x00{n} horse' \
                  b'\x00{n} horses\x00Project-Id-Version: python 3.8\n' \
                  b'Report-Msgid-Bugs-To: \nPOT-Creation-Date: 2018-11-30 ' \
                  b'23:46+0100\nPO-Revision-Date: 2018-11-30 23:47+0100\n'\
                  b'Last-Translator: s-ball <s-ball@laposte.net>\n' \
                  b'Language-Team: French\nLanguage: fr\nMIME-Version: 1.0' \
                  b'\nContent-Type: text/plain; charset=UTF-8\nContent-' \
                  b'Transfer-Encoding: 8bit\nPlural-Forms: nplurals=2' \
                  b'; plural=(n > 1);\n\x00Bonjour !\x00{n} cheval\x00{n' \
                  b'} chevaux\x00'
    file2_fr_mo = b"\xde\x12\x04\x95\x00\x00\x00\x00\x03\x00\x00\x00" \
                  b"\x1c\x00\x00\x004\x00\x00\x00\x00\x00\x00\x00\x00" \
                  b"\x00\x00\x00\x00\x00\x00\x00L\x00\x00\x00\x06\x00" \
                  b"\x00\x00M\x00\x00\x00\n\x00\x00\x00T\x00\x00\x00[" \
                  b"\x01\x00\x00_\x00\x00\x00\r\x00\x00\x00\xbb\x01" \
                  b"\x00\x00\x0f\x00\x00\x00\xc9\x01\x00\x00\x00Bye.." \
                  b".\x00It's over.\x00Project-Id-Version: python 3.8" \
                  b"\nReport-Msgid-Bugs-To: \nPOT-Creation-Date: 2018" \
                  b"-11-30 23:57+0100\nPO-Revision-Date: 2018-11-30 2" \
                  b"3:57+0100\nLast-Translator: s-ball <s-ball@lapost" \
                  b"e.net>\nLanguage-Team: French\nLanguage: fr\nMIME" \
                  b"-Version: 1.0\nContent-Type: text/plain; charset=" \
                  b"UTF-8\nContent-Transfer-Encoding: 8bit\nPlural-Fo" \
                  b"rms: nplurals=2; plural=(n > 1);\n\x00Au revoir ." \
                  b"..\x00C'est termin\xc3\xa9.\x00"

    # image of merging both po files keeping second header
    file12_fr_mo = b"\xde\x12\x04\x95\x00\x00\x00\x00\x05\x00\x00\x00" \
                  b"\x1c\x00\x00\x00D\x00\x00\x00\x00\x00\x00\x00\x00" \
                  b"\x00\x00\x00\x00\x00\x00\x00l\x00\x00\x00\x06\x00" \
                  b"\x00\x00m\x00\x00\x00\x06\x00\x00\x00t\x00\x00\x00" \
                  b"\n\x00\x00\x00{\x00\x00\x00\x14\x00\x00\x00\x86" \
                  b"\x00\x00\x00[\x01\x00\x00\x9b\x00\x00\x00\r\x00" \
                  b"\x00\x00\xf7\x01\x00\x00\t\x00\x00\x00\x05\x02\x00" \
                  b"\x00\x0f\x00\x00\x00\x0f\x02\x00\x00\x16\x00\x00" \
                  b"\x00\x1f\x02\x00\x00\x00Bye...\x00Hello!\x00It's " \
                  b"over.\x00{n} horse\x00{n} horses\x00Project-Id-Ver" \
                  b"sion: python 3.8\nReport-Msgid-Bugs-To: \nPOT-Crea" \
                  b"tion-Date: 2018-11-30 23:57+0100\nPO-Revision-Date" \
                  b": 2018-11-30 23:57+0100\nLast-Translator: s-ball <" \
                  b"s-ball@laposte.net>\nLanguage-Team: French\nLangua" \
                  b"ge: fr\nMIME-Version: 1.0\nContent-Type: text/plai" \
                  b"n; charset=UTF-8\nContent-Transfer-Encoding: 8bit" \
                  b"\nPlural-Forms: nplurals=2; plural=(n > 1);\n\x00A" \
                  b"u revoir ...\x00Bonjour !\x00C'est termin\xc3\xa9." \
                  b"\x00{n} cheval\x00{n} chevaux\x00"
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
            )
        )

    def test_wrong(self):
        """Test wrong option"""
        rc, stdout, stderr = assert_python_failure(self.script, '-x')
        self.assertEqual(1, rc)
        self.assertTrue(stderr.startswith(
            b'Generate binary message catalog from textual'
            b' translation description.'
            )
        )

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
