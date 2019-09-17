import re
import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.info import Variable
    from c_analyzer_common.info import ID
    from c_analyzer_common.known import from_file


class FromFileTests(unittest.TestCase):

    maxDiff = None

    _return_read_tsv = ()

    def tearDown(self):
        Variable._isglobal.instances.clear()

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
            filename    funcname        name    kind    declaration
            file1.c     -       var1    variable        static int
            file1.c     func1   local1  variable        static int
            file1.c     -       var2    variable        int
            file1.c     func2   local2  variable        char *
            file2.c     -       var1    variable        char *
            ''').strip().splitlines()
        lines = [re.sub(r'\s+', '\t', line, 4) for line in lines]
        self._return_read_tsv = [tuple(v.strip() for v in line.split('\t'))
                                 for line in lines[1:]]

        known = from_file('spam.c', _read_tsv=self._read_tsv)

        self.assertEqual(known, {
            'variables': {v.id: v for v in [
                Variable.from_parts('file1.c', '', 'var1', 'static int'),
                Variable.from_parts('file1.c', 'func1', 'local1', 'static int'),
                Variable.from_parts('file1.c', '', 'var2', 'int'),
                Variable.from_parts('file1.c', 'func2', 'local2', 'char *'),
                Variable.from_parts('file2.c', '', 'var1', 'char *'),
                ]},
            })
        self.assertEqual(self.calls, [
            ('_read_tsv', ('spam.c', 'filename\tfuncname\tname\tkind\tdeclaration')),
            ])

    def test_empty(self):
        self._return_read_tsv = []

        known = from_file('spam.c', _read_tsv=self._read_tsv)

        self.assertEqual(known, {
            'variables': {},
            })
        self.assertEqual(self.calls, [
            ('_read_tsv', ('spam.c', 'filename\tfuncname\tname\tkind\tdeclaration')),
            ])
