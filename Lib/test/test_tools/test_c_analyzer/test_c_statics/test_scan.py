import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info, declarations
    from c_statics.scan import (
            iter_statics, statics_from_symbols, statics_from_declarations
            )
    from c_symbols import (
            info as s_info,
            binary as b_symbols,
            source as s_symbols,
            )


class _Base(unittest.TestCase):

    maxDiff = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls


class StaticsFromSymbolsTests(_Base):

    _return_iter_symbols = ()

    def iter_symbols(self, dirnames):
        self.calls.append(('iter_symbols', (dirnames,)))
        return iter(self._return_iter_symbols)

    def test_typical(self):
        self._return_iter_symbols = [
                s_info.Symbol(('dir1/spam.c', None, 'var1'), 'variable', False),
                s_info.Symbol(('dir1/spam.c', None, 'var2'), 'variable', False),
                s_info.Symbol(('dir1/spam.c', None, 'func1'), 'function', False),
                s_info.Symbol(('dir1/spam.c', None, 'func2'), 'function', True),
                s_info.Symbol(('dir1/spam.c', None, 'var3'), 'variable', False),
                s_info.Symbol(('dir1/spam.c', 'func2', 'var4'), 'variable', False),
                s_info.Symbol(('dir1/ham.c', None, 'var1'), 'variable', True),
                s_info.Symbol(('dir1/eggs.c', None, 'var1'), 'variable', False),
                s_info.Symbol(('dir1/eggs.c', None, 'xyz'), 'other', False),
                s_info.Symbol((None, None, '???'), 'other', False),
                ]

        found = list(statics_from_symbols(['dir1'], self.iter_symbols))

        self.assertEqual(found, [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ])
        self.assertEqual(self.calls, [
            ('iter_symbols', (['dir1'],)),
            ])

    def test_no_symbols(self):
        self._return_iter_symbols = []

        found = list(statics_from_symbols(['dir1'], self.iter_symbols))

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [
            ('iter_symbols', (['dir1'],)),
            ])


class StaticFromDeclarationsTests(_Base):

    _return_iter_declarations = ()

    def iter_declarations(self, dirnames):
        self.calls.append(('iter_declarations', (dirnames,)))
        return iter(self._return_iter_declarations)

    def test_typical(self):
        self._return_iter_declarations = [
            None,
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            object(),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            object(),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            object(),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            object(),
            ]

        found = list(statics_from_declarations(['dir1'], self.iter_declarations))

        self.assertEqual(found, [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ])
        self.assertEqual(self.calls, [
            ('iter_declarations', (['dir1'],)),
            ])

    def test_no_declarations(self):
        self._return_iter_declarations = []

        found = list(statics_from_declarations(['dir1'], self.iter_declarations))

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [
            ('iter_declarations', (['dir1'],)),
            ])


class IterStaticsTests(_Base):

    _return_from_symbols = ()
    _return_from_declarations = ()

    def _from_symbols(self, dirnames, iter_symbols):
        self.calls.append(('_from_symbols', (dirnames, iter_symbols)))
        return iter(self._return_from_symbols)

    def _from_declarations(self, dirnames, iter_declarations):
        self.calls.append(('_from_declarations', (dirnames, iter_declarations)))
        return iter(self._return_from_declarations)

    def test_typical(self):
        expected = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ]
        self._return_from_symbols = expected

        found = list(iter_statics(['dir1'],
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
            ])

    def test_no_symbols(self):
        self._return_from_symbols = []

        found = list(iter_statics(['dir1'],
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, [])
        self.assertEqual(self.calls, [
            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
            ])

    def test_from_binary(self):
        expected = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ]
        self._return_from_symbols = expected

        found = list(iter_statics(['dir1'], 'platform',
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
            ])

    def test_from_symbols(self):
        expected = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ]
        self._return_from_symbols = expected

        found = list(iter_statics(['dir1'], 'symbols',
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_from_symbols', (['dir1'], s_symbols.iter_symbols)),
            ])

    def test_from_declarations(self):
        expected = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ]
        self._return_from_declarations = expected

        found = list(iter_statics(['dir1'], 'declarations',
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_from_declarations', (['dir1'], declarations.iter_all)),
            ])

    def test_from_preprocessed(self):
        expected = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
            ]
        self._return_from_declarations = expected

        found = list(iter_statics(['dir1'], 'preprocessed',
                                  _from_symbols=self._from_symbols,
                                  _from_declarations=self._from_declarations))

        self.assertEqual(found, expected)
        self.assertEqual(self.calls, [
            ('_from_declarations', (['dir1'], declarations.iter_preprocessed)),
            ])
