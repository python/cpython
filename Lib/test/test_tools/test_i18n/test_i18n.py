"""Tests to cover the Tools/i18n package"""

import os
import re
import sys
import unittest
from pathlib import Path
from test.support.os_helper import temp_cwd
from test.support.script_helper import assert_python_ok
from test.test_tools import skip_if_missing, toolsdir

skip_if_missing()

DATA_DIR = Path(__file__).parent / 'data'


class Test_pygettext(unittest.TestCase):
    """Tests for the pygettext.py tool"""

    script = os.path.join(toolsdir, 'i18n', 'pygettext.py')

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

    def assertPOEqual(self, expected, actual):
        """Check if two PO files are equal"""
        # Normalize the creation date
        date_pattern = re.compile(r'"POT-Creation-Date: .+?\n"')
        header = '"POT-Creation-Date: 2000-01-01 00:00+0000\\n"'
        expected = re.sub(date_pattern, header, expected)
        actual = re.sub(date_pattern, header, actual)

        # Normalize the path separators in case this test is running on a
        # platform which does not use '/' as a default separator
        fileloc_pattern = re.compile(r'#:.+')

        def replace(match):
            return match[0].replace(os.sep, "/")
        expected = re.sub(fileloc_pattern, replace, expected)
        actual = re.sub(fileloc_pattern, replace, actual)

        self.assertEqual(expected, actual)

    def test_header(self):
        """Make sure the required fields are in the header, according to:
           http://www.gnu.org/software/gettext/manual/gettext.html#Header-Entry
        """
        with temp_cwd(None):
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
            # self.assertIn("Report-Msgid-Bugs-To", header)
            # self.assertIn("Language", header)

            # "Plural-Forms" is optional

    @unittest.skipIf(sys.platform.startswith('aix'),
                     'bpo-29972: broken test on AIX')
    def test_POT_Creation_Date(self):
        """ Match the date format from xgettext for POT-Creation-Date """
        from datetime import datetime
        with temp_cwd(None):
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

    def test_files(self):
        """Test message and docstring extraction.
        Compares the script output against the .po files in the data folder.
        """
        filenames = (('messages.py', 'messages.pot'),
                     ('docstrings.py', 'docstrings.pot'),
                     ('mixed.py', 'mixed.pot'))

        for input_file, output_file in filenames:
            with self.subTest(f'Input file: data/{input_file}'):
                contents = (DATA_DIR / input_file).read_text(encoding='utf-8')
                with temp_cwd(None):
                    Path(input_file).write_text(contents, encoding='utf-8')
                    assert_python_ok(self.script, '-D', input_file)
                    output = Path('messages.pot').read_text(encoding='utf-8')

                expected = (DATA_DIR / output_file).read_text(encoding='utf-8')
                self.assertPOEqual(expected, output)

    def test_files_list(self):
        """Make sure the directories are inspected for source files
           bpo-31920
        """
        filenames = ('messages.py', 'docstrings.py', 'mixed.py')

        with temp_cwd(None):
            pkg_dir = Path('pypkg')
            pkg_dir.mkdir()

            for filename in filenames:
                data = (DATA_DIR / filename).read_text(encoding='utf-8')
                path = pkg_dir / filename
                path.write_text(data, encoding='utf-8')

            assert_python_ok(self.script, '-D', 'pypkg')
            output = Path('messages.pot').read_text(encoding='utf-8')

        expected = (Path(DATA_DIR) / 'all.pot').read_text(encoding='utf-8')
        self.assertPOEqual(expected, output)
