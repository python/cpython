import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser import info
    from c_globals.find import globals_from_binary, globals


class _Base(unittest.TestCase):

    maxDiff = None

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls


class StaticsFromBinaryTests(_Base):

    _return_iter_symbols = ()
    _return_resolve_symbols = ()
    _return_get_symbol_resolver = None

    def setUp(self):
        super().setUp()

        self.kwargs = dict(
                _iter_symbols=self._iter_symbols,
                _resolve=self._resolve_symbols,
                _get_symbol_resolver=self._get_symbol_resolver,
                )

    def _iter_symbols(self, binfile, find_local_symbol):
        self.calls.append(('_iter_symbols', (binfile, find_local_symbol)))
        return self._return_iter_symbols

    def _resolve_symbols(self, symbols, resolve):
        self.calls.append(('_resolve_symbols', (symbols, resolve,)))
        return self._return_resolve_symbols

    def _get_symbol_resolver(self, knownvars, dirnames=None):
        self.calls.append(('_get_symbol_resolver', (knownvars, dirnames)))
        return self._return_get_symbol_resolver

    def test_typical(self):
        symbols = self._return_iter_symbols = ()
        resolver = self._return_get_symbol_resolver = object()
        variables = self._return_resolve_symbols = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', 'int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', 'static int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', 'char *'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', 'const char *'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', 'static int'),
            info.Variable.from_parts('dir1/eggs.c', 'func1', 'var2', 'static char *'),
            ]
        knownvars = object()

        found = list(globals_from_binary('python',
                                         knownvars=knownvars,
                                         **self.kwargs))

        self.assertEqual(found, [
            info.Variable.from_parts('dir1/spam.c', None, 'var2', 'static int'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', 'static int'),
            info.Variable.from_parts('dir1/eggs.c', 'func1', 'var2', 'static char *'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_symbols', ('python', None)),
            ('_get_symbol_resolver', (knownvars, None)),
            ('_resolve_symbols', (symbols, resolver)),
            ])

#        self._return_iter_symbols = [
#                s_info.Symbol(('dir1/spam.c', None, 'var1'), 'variable', False),
#                s_info.Symbol(('dir1/spam.c', None, 'var2'), 'variable', False),
#                s_info.Symbol(('dir1/spam.c', None, 'func1'), 'function', False),
#                s_info.Symbol(('dir1/spam.c', None, 'func2'), 'function', True),
#                s_info.Symbol(('dir1/spam.c', None, 'var3'), 'variable', False),
#                s_info.Symbol(('dir1/spam.c', 'func2', 'var4'), 'variable', False),
#                s_info.Symbol(('dir1/ham.c', None, 'var1'), 'variable', True),
#                s_info.Symbol(('dir1/eggs.c', None, 'var1'), 'variable', False),
#                s_info.Symbol(('dir1/eggs.c', None, 'xyz'), 'other', False),
#                s_info.Symbol(('dir1/eggs.c', '???', 'var2'), 'variable', False),
#                s_info.Symbol(('???', None, 'var_x'), 'variable', False),
#                s_info.Symbol(('???', '???', 'var_y'), 'variable', False),
#                s_info.Symbol((None, None, '???'), 'other', False),
#                ]
#        known = object()
#
#        globals_from_binary('python', knownvars=known, **this.kwargs)
#        found = list(globals_from_symbols(['dir1'], self.iter_symbols))
#
#        self.assertEqual(found, [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ])
#        self.assertEqual(self.calls, [
#            ('iter_symbols', (['dir1'],)),
#            ])
#
#    def test_no_symbols(self):
#        self._return_iter_symbols = []
#
#        found = list(globals_from_symbols(['dir1'], self.iter_symbols))
#
#        self.assertEqual(found, [])
#        self.assertEqual(self.calls, [
#            ('iter_symbols', (['dir1'],)),
#            ])

    # XXX need functional test


#class StaticFromDeclarationsTests(_Base):
#
#    _return_iter_declarations = ()
#
#    def iter_declarations(self, dirnames):
#        self.calls.append(('iter_declarations', (dirnames,)))
#        return iter(self._return_iter_declarations)
#
#    def test_typical(self):
#        self._return_iter_declarations = [
#            None,
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            object(),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            object(),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            object(),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            object(),
#            ]
#
#        found = list(globals_from_declarations(['dir1'], self.iter_declarations))
#
#        self.assertEqual(found, [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ])
#        self.assertEqual(self.calls, [
#            ('iter_declarations', (['dir1'],)),
#            ])
#
#    def test_no_declarations(self):
#        self._return_iter_declarations = []
#
#        found = list(globals_from_declarations(['dir1'], self.iter_declarations))
#
#        self.assertEqual(found, [])
#        self.assertEqual(self.calls, [
#            ('iter_declarations', (['dir1'],)),
#            ])


#class IterVariablesTests(_Base):
#
#    _return_from_symbols = ()
#    _return_from_declarations = ()
#
#    def _from_symbols(self, dirnames, iter_symbols):
#        self.calls.append(('_from_symbols', (dirnames, iter_symbols)))
#        return iter(self._return_from_symbols)
#
#    def _from_declarations(self, dirnames, iter_declarations):
#        self.calls.append(('_from_declarations', (dirnames, iter_declarations)))
#        return iter(self._return_from_declarations)
#
#    def test_typical(self):
#        expected = [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ]
#        self._return_from_symbols = expected
#
#        found = list(iter_variables(['dir1'],
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, expected)
#        self.assertEqual(self.calls, [
#            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
#            ])
#
#    def test_no_symbols(self):
#        self._return_from_symbols = []
#
#        found = list(iter_variables(['dir1'],
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, [])
#        self.assertEqual(self.calls, [
#            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
#            ])
#
#    def test_from_binary(self):
#        expected = [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ]
#        self._return_from_symbols = expected
#
#        found = list(iter_variables(['dir1'], 'platform',
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, expected)
#        self.assertEqual(self.calls, [
#            ('_from_symbols', (['dir1'], b_symbols.iter_symbols)),
#            ])
#
#    def test_from_symbols(self):
#        expected = [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ]
#        self._return_from_symbols = expected
#
#        found = list(iter_variables(['dir1'], 'symbols',
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, expected)
#        self.assertEqual(self.calls, [
#            ('_from_symbols', (['dir1'], s_symbols.iter_symbols)),
#            ])
#
#    def test_from_declarations(self):
#        expected = [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ]
#        self._return_from_declarations = expected
#
#        found = list(iter_variables(['dir1'], 'declarations',
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, expected)
#        self.assertEqual(self.calls, [
#            ('_from_declarations', (['dir1'], declarations.iter_all)),
#            ])
#
#    def test_from_preprocessed(self):
#        expected = [
#            info.Variable.from_parts('dir1/spam.c', None, 'var1', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var2', '???'),
#            info.Variable.from_parts('dir1/spam.c', None, 'var3', '???'),
#            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', '???'),
#            info.Variable.from_parts('dir1/eggs.c', None, 'var1', '???'),
#            ]
#        self._return_from_declarations = expected
#
#        found = list(iter_variables(['dir1'], 'preprocessed',
#                                  _from_symbols=self._from_symbols,
#                                  _from_declarations=self._from_declarations))
#
#        self.assertEqual(found, expected)
#        self.assertEqual(self.calls, [
#            ('_from_declarations', (['dir1'], declarations.iter_preprocessed)),
#            ])


class StaticsTest(_Base):

    _return_iter_variables = None

    def _iter_variables(self, kind, *, known, dirnames):
        self.calls.append(
                ('_iter_variables', (kind, known, dirnames)))
        return iter(self._return_iter_variables or ())

    def test_typical(self):
        self._return_iter_variables = [
            info.Variable.from_parts('src1/spam.c', None, 'var1', 'static const char *'),
            info.Variable.from_parts('src1/spam.c', None, 'var1b', 'const char *'),
            info.Variable.from_parts('src1/spam.c', 'ham', 'initialized', 'static int'),
            info.Variable.from_parts('src1/spam.c', 'ham', 'result', 'int'),
            info.Variable.from_parts('src1/spam.c', None, 'var2', 'static PyObject *'),
            info.Variable.from_parts('src1/eggs.c', 'tofu', 'ready', 'static int'),
            info.Variable.from_parts('src1/spam.c', None, 'freelist', 'static (PyTupleObject *)[10]'),
            info.Variable.from_parts('src1/sub/ham.c', None, 'var1', 'static const char const *'),
            info.Variable.from_parts('src2/jam.c', None, 'var1', 'static int'),
            info.Variable.from_parts('src2/jam.c', None, 'var2', 'static MyObject *'),
            info.Variable.from_parts('Include/spam.h', None, 'data', 'static const int'),
            ]
        dirnames = object()
        known = object()

        found = list(globals(dirnames, known,
                             kind='platform',
                             _iter_variables=self._iter_variables,
                             ))

        self.assertEqual(found, [
            info.Variable.from_parts('src1/spam.c', None, 'var1', 'static const char *'),
            info.Variable.from_parts('src1/spam.c', 'ham', 'initialized', 'static int'),
            info.Variable.from_parts('src1/spam.c', None, 'var2', 'static PyObject *'),
            info.Variable.from_parts('src1/eggs.c', 'tofu', 'ready', 'static int'),
            info.Variable.from_parts('src1/spam.c', None, 'freelist', 'static (PyTupleObject *)[10]'),
            info.Variable.from_parts('src1/sub/ham.c', None, 'var1', 'static const char const *'),
            info.Variable.from_parts('src2/jam.c', None, 'var1', 'static int'),
            info.Variable.from_parts('src2/jam.c', None, 'var2', 'static MyObject *'),
            info.Variable.from_parts('Include/spam.h', None, 'data', 'static const int'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_variables', ('platform', known, dirnames)),
            ])
