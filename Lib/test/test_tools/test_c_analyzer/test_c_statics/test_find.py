import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info
    from c_statics.find import statics


class _Base(unittest.TestCase):

    maxDiff = None

    _return_iter_statics = None
    _return_is_supported = None
    _return_load_ignored = None
    _return_load_known = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def _add_static(self, static, funcs):
        if not funcs:
            funcs = ['iter_statics']
        elif 'is_supported' in funcs and 'iter_statics' not in funcs:
            funcs.append('iter_statics')

        for func in funcs:
            func = f'_return_{func}'
            results = getattr(self, func)
            if results is None:
                results = []
                setattr(self, func, results)
            results.append(static)

    def add_static(self, filename, funcname, name, vartype, *,
                   supported=True,
                   ignored=False,
                   known=False,
                   ):
        static = info.StaticVar(filename, funcname, name, vartype)
        funcs = []
        if supported is not None:
            funcs.append('iter_statics')
            if supported:
                funcs.append('is_supported')
        if ignored:
            funcs.append('load_ingored')
        if known:
            funcs.append('load_known')
        self._add_static(static, funcs)
        return static, supported

    def _iter_statics(self, *args):
        self.calls.append(('_iter_statics', args))
        return iter(self._return_iter_statics or ())

    def _is_supported(self, static, ignored=None, known=None):
        self.calls.append(('_is_supported', (static, ignored, known)))
        return static in (self._return_is_supported or ())

    def _load_ignored(self, filename):
        self.calls.append(('_load_ignored', (filename,)))
        return iter(self._return_load_ignored or ())

    def _load_known(self, filename):
        self.calls.append(('_load_known', (filename,)))
        return iter(self._return_load_known or ())


class StaticsTest(_Base):

    def test_typical(self):
        self._return_load_ignored = ()
        self._return_load_known = ()
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
                _load_ignored=self._load_ignored,
                _load_known=self._load_known,
                )

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_load_ignored', ('ignored.tsv',)),
            ('_load_known', ('known.tsv',)),
            ('_iter_statics', (dirs,)),
            ] + [('_is_supported', (v, set(), set()))
                 for v, _ in expected])

    def test_no_dirs(self):
        found = statics([], '', '')

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [])
