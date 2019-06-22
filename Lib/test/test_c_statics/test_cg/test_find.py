import unittest

from test.test_c_statics.cg import info
from test.test_c_statics.cg.find import statics


def supported_global(filename, name, vartype):
    static = info.StaticVar(filename, None, name, vartype)
    return static, True


def unsupported_global(filename, name, vartype):
    static = info.StaticVar(filename, None, name, vartype)
    return static, False


def supported_local(filename, funcname, name, vartype):
    static = info.StaticVar(filename, funcname, name, vartype)
    return static, True


def unsupported_local(filename, funcname, name, vartype):
    static = info.StaticVar(filename, funcname, name, vartype)
    return static, False


class StaticsTest(unittest.TestCase):

    maxDiff = None

    @unittest.expectedFailure
    def test_typical(self):
        raise NotImplementedError
        found = statics(
                ['src1', 'src2', 'Include'],
                'ignored.tsv',
                'known.tsv',
                )

        self.assertEqual(found, [
            supported_global('src1/spam.c', 'var1', 'const char *'),
            supported_local('src1/spam.c', 'ham', 'initialized', 'int'),
            unsupported_global('src1/spam.c', 'var2', 'PyObject *'),
            unsupported_local('src1/eggs.c', 'tofu', 'ready', 'int'),
            unsupported_global('src1/spam.c', 'freelist', '(PyTupleObject *)[10]'),
            supported_global('src1/sub/ham.c', 'var1', 'const char const *'),
            supported_global('src2/jam.c', 'var1', 'int'),
            unsupported_global('src2/jam.c', 'var2', 'MyObject *'),
            supported_global('Include/spam.h', 'data', 'const int'),
            ])

    @unittest.expectedFailure
    def test_no_modifiers(self):
        raise NotImplementedError
        found = statics(
                ['src1', 'src2', 'Include'],
                '',
                '',
                )

        self.assertEqual(found, [
            supported_global('src1/spam.c', 'var1', 'const char *'),
            supported_local('src1/spam.c', 'ham', 'initialized', 'int'),
            unsupported_global('src1/spam.c', 'var2', 'PyObject *'),
            unsupported_local('src1/eggs.c', 'tofu', 'ready', 'int'),
            unsupported_global('src1/spam.c', 'freelist', '(PyTupleObject *)[10]'),
            supported_global('src1/sub/ham.c', 'var1', 'const char const *'),
            supported_global('src2/jam.c', 'var1', 'int'),
            unsupported_global('src2/jam.c', 'var2', 'MyObject *'),
            supported_global('Include/spam.h', 'data', 'const int'),
            ])

    def test_no_dirs(self):
        found = statics([], '', '')

        self.assertEqual(found, [])
