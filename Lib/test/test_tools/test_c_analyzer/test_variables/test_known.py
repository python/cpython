import re
import textwrap
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.common.info import ID
    from c_analyzer.variables.info import Variable
    from c_analyzer.variables.known import (
            read_file,
            from_file,
            )

class _BaseTests(unittest.TestCase):

    maxDiff = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls


class ReadFileTests(_BaseTests):

    _return_read_tsv = ()

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

        known = list(read_file('known.tsv', _read_tsv=self._read_tsv))

        self.assertEqual(known, [
            ('variable', ID('file1.c', '', 'var1'), 'static int'),
            ('variable', ID('file1.c', 'func1', 'local1'), 'static int'),
            ('variable', ID('file1.c', '', 'var2'), 'int'),
            ('variable', ID('file1.c', 'func2', 'local2'), 'char *'),
            ('variable', ID('file2.c', '', 'var1'), 'char *'),
            ])
        self.assertEqual(self.calls, [
            ('_read_tsv',
             ('known.tsv', 'filename\tfuncname\tname\tkind\tdeclaration')),
            ])

    def test_empty(self):
        self._return_read_tsv = []

        known = list(read_file('known.tsv', _read_tsv=self._read_tsv))

        self.assertEqual(known, [])
        self.assertEqual(self.calls, [
            ('_read_tsv', ('known.tsv', 'filename\tfuncname\tname\tkind\tdeclaration')),
            ])


class FromFileTests(_BaseTests):

    _return_read_file = ()
    _return_handle_var = ()

    def _read_file(self, infile):
        self.calls.append(('_read_file', (infile,)))
        return iter(self._return_read_file)

    def _handle_var(self, varid, decl):
        self.calls.append(('_handle_var', (varid, decl)))
        var = self._return_handle_var.pop(0)
        return var

    def test_typical(self):
        expected = [
            Variable.from_parts('file1.c', '', 'var1', 'static int'),
            Variable.from_parts('file1.c', 'func1', 'local1', 'static int'),
            Variable.from_parts('file1.c', '', 'var2', 'int'),
            Variable.from_parts('file1.c', 'func2', 'local2', 'char *'),
            Variable.from_parts('file2.c', '', 'var1', 'char *'),
            ]
        self._return_read_file = [('variable', v.id, v.vartype)
                                  for v in expected]
#            ('variable', ID('file1.c', '', 'var1'), 'static int'),
#            ('variable', ID('file1.c', 'func1', 'local1'), 'static int'),
#            ('variable', ID('file1.c', '', 'var2'), 'int'),
#            ('variable', ID('file1.c', 'func2', 'local2'), 'char *'),
#            ('variable', ID('file2.c', '', 'var1'), 'char *'),
#            ]
        self._return_handle_var = list(expected)  # a copy

        known = from_file('known.tsv',
                          handle_var=self._handle_var,
                          _read_file=self._read_file,
                          )

        self.assertEqual(known, {
            'variables': {v.id: v for v in expected},
            })
#                Variable.from_parts('file1.c', '', 'var1', 'static int'),
#                Variable.from_parts('file1.c', 'func1', 'local1', 'static int'),
#                Variable.from_parts('file1.c', '', 'var2', 'int'),
#                Variable.from_parts('file1.c', 'func2', 'local2', 'char *'),
#                Variable.from_parts('file2.c', '', 'var1', 'char *'),
#                ]},
#            })
        self.assertEqual(self.calls, [
            ('_read_file', ('known.tsv',)),
            *[('_handle_var', (v.id, v.vartype))
              for v in expected],
            ])

    def test_empty(self):
        self._return_read_file = []

        known = from_file('known.tsv',
                          handle_var=self._handle_var,
                          _read_file=self._read_file,
                          )

        self.assertEqual(known, {
            'variables': {},
            })
        self.assertEqual(self.calls, [
            ('_read_file', ('known.tsv',)),
            ])
