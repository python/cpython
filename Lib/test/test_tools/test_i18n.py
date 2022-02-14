"""Tests to cover the Tools/i18n package"""

import os
import sys
import unittest
from textwrap import dedent

from test.support.script_helper import assert_python_ok
from test.test_tools import skip_if_missing, toolsdir
from test.support.os_helper import temp_cwd, temp_dir


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
            with open(filename, 'w', encoding='utf-8') as fp:
                fp.write(module_content)
            assert_python_ok(self.script, '-D', filename)
            with open('messages.pot', encoding='utf-8') as fp:
                data = fp.read()
        return self.get_msgids(data)

    def test_header(self):
        """Make sure the required fields are in the header, according to:
           http://www.gnu.org/software/gettext/manual/gettext.html#Header-Entry
        """
        with temp_cwd(None) as cwd:
            assert_python_ok(self.script)
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
            assert_python_ok(self.script)
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
        self.assertNotIn('foo', msgids)
        self.assertNotIn('bar', msgids)

    def test_calls_in_fstring_with_keyword_args(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_('foo', bar='baz')}"
        '''))
        self.assertNotIn('foo', msgids)
        self.assertNotIn('bar', msgids)
        self.assertNotIn('baz', msgids)

    def test_calls_in_fstring_with_partially_wrong_expression(self):
        msgids = self.extract_docstrings_from_str(dedent('''\
        f"{_(f'foo') + _('bar')}"
        '''))
        self.assertNotIn('foo', msgids)
        self.assertIn('bar', msgids)

    def test_files_list(self):
        """Make sure the directories are inspected for source files
           bpo-31920
        """
        text1 = 'Text to translate1'
        text2 = 'Text to translate2'
        text3 = 'Text to ignore'
        with temp_cwd(None), temp_dir(None) as sdir:
            os.mkdir(os.path.join(sdir, 'pypkg'))
            with open(os.path.join(sdir, 'pypkg', 'pymod.py'), 'w',
                      encoding='utf-8') as sfile:
                sfile.write(f'_({text1!r})')
            os.mkdir(os.path.join(sdir, 'pkg.py'))
            with open(os.path.join(sdir, 'pkg.py', 'pymod2.py'), 'w',
                      encoding='utf-8') as sfile:
                sfile.write(f'_({text2!r})')
            os.mkdir(os.path.join(sdir, 'CVS'))
            with open(os.path.join(sdir, 'CVS', 'pymod3.py'), 'w',
                      encoding='utf-8') as sfile:
                sfile.write(f'_({text3!r})')
            assert_python_ok(self.script, sdir)
            with open('messages.pot', encoding='utf-8') as fp:
                data = fp.read()
            self.assertIn(f'msgid "{text1}"', data)
            self.assertIn(f'msgid "{text2}"', data)
            self.assertNotIn(text3, data)
