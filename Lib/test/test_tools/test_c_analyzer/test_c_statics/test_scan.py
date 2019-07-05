import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info
    from c_statics.scan import iter_statics


class IterStaticsTests(unittest.TestCase):

    _return_iter_symbols = ()

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

    def _iter_symbols(self, dirnames):
        self.calls.append(('_iter_symbols', (dirnames,)))
        return iter(self. _return_iter_symbols)

    def test_typical(self):
        self._return_iter_symbols = [
                info.Symbol('var1', 'variable', False, 'dir1/spam.c', None, None),
                info.Symbol('var2', 'variable', False, 'dir1/spam.c', None, None),
                info.Symbol('func1', 'function', False, 'dir1/spam.c', None, None),
                info.Symbol('func2', 'function', True, 'dir1/spam.c', None, None),
                info.Symbol('var3', 'variable', False, 'dir1/spam.c', None, None),
                info.Symbol('var4', 'variable', False, 'dir1/spam.c', 'func2', None),
                info.Symbol('var1', 'variable', True, 'dir1/ham.c', None, None),
                info.Symbol('var1', 'variable', False, 'dir1/eggs.c', None, None),
                info.Symbol('xyz', 'other', False, 'dir1/eggs.c', None, None),
                info.Symbol('???', 'other', False, None, None, None),
                ]

        found = list(iter_statics(['dir1'],
                                  _iter_symbols=self._iter_symbols))

        self.assertEqual(found, [
            info.StaticVar('dir1/spam.c', None, 'var1', '???'),
            info.StaticVar('dir1/spam.c', None, 'var2', '???'),
            info.StaticVar('dir1/spam.c', None, 'var3', '???'),
            info.StaticVar('dir1/spam.c', 'func2', 'var4', '???'),
            info.StaticVar('dir1/eggs.c', None, 'var1', '???'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_symbols', (['dir1'],)),
            ])

    def test_no_symbols(self):
        self._return_iter_symbols = []

        found = list(iter_statics(['dir1'],
                                  _iter_symbols=self._iter_symbols))

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [
            ('_iter_symbols', (['dir1'],)),
            ])
