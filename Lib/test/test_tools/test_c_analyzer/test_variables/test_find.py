import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.variables import info
    from c_analyzer.variables.find import (
            vars_from_binary,
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


class VarsFromBinaryTests(_Base):

    _return_iter_vars = ()
    _return_get_symbol_resolver = None

    def setUp(self):
        super().setUp()

        self.kwargs = dict(
                _iter_vars=self._iter_vars,
                _get_symbol_resolver=self._get_symbol_resolver,
                )

    def _iter_vars(self, binfile, resolve, handle_id):
        self.calls.append(('_iter_vars', (binfile, resolve, handle_id)))
        return [(v, v.id) for v in self._return_iter_vars]

    def _get_symbol_resolver(self, known=None, dirnames=(), *,
                             handle_var,
                             filenames=None,
                             check_filename=None,
                             perfilecache=None,
                             ):
        self.calls.append(('_get_symbol_resolver',
                           (known, dirnames, handle_var, filenames,
                            check_filename, perfilecache)))
        return self._return_get_symbol_resolver

    def test_typical(self):
        resolver = self._return_get_symbol_resolver = object()
        variables = self._return_iter_vars = [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', 'int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', 'static int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', 'char *'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', 'const char *'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', 'static int'),
            info.Variable.from_parts('dir1/eggs.c', 'func1', 'var2', 'static char *'),
            ]
        known = object()
        filenames = object()

        found = list(vars_from_binary('python',
                                      known=known,
                                      filenames=filenames,
                                      **self.kwargs))

        self.assertEqual(found, [
            info.Variable.from_parts('dir1/spam.c', None, 'var1', 'int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var2', 'static int'),
            info.Variable.from_parts('dir1/spam.c', None, 'var3', 'char *'),
            info.Variable.from_parts('dir1/spam.c', 'func2', 'var4', 'const char *'),
            info.Variable.from_parts('dir1/eggs.c', None, 'var1', 'static int'),
            info.Variable.from_parts('dir1/eggs.c', 'func1', 'var2', 'static char *'),
            ])
        self.assertEqual(self.calls, [
            ('_get_symbol_resolver', (filenames, known, info.Variable.from_id, None, None, {})),
            ('_iter_vars', ('python', resolver, None)),
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
#        vars_from_binary('python', knownvars=known, **this.kwargs)
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
