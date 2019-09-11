import re
import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer_common.info import ID
    from c_parser import info
    from c_globals.supported import is_supported, ignored_from_file


class IsSupportedTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'const char *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'int'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertTrue(result)

    @unittest.expectedFailure
    def test_not_supported(self):
        statics = [
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject *'),
                info.StaticVar('src1/spam.c', None, 'var1', 'PyObject[10]'),
                ]
        for static in statics:
            with self.subTest(static):
                result = is_supported(static)

                self.assertFalse(result)


class IgnoredFromFileTests(unittest.TestCase):

    maxDiff = None

    _return_read_tsv = ()

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def _read_tsv(self, *args):
        self.calls.append(('_read_tsv', args))
        return self._return_read_tsv

    def test_typical(self):
        lines = textwrap.dedent('''
            filename    funcname        name    kind    reason
            file1.c     -       var1    variable        ...
            file1.c     func1   local1  variable        |
            file1.c     -       var2    variable        ???
            file1.c     func2   local2  variable           |
            file2.c     -       var1    variable        reasons
            ''').strip().splitlines()
        lines = [re.sub(r'\s{1,8}', '\t', line, 4).replace('|', '')
                 for line in lines]
        self._return_read_tsv = [tuple(v.strip() for v in line.split('\t'))
                                 for line in lines[1:]]

        ignored = ignored_from_file('spam.c', _read_tsv=self._read_tsv)

        self.assertEqual(ignored, {
            'variables': {
                ID('file1.c', '', 'var1'): '...',
                ID('file1.c', 'func1', 'local1'): '',
                ID('file1.c', '', 'var2'): '???',
                ID('file1.c', 'func2', 'local2'): '',
                ID('file2.c', '', 'var1'): 'reasons',
                },
            })
        self.assertEqual(self.calls, [
            ('_read_tsv', ('spam.c', 'filename\tfuncname\tname\tkind\treason')),
            ])

    def test_empty(self):
        self._return_read_tsv = []

        ignored = ignored_from_file('spam.c', _read_tsv=self._read_tsv)

        self.assertEqual(ignored, {
            'variables': {},
            })
        self.assertEqual(self.calls, [
            ('_read_tsv', ('spam.c', 'filename\tfuncname\tname\tkind\treason')),
            ])
