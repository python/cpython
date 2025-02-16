"""Tests to cover the Tools/i18n package"""

import os
import re
import sys
import unittest
from textwrap import dedent
from pathlib import Path

from test.support.script_helper import assert_python_ok
from test.test_tools import skip_if_missing, toolsdir
from test.support.os_helper import temp_cwd, temp_dir


skip_if_missing()

DATA_DIR = Path(__file__).resolve().parent / 'i18n_data'


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

    def extract_from_str(self, module_content, *, args=(), strict=True, with_stderr=False):
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
        msgids = self.get_msgids(data)
        if not with_stderr:
            return msgids
        return msgids, res.err

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
        for input_file in DATA_DIR.glob('*.py'):
            output_file = input_file.with_suffix('.pot')
            with self.subTest(input_file=f'i18n_data/{input_file}'):
                contents = input_file.read_text(encoding='utf-8')
                with temp_cwd(None):
                    Path(input_file.name).write_text(contents)
                    assert_python_ok('-Xutf8', self.script, '--docstrings', input_file.name)
                    output = Path('messages.pot').read_text(encoding='utf-8')

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


def update_POT_snapshots():
    for input_file in DATA_DIR.glob('*.py'):
        output_file = input_file.with_suffix('.pot')
        contents = input_file.read_bytes()
        with temp_cwd(None):
            Path(input_file.name).write_bytes(contents)
            assert_python_ok('-Xutf8', Test_pygettext.script, '--docstrings', input_file.name)
            output = Path('messages.pot').read_text(encoding='utf-8')

        output = normalize_POT_file(output)
        output_file.write_text(output, encoding='utf-8')


if __name__ == '__main__':
    # To regenerate POT files
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_POT_snapshots()
        sys.exit(0)
    unittest.main()
