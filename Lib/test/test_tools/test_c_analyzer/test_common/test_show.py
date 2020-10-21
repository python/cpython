import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.variables import info
    from c_analyzer.common.show import (
            basic,
            )


TYPICAL = [
        info.Variable.from_parts('src1/spam.c', None, 'var1', 'static const char *'),
        info.Variable.from_parts('src1/spam.c', 'ham', 'initialized', 'static int'),
        info.Variable.from_parts('src1/spam.c', None, 'var2', 'static PyObject *'),
        info.Variable.from_parts('src1/eggs.c', 'tofu', 'ready', 'static int'),
        info.Variable.from_parts('src1/spam.c', None, 'freelist', 'static (PyTupleObject *)[10]'),
        info.Variable.from_parts('src1/sub/ham.c', None, 'var1', 'static const char const *'),
        info.Variable.from_parts('src2/jam.c', None, 'var1', 'static int'),
        info.Variable.from_parts('src2/jam.c', None, 'var2', 'static MyObject *'),
        info.Variable.from_parts('Include/spam.h', None, 'data', 'static const int'),
        ]


class BasicTests(unittest.TestCase):

    maxDiff = None

    def setUp(self):
        self.lines = []

    def print(self, line):
        self.lines.append(line)

    def test_typical(self):
        basic(TYPICAL,
              _print=self.print)

        self.assertEqual(self.lines, [
            'src1/spam.c:var1                                                 static const char *',
            'src1/spam.c:ham():initialized                                    static int',
            'src1/spam.c:var2                                                 static PyObject *',
            'src1/eggs.c:tofu():ready                                         static int',
            'src1/spam.c:freelist                                             static (PyTupleObject *)[10]',
            'src1/sub/ham.c:var1                                              static const char const *',
            'src2/jam.c:var1                                                  static int',
            'src2/jam.c:var2                                                  static MyObject *',
            'Include/spam.h:data                                              static const int',
            ])

    def test_no_rows(self):
        basic([],
              _print=self.print)

        self.assertEqual(self.lines, [])
