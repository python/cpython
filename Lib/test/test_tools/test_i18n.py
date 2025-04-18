"""Tests to cover the Tools/i18n package"""

import os
import re
import sys
import unittest
from textwrap import dedent
from pathlib import Path

from test.support.script_helper import assert_python_ok
from test.test_tools import imports_under_tool, skip_if_missing, toolsdir
from test.support.os_helper import temp_cwd, temp_dir


skip_if_missing()

DATA_DIR = Path(__file__).resolve().parent / 'i18n_data'


with imports_under_tool("i18n"):
    from pygettext import (parse_spec, process_keywords, DEFAULTKEYWORDS,
                           unparse_spec)


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
            ('foo', ('foo', {'msgid': 0})),
            ('foo:1', ('foo', {'msgid': 0})),
            ('foo:1,2', ('foo', {'msgid': 0, 'msgid_plural': 1})),
            ('foo:1, 2', ('foo', {'msgid': 0, 'msgid_plural': 1})),
            ('foo:1,2c', ('foo', {'msgid': 0, 'msgctxt': 1})),
            ('foo:2c,1', ('foo', {'msgid': 0, 'msgctxt': 1})),
            ('foo:2c ,1', ('foo', {'msgid': 0, 'msgctxt': 1})),
            ('foo:1,2,3c', ('foo', {'msgid': 0, 'msgid_plural': 1, 'msgctxt': 2})),
            ('foo:1, 2, 3c', ('foo', {'msgid': 0, 'msgid_plural': 1, 'msgctxt': 2})),
            ('foo:3c,1,2', ('foo', {'msgid': 0, 'msgid_plural': 1, 'msgctxt': 2})),
        )
        for spec, expected in valid:
            with self.subTest(spec=spec):
                self.assertEqual(parse_spec(spec), expected)
                # test unparse-parse round-trip
                self.assertEqual(parse_spec(unparse_spec(*expected)), expected)

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

    def test_process_keywords(self):
        default_keywords = {name: [spec] for name, spec
                            in DEFAULTKEYWORDS.items()}
        inputs = (
            (['foo'], True),
            (['_:1,2'], True),
            (['foo', 'foo:1,2'], True),
            (['foo'], False),
            (['_:1,2', '_:1c,2,3', 'pgettext'], False),
            # Duplicate entries
            (['foo', 'foo'], True),
            (['_'], False)
        )
        expected = (
            {'foo': [{'msgid': 0}]},
            {'_': [{'msgid': 0, 'msgid_plural': 1}]},
            {'foo': [{'msgid': 0}, {'msgid': 0, 'msgid_plural': 1}]},
            default_keywords | {'foo': [{'msgid': 0}]},
            default_keywords | {'_': [{'msgid': 0, 'msgid_plural': 1},
                                      {'msgctxt': 0, 'msgid': 1, 'msgid_plural': 2},
                                      {'msgid': 0}],
                                'pgettext': [{'msgid': 0},
                                             {'msgctxt': 0, 'msgid': 1}]},
            {'foo': [{'msgid': 0}]},
            default_keywords,
        )
        for (keywords, no_default_keywords), expected in zip(inputs, expected):
            with self.subTest(keywords=keywords,
                              no_default_keywords=no_default_keywords):
                processed = process_keywords(
                    keywords,
                    no_default_keywords=no_default_keywords)
                self.assertEqual(processed, expected)

    def test_multiple_keywords_same_funcname_errors(self):
        # If at least one keyword spec for a given funcname matches,
        # no error should be printed.
        msgids, stderr = self.extract_from_str(dedent('''\
        _("foo", 42)
        _(42, "bar")
        '''), args=('--keyword=_:1', '--keyword=_:2'), with_stderr=True)
        self.assertIn('foo', msgids)
        self.assertIn('bar', msgids)
        self.assertEqual(stderr, b'')

        # If no keyword spec for a given funcname matches,
        # all errors are printed.
        msgids, stderr = self.extract_from_str(dedent('''\
        _(x, 42)
        _(42, y)
        '''), args=('--keyword=_:1', '--keyword=_:2'), with_stderr=True,
              strict=False)
        self.assertEqual(msgids, [''])
        # Normalize line endings on Windows
        stderr = stderr.decode('utf-8').replace('\r', '')
        self.assertEqual(
            stderr,
            '*** test.py:1: No keywords matched gettext call "_":\n'
            '\tkeyword="_": Expected a string constant for argument 1, got x\n'
            '\tkeyword="_:2": Expected a string constant for argument 2, got 42\n'
            '*** test.py:2: No keywords matched gettext call "_":\n'
            '\tkeyword="_": Expected a string constant for argument 1, got 42\n'
            '\tkeyword="_:2": Expected a string constant for argument 2, got y\n')


def extract_from_snapshots():
    snapshots = {
        'messages.py': (),
        'fileloc.py': ('--docstrings',),
        'docstrings.py': ('--docstrings',),
        'comments.py': ('--add-comments=i18n:',),
        'custom_keywords.py': ('--keyword=foo', '--keyword=nfoo:1,2',
                               '--keyword=pfoo:1c,2',
                               '--keyword=npfoo:1c,2,3', '--keyword=_:1,2'),
        'multiple_keywords.py': ('--keyword=foo:1c,2,3', '--keyword=foo:1c,2',
                                 '--keyword=foo:1,2',
                                 # repeat a keyword to make sure it is extracted only once
                                 '--keyword=foo', '--keyword=foo'),
        # == Test character escaping
        # Escape ascii and unicode:
        'escapes.py': ('--escape', '--add-comments='),
        # Escape only ascii and let unicode pass through:
        ('escapes.py', 'ascii-escapes.pot'): ('--add-comments=',),
    }

    for filename, args in snapshots.items():
        if isinstance(filename, tuple):
            filename, output_file = filename
            output_file = DATA_DIR / output_file
            input_file = DATA_DIR / filename
        else:
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
