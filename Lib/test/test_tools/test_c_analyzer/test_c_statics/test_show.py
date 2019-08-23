import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info
    from c_statics.show import basic


TYPICAL = [
        info.Variable.from_parts('src1/spam.c', None, 'var1', 'const char *'),
        info.Variable.from_parts('src1/spam.c', 'ham', 'initialized', 'int'),
        info.Variable.from_parts('src1/spam.c', None, 'var2', 'PyObject *'),
        info.Variable.from_parts('src1/eggs.c', 'tofu', 'ready', 'int'),
        info.Variable.from_parts('src1/spam.c', None, 'freelist', '(PyTupleObject *)[10]'),
        info.Variable.from_parts('src1/sub/ham.c', None, 'var1', 'const char const *'),
        info.Variable.from_parts('src2/jam.c', None, 'var1', 'int'),
        info.Variable.from_parts('src2/jam.c', None, 'var2', 'MyObject *'),
        info.Variable.from_parts('Include/spam.h', None, 'data', 'const int'),
        ]


class BasicTests(unittest.TestCase):

    def setUp(self):
        self.lines = []

    def print(self, line):
        self.lines.append(line)

    def test_typical(self):
        basic(TYPICAL,
              _print=self.print)

        self.assertEqual(self.lines, [
            'src1/spam.c:var1',
            'src1/spam.c:ham():initialized',
            'src1/spam.c:var2',
            'src1/eggs.c:tofu():ready',
            'src1/spam.c:freelist',
            'src1/sub/ham.c:var1',
            'src2/jam.c:var1',
            'src2/jam.c:var2',
            'Include/spam.h:data',
            ])

    def test_no_rows(self):
        basic([],
              _print=self.print)

        self.assertEqual(self.lines, [])
