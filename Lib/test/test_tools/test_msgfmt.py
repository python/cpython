"""Tests for the Tools/i18n/msgfmt.py tool."""

import json
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


def update_catalog_snapshots():
    for po_file in data_dir.glob('*.po'):
        mo_file = po_file.with_suffix('.mo')
        compile_messages(po_file, mo_file)
        # Create a human-readable JSON file which is
        # easier to review than the binary .mo file.
        with open(mo_file, 'rb') as f:
            translations = GNUTranslations(f)
        catalog_file = po_file.with_suffix('.json')
        with open(catalog_file, 'w') as f:
            data = translations._catalog.items()
            data = sorted(data, key=lambda x: (isinstance(x[0], tuple), x[0]))
            json.dump(data, f, indent=4)
            f.write('\n')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_catalog_snapshots()
        sys.exit(0)
    unittest.main()
