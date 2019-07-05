import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_statics import info
    from c_statics.find import statics


class _Base(unittest.TestCase):

#    _return_iter_statics = ()
#    _return_is_supported = ()

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def add_static(self, filename, funcname, name, vartype, *, supported=True):
        static = info.StaticVar(filename, funcname, name, vartype)
        try:
            statics = self._return_iter_statics
        except AttributeError:
            statics = self._return_iter_statics = []
        statics.append(static)

        try:
            unsupported = self._return_is_supported
        except AttributeError:
            unsupported = self._return_is_supported = set()
        if not supported:
            unsupported.add(static)

        return static, supported

    def _iter_statics(self, *args):
        self.calls.append(('_iter_statics', args))
        return iter(self._return_iter_statics)

    def _is_supported(self, static):
        self.calls.append(('_is_supported', (static,)))
        return static not in self._return_is_supported


class StaticsTest(_Base):

    maxDiff = None

    def test_typical(self):
        expected = [
            self.add_static('src1/spam.c', None, 'var1', 'const char *'),
            self.add_static('src1/spam.c', 'ham', 'initialized', 'int'),
            self.add_static('src1/spam.c', None, 'var2', 'PyObject *', supported=False),
            self.add_static('src1/eggs.c', 'tofu', 'ready', 'int', supported=False),
            self.add_static('src1/spam.c', None, 'freelist', '(PyTupleObject *)[10]', supported=False),
            self.add_static('src1/sub/ham.c', None, 'var1', 'const char const *'),
            self.add_static('src2/jam.c', None, 'var1', 'int'),
            self.add_static('src2/jam.c', None, 'var2', 'MyObject *', supported=False),
            self.add_static('Include/spam.h', None, 'data', 'const int'),
            ]
        dirs = ['src1', 'src2', 'Include']

        found = statics(
                dirs,
                'ignored.tsv',
                'known.tsv',
                _iter_statics=self._iter_statics,
                _is_supported=self._is_supported,
                )

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_iter_statics', (dirs,)),
            ] + [('_is_supported', (v,))
                 for v, _ in expected])

    def test_no_dirs(self):
        found = statics([], '', '')

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [])
