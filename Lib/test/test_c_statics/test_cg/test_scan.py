import unittest

from test.test_c_statics.cg import info, symbols
from test.test_c_statics.cg.scan import iter_statics


class IterStaticsTests(unittest.TestCase):

    _return_iter_symbols = ()

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def _iter_symbols(self, *args):
        self.calls.append(('_iter_symbols', args))
        return iter(self. _return_iter_symbols)

    def test_typical(self):
        self._return_iter_symbols = [
                symbols.Symbol('var1', 'variable', False, 'dir1/spam.c'),
                symbols.Symbol('var2', 'variable', False, 'dir1/spam.c'),
                symbols.Symbol('func1', 'function', False, 'dir1/spam.c'),
                symbols.Symbol('func2', 'function', True, 'dir1/spam.c'),
                symbols.Symbol('var3', 'variable', False, 'dir1/spam.c'),
                symbols.Symbol('var4-1', 'variable', False, None),
                symbols.Symbol('var1', 'variable', True, 'dir1/ham.c'),
                symbols.Symbol('var1', 'variable', False, 'dir1/eggs.c'),
                symbols.Symbol('xyz', 'other', False, 'dir1/eggs.c'),
                symbols.Symbol('???', 'other', False, None),
                ]

        found = list(iter_statics(['dir1'],
                                  _iter_symbols=self._iter_symbols))

        self.assertEqual(found, [
            info.StaticVar('dir1/spam.c', None, 'var1', '???'),
            info.StaticVar('dir1/spam.c', None, 'var2', '???'),
            info.StaticVar('dir1/spam.c', None, 'var3', '???'),
            info.StaticVar('<???>', '<???>', 'var4-1', '???'),
            info.StaticVar('dir1/eggs.c', None, 'var1', '???'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_symbols', ()),
            ])

    def test_no_symbols(self):
        self._return_iter_symbols = []

        found = list(iter_statics(['dir1'],
                                  _iter_symbols=self._iter_symbols))

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [
            ('_iter_symbols', ()),
            ])
