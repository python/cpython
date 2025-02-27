"""Tests to cover the Tools/i18n package"""

import os
import re
import sys
import unittest
from textwrap import dedent
from pathlib import Path

from test.support.script_helper import assert_python_ok, assert_python_failure
from test.test_tools import imports_under_tool, skip_if_missing, toolsdir
from test.support.os_helper import temp_cwd, temp_dir


skip_if_missing()

DATA_DIR = Path(__file__).resolve().parent / 'i18n_data'


with imports_under_tool("i18n"):
    from pygettext import parse_spec


def normalize_POT_file(pot):
    """Normalize the POT creation timestamp, charset and
    file locations to make the POT file easier to compare.

    """
    # Normalize the creation date.
    date_pattern = re.compile(r'"POT-Creation-Date: .+?\\n"')
    header = r'"POT-Creation-Date: 2000-01-01 00:00+0000\\n"'
    pot = re.sub(date_pattern, header, pot)

    # Normalize charset to UTF-8 (currently there's no way to specify the output charset).
    charset_pattern = re.compile(r'"Content-Type: text/plain; charset=.+?\\n"')
    charset = r'"Content-Type: text/plain; charset=UTF-8\\n"'
    pot = re.sub(charset_pattern, charset, pot)

    # Normalize file location path separators in case this test is
    # running on Windows (which uses '\').
    fileloc_pattern = re.compile(r'#:.+')

    def replace(match):
        return match[0].replace(os.sep, "/")
    pot = re.sub(fileloc_pattern, replace, pot)
    return pot


class Test_pygettext(unittest.TestCase):
    """Tests for the pygettext.py tool"""

    script = Path(toolsdir, 'i18n', 'pygettext.py')

    def get_header(self, data):
        """ utility: return the header of a .po file as a dictionary """
        headers = {}
        for line in data.split('\n'):
            if not line or line.startswith(('#', 'msgid', 'msgstr')):
                continue
            line = line.strip('"')
            key, val = line.split(':', 1)
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

    def assert_POT_equal(self, expected, actual):
        """Check if two POT files are equal"""
        self.maxDiff = None
        self.assertEqual(normalize_POT_file(expected), normalize_POT_file(actual))

    def extract_from_str(self, module_content, *, args=(), strict=True,
                         with_stderr=False, raw=False):
        """Return all msgids extracted from module_content."""
        filename = 'test.py'
        with temp_cwd(None):
            with open(filename, 'w', encoding='utf-8') as fp:
                fp.write(module_content)
            res = assert_python_ok('-Xutf8', self.script, *args, filename)
            if strict:
                self.assertEqual(res.err, b'')
            with open('messages.pot', encoding='utf-8') as fp:
                data = fp.read()
        if not raw:
            data = self.get_msgids(data)
        if not with_stderr:
            return data
        return data, res.err

    def extract_docstrings_from_str(self, module_content):
        """Return all docstrings extracted from module_content."""
        return self.extract_from_str(module_content, args=('--docstrings',), strict=False)

    def get_stderr(self, module_content):
        return self.extract_from_str(module_content, strict=False, with_stderr=True)[1]

    def test_header(self):
        """Make sure the required fields are in the header, according to:
           http://www.gnu.org/software/gettext/manual/gettext.html#Header-Entry
        """
        with temp_cwd(None) as cwd:
            assert_python_ok('-Xutf8', self.script)
            with open('messages.pot', encoding='utf-8') as fp:
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
            assert_python_ok('-Xutf8', self.script)
            with open('messages.pot', encoding='utf-8') as fp:
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

    def test_moduledocstring(self):
        for doc in ('"""doc"""', "r'''doc'''", "R'doc'", 'u"doc"'):
            with self.subTest(doc):
                msgids = self.extract_docstrings_from_str(dedent('''\
                %s
                ''' % doc))
                self.assertIn('doc', msgids)

    def test_moduledocstring_bytes(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        b"""doc"""
        '''))
        self.assertFalse([msgid for msgid in msgids if 'doc' in msgid])

    def test_moduledocstring_fstring(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
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
        """ Test docstring extraction for a class with colons occurring within
        the parentheses.
        """
        msgids = self.extract_docstrings_from_str(dedent('''\
        class D(L[1:2], F({1: 2}), metaclass=M(lambda x: x)):
            """doc"""
        '''))
        self.assertIn('doc', msgids)

    def test_calls_in_fstrings(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_('foo bar')}"
        '''))
        self.assertIn('foo bar', msgids)

    def test_calls_in_fstrings_raw(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        rf"{_('foo bar')}"
        '''))
        self.assertIn('foo bar', msgids)

    def test_calls_in_fstrings_nested(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"""{f'{_("foo bar")}'}"""
        '''))
        self.assertIn('foo bar', msgids)

    def test_calls_in_fstrings_attribute(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{obj._('foo bar')}"
        '''))
        self.assertIn('foo bar', msgids)

    def test_calls_in_fstrings_with_call_on_call(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{type(str)('foo bar')}"
        '''))
        self.assertNotIn('foo bar', msgids)

    def test_calls_in_fstrings_with_format(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_('foo {bar}').format(bar='baz')}"
        '''))
        self.assertIn('foo {bar}', msgids)

    def test_calls_in_fstrings_with_wrong_input_1(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_(f'foo {bar}')}"
        '''))
        self.assertFalse([msgid for msgid in msgids if 'foo {bar}' in msgid])

    def test_calls_in_fstrings_with_wrong_input_2(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_(1)}"
        '''))
        self.assertNotIn(1, msgids)

    def test_calls_in_fstring_with_multiple_args(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_('foo', 'bar')}"
        '''))
        self.assertIn('foo', msgids)
        self.assertNotIn('bar', msgids)

    def test_calls_in_fstring_with_keyword_args(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_('foo', bar='baz')}"
        '''))
        self.assertIn('foo', msgids)
        self.assertNotIn('bar', msgids)
        self.assertNotIn('baz', msgids)

    def test_calls_in_fstring_with_partially_wrong_expression(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_(f'foo') + _('bar')}"
        '''))
        self.assertNotIn('foo', msgids)
        self.assertIn('bar', msgids)

    def test_function_and_class_names(self):
        """Test that function and class names are not mistakenly extracted."""
        msgids = self.extract_from_str(dedent('''\
        def _(x):
            pass

        def _(x="foo"):
            pass

        async def _(x):
            pass

        class _(object):
            pass
        '''))
        self.assertEqual(msgids, [''])

    def test_pygettext_output(self):
        """Test that the pygettext output exactly matches snapshots."""
        for input_file, output_file, output in extract_from_snapshots():
            with self.subTest(input_file=input_file):
                expected = output_file.read_text(encoding='utf-8')
                self.assert_POT_equal(expected, output)

    def test_files_list(self):
        """Make sure the directories are inspected for source files
           bpo-31920
        """
        text1 = 'Text to translate1'
        text2 = 'Text to translate2'
        text3 = 'Text to ignore'
        with temp_cwd(None), temp_dir(None) as sdir:
            pymod = Path(sdir, 'pypkg', 'pymod.py')
            pymod.parent.mkdir()
            pymod.write_text(f'_({text1!r})', encoding='utf-8')

            pymod2 = Path(sdir, 'pkg.py', 'pymod2.py')
            pymod2.parent.mkdir()
            pymod2.write_text(f'_({text2!r})', encoding='utf-8')

            pymod3 = Path(sdir, 'CVS', 'pymod3.py')
            pymod3.parent.mkdir()
            pymod3.write_text(f'_({text3!r})', encoding='utf-8')

            assert_python_ok('-Xutf8', self.script, sdir)
            data = Path('messages.pot').read_text(encoding='utf-8')
            self.assertIn(f'msgid "{text1}"', data)
            self.assertIn(f'msgid "{text2}"', data)
            self.assertNotIn(text3, data)

    def test_help_text(self):
        """Test that the help text is displayed."""
        res = assert_python_ok(self.script, '--help')
        self.assertEqual(res.out, b'')
        self.assertIn(b'pygettext -- Python equivalent of xgettext(1)', res.err)

    def test_error_messages(self):
        """Test that pygettext outputs error messages to stderr."""
        stderr = self.get_stderr(dedent('''\
        _(1+2)
        ngettext('foo')
        dgettext(*args, 'foo')
        '''))

        # Normalize line endings on Windows
        stderr = stderr.decode('utf-8').replace('\r', '')

        self.assertEqual(
            stderr,
            "*** test.py:1: Expected a string constant for argument 1, got 1 + 2\n"
            "*** test.py:2: Expected at least 2 positional argument(s) in gettext call, got 1\n"
            "*** test.py:3: Variable positional arguments are not allowed in gettext calls\n"
        )

    def test_extract_all_comments(self):
        """
        Test that the --add-comments option without an
        explicit tag extracts all translator comments.
        """
        for arg in ('--add-comments', '-c'):
            with self.subTest(arg=arg):
                data = self.extract_from_str(dedent('''\
                # Translator comment
                _("foo")
                '''), args=(arg,), raw=True)
                self.assertIn('#. Translator comment', data)

    def test_comments_with_multiple_tags(self):
        """
        Test that multiple --add-comments tags can be specified.
        """
        for arg in ('--add-comments={}', '-c{}'):
            with self.subTest(arg=arg):
                args = (arg.format('foo:'), arg.format('bar:'))
                data = self.extract_from_str(dedent('''\
                # foo: comment
                _("foo")

                # bar: comment
                _("bar")

                # baz: comment
                _("baz")
                '''), args=args, raw=True)
                self.assertIn('#. foo: comment', data)
                self.assertIn('#. bar: comment', data)
                self.assertNotIn('#. baz: comment', data)

    def test_comments_not_extracted_without_tags(self):
        """
        Test that translator comments are not extracted without
        specifying --add-comments.
        """
        data = self.extract_from_str(dedent('''\
        # Translator comment
        _("foo")
        '''), raw=True)
        self.assertNotIn('#.', data)

    def test_parse_keyword_spec(self):
        valid = (
            ('foo', ('foo', {0: 'msgid'})),
            ('foo:1', ('foo', {0: 'msgid'})),
            ('foo:1,2', ('foo', {0: 'msgid', 1: 'msgid_plural'})),
            ('foo:1, 2', ('foo', {0: 'msgid', 1: 'msgid_plural'})),
            ('foo:1,2c', ('foo', {0: 'msgid', 1: 'msgctxt'})),
            ('foo:2c,1', ('foo', {0: 'msgid', 1: 'msgctxt'})),
            ('foo:2c ,1', ('foo', {0: 'msgid', 1: 'msgctxt'})),
            ('foo:1,2,3c', ('foo', {0: 'msgid', 1: 'msgid_plural', 2: 'msgctxt'})),
            ('foo:1, 2, 3c', ('foo', {0: 'msgid', 1: 'msgid_plural', 2: 'msgctxt'})),
            ('foo:3c,1,2', ('foo', {0: 'msgid', 1: 'msgid_plural', 2: 'msgctxt'})),
        )
        for spec, expected in valid:
            with self.subTest(spec=spec):
                self.assertEqual(parse_spec(spec), expected)

        invalid = (
            ('foo:', "Invalid keyword spec 'foo:': missing argument positions"),
            ('foo:bar', "Invalid keyword spec 'foo:bar': position is not an integer"),
            ('foo:0', "Invalid keyword spec 'foo:0': argument positions must be strictly positive"),
            ('foo:-2', "Invalid keyword spec 'foo:-2': argument positions must be strictly positive"),
            ('foo:1,1', "Invalid keyword spec 'foo:1,1': duplicate positions"),
            ('foo:1,2,1', "Invalid keyword spec 'foo:1,2,1': duplicate positions"),
            ('foo:1c,2,1c', "Invalid keyword spec 'foo:1c,2,1c': duplicate positions"),
            ('foo:1c,2,3c', "Invalid keyword spec 'foo:1c,2,3c': msgctxt can only appear once"),
            ('foo:1,2,3', "Invalid keyword spec 'foo:1,2,3': too many positions"),
            ('foo:1c', "Invalid keyword spec 'foo:1c': msgctxt cannot appear without msgid"),
        )
        for spec, message in invalid:
            with self.subTest(spec=spec):
                with self.assertRaises(ValueError) as cm:
                    parse_spec(spec)
                self.assertEqual(str(cm.exception), message)


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
        sys.path.append(os.path.join(toolsdir,'i18n'))
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


def extract_from_snapshots():
    snapshots = {
        'messages.py': (),
        'fileloc.py': ('--docstrings',),
        'docstrings.py': ('--docstrings',),
        'comments.py': ('--add-comments=i18n:',),
        'custom_keywords.py': ('--keyword=foo', '--keyword=nfoo:1,2',
                               '--keyword=pfoo:1c,2',
                               '--keyword=npfoo:1c,2,3'),
    }

    for filename, args in snapshots.items():
        input_file = DATA_DIR / filename
        output_file = input_file.with_suffix('.pot')
        contents = input_file.read_bytes()
        with temp_cwd(None):
            Path(input_file.name).write_bytes(contents)
            assert_python_ok('-Xutf8', Test_pygettext.script, *args,
                             input_file.name)
            yield (input_file, output_file,
                   Path('messages.pot').read_text(encoding='utf-8'))


def update_POT_snapshots():
    for _, output_file, output in extract_from_snapshots():
        output = normalize_POT_file(output)
        output_file.write_text(output, encoding='utf-8')



if __name__ == '__main__':
    # To regenerate POT files
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_POT_snapshots()
        sys.exit(0)
    unittest.main()
