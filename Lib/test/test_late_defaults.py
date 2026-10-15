"""Behavioral tests for the experimental PEP 671 implementation."""

import ast
import copy
import functools
import gc
import inspect
import marshal
import pickle
import unittest
import weakref


class LateDefaultsTests(unittest.TestCase):
    def test_fresh_mutable_default(self):
        def add(item, target=>[]):
            target.append(item)
            return target
        a, b = add(1), add(2)
        self.assertEqual(a, [1])
        self.assertEqual(b, [2])
        self.assertIsNot(a, b)
        target = []
        self.assertIs(add(3, target), target)

    def test_definition_does_not_evaluate(self):
        calls = []
        def f(x=>calls.append('default')):
            return x
        self.assertEqual(calls, [])
        self.assertEqual(f(12), 12)
        self.assertEqual(calls, [])
        self.assertIsNone(f())
        self.assertEqual(calls, ['default'])

    def test_explicit_values_including_ellipsis(self):
        def f(a=>1, *, b=>2):
            return a, b
        for value in [None, False, 0, '', [], Ellipsis]:
            with self.subTest(value=value):
                a, b = f(value, b=value)
                self.assertIs(a, value)
                self.assertIs(b, value)
        self.assertEqual(f(), (1, 2))

    def test_left_to_right(self):
        calls = []
        def mark(n):
            calls.append(n)
            return n
        def f(a=>mark(1), b=>mark(a + 1), *, c=>mark(b + 1)):
            return a, b, c
        self.assertEqual(f(), (1, 2, 3))
        self.assertEqual(calls, [1, 2, 3])
        calls.clear()
        self.assertEqual(f(c=20, a=4), (4, 5, 20))
        self.assertEqual(calls, [5])

    def test_mixed_binding_times(self):
        calls = []
        def f(a=calls.append('early'), b=>calls.append('late')):
            return a, b
        self.assertEqual(calls, ['early'])
        self.assertEqual(f(), (None, None))
        self.assertEqual(f(), (None, None))
        self.assertEqual(calls, ['early', 'late', 'late'])

    def test_parameter_kinds(self):
        def f(a, b=>len(a), /, c=>b + 1, *rest, d=>c + 1, **kw):
            return b, c, rest, d, kw
        self.assertEqual(f([1, 2]), (2, 3, (), 4, {}))
        self.assertEqual(f([], 5, 8, 9, d=10, x=11), (5, 8, (9,), 10, {'x': 11}))
        self.assertEqual(f([], **{'b': 30}), (0, 1, (), 2, {'b': 30}))

    def test_keyword_only_without_default(self):
        def f(*, required, x=>required + 1):
            return x
        self.assertEqual(f(required=3), 4)
        with self.assertRaises(TypeError):
            f()

    def test_runtime_scope(self):
        x = 100
        def f(x, y=>x):
            return y
        self.assertEqual(f(2), 2)
        def g(x=>x):
            return x
        with self.assertRaises(UnboundLocalError):
            g()
        self.assertEqual(g(3), 3)

    def test_forward_reference(self):
        def f(a=>b, b=>2):
            return a, b
        with self.assertRaises(UnboundLocalError):
            f()
        self.assertEqual(f(b=9), (9, 9))
        def g(a=>b, b=4):
            return a, b
        self.assertEqual(g(), (4, 4))

    def test_closure_is_read_at_call_time(self):
        x = 1
        def f(a=>x):
            return a
        self.assertEqual(f(), 1)
        x = 2
        self.assertEqual(f(), 2)

    def test_cell_parameter(self):
        def f(a=>[], b=>a):
            def inner():
                return a
            return inner, b
        inner, b = f()
        self.assertIs(inner(), b)
        value = object()
        inner, b = f(value)
        self.assertIs(inner(), value)
        self.assertIs(b, value)

    def test_self_reference_cell(self):
        def f(a=>a):
            return lambda: a
        with self.assertRaises(UnboundLocalError):
            f()
        self.assertEqual(f(4)(), 4)

    def test_lambdas_and_nested_scopes(self):
        f = lambda a=>[], *, b=>len(a): (a, b)
        self.assertEqual(f(), ([], 0))
        self.assertEqual(f([1, 2]), ([1, 2], 2))
        def g(a=>3, b=>lambda: a):
            return b()
        self.assertEqual(g(), 3)

    def test_comprehension(self):
        def f(n, values=>[i * n for i in range(n)]):
            return values
        self.assertEqual(f(3), [0, 3, 6])

    def test_exception(self):
        def f(x=>1 / 0):
            return x
        with self.assertRaises(ZeroDivisionError):
            f()
        self.assertEqual(f(3), 3)

    def test_argument_errors_precede_evaluation(self):
        calls = []
        def f(required, x=>calls.append(1)):
            return x
        for args, kw in [((), {}), ((1, 2, 3), {}), ((1,), {'unknown': 2}),
                         ((1, 2), {'x': 3})]:
            with self.subTest(args=args, kw=kw):
                with self.assertRaises(TypeError):
                    f(*args, **kw)
        self.assertEqual(calls, [])

    def test_method_and_mangling(self):
        class C:
            def f(self, __x=>4, *, __y=>5):
                return __x, __y
            def owner(self, cls=>__class__):
                return cls
        c = C()
        self.assertEqual(c.f(), (4, 5))
        self.assertEqual(c.f(_C__x=6, _C__y=7), (6, 7))
        self.assertIs(c.owner(), C)

    def test_generic(self):
        def f[T](x: T, items=>[x]):
            return items
        self.assertEqual(f(3), [3])

    def test_docstring(self):
        def f(x=>'default'):
            """Function documentation."""
            return x
        self.assertEqual(f.__doc__, 'Function documentation.')
        self.assertEqual(f(), 'default')

    def test_coroutine(self):
        async def f(x=>[]):
            return x
        def run():
            coro = f()
            try:
                coro.send(None)
            except StopIteration as e:
                return e.value
            finally:
                coro.close()
        a, b = run(), run()
        self.assertEqual(a, [])
        self.assertIsNot(a, b)

    def test_generator(self):
        def f(x=>[]):
            yield x
        a, b = next(f()), next(f())
        self.assertEqual(a, [])
        self.assertIsNot(a, b)

    def test_specialized_call_sites(self):
        def f(x=>[], *, y=>[]):
            return x, y
        def call():
            return f()
        for _ in range(1000):
            a, b = call()
            self.assertEqual(a, [])
            self.assertIsNot(a, b)

    def test_c_vectorcall(self):
        from test.support import import_helper
        _testcapi = import_helper.import_module('_testcapi')
        def f(a=>[], *, b=>{}):
            return a, b
        self.assertEqual(_testcapi.pyobject_vectorcall(f, None, None), ([], {}))
        value = object()
        a, b = _testcapi.pyobject_vectorcall(f, (value, value), ('b',))
        self.assertIs(a, value)
        self.assertIs(b, value)

    def test_missing_keyword_error_names(self):
        def f(*, a, b=>[], c):
            pass
        with self.assertRaisesRegex(TypeError, "2 required keyword-only arguments: 'a' and 'c'"):
            f()

    def test_generic_with_both_default_kinds(self):
        def f[T](a=1, b=>a + 1, *, c=3, d=>b + c):
            return a, b, c, d
        self.assertEqual(f(), (1, 2, 3, 5))
        self.assertEqual(f(5, d=20), (5, 6, 3, 20))

    def test_keyword_cell_parameter(self):
        def f(*, x=>[]):
            return lambda: x
        a, b = f()(), f()()
        self.assertEqual(a, [])
        self.assertIsNot(a, b)

    def test_partial(self):
        def f(a=>1, b=>a + 1):
            return a, b
        self.assertEqual(functools.partial(f, b=9)(), (1, 9))
        self.assertEqual(functools.partial(f, 4)(), (4, 5))

    def test_many_parameters(self):
        names = [f'x{i}' for i in range(75)]
        namespace = {}
        exec('def f(' + ', '.join(n + '=>[]' for n in names) + '):\n'
             '    return (' + ', '.join(names) + ')', namespace)
        result = namespace['f']()
        self.assertEqual(len(result), 75)
        self.assertTrue(all(value == [] for value in result))
        self.assertEqual(len({id(value) for value in result}), 75)

    def test_metadata_and_signature(self):
        def f(a=..., b=>[], *, c=>len(b), d=...):
            pass
        self.assertEqual(f.__defaults__, (..., ...))
        self.assertEqual(f.__defaults_extra__, (None, '[]'))
        self.assertEqual(f.__kwdefaults_extra__, {'c': 'len(b)'})
        sig = inspect.signature(f)
        self.assertEqual(str(sig), '(a=Ellipsis, b=>[], *, c=>len(b), d=Ellipsis)')
        self.assertEqual(sig.parameters['b'].late_default, '[]')
        self.assertIsNone(sig.parameters['a'].late_default)
        self.assertEqual(len(inspect.getfullargspec(f)), 7)

    def test_signature_copy_and_pickle(self):
        def f(x=>[]):
            pass
        sig = inspect.signature(f)
        parameter = sig.parameters['x']
        self.assertEqual(parameter.replace(), parameter)
        self.assertEqual(copy.copy(parameter), parameter)
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            self.assertEqual(pickle.loads(pickle.dumps(sig, protocol)), sig)
        early = parameter.replace(late_default=None)
        self.assertNotEqual(early, parameter)

    def test_mutated_defaults(self):
        def f(x=>[], *, y=>{}):
            return x, y
        f.__defaults__ = (7,)
        f.__kwdefaults__ = {'y': 8}
        self.assertEqual(f(), (7, 8))
        f.__defaults__ = (...,)
        f.__kwdefaults__ = {'y': ...}
        self.assertEqual(f(), ([], {}))
        f.__defaults_extra__ = None
        f.__kwdefaults_extra__ = None
        self.assertEqual(f(), (..., ...))

    def test_metadata_type_checks(self):
        def f(x=>[]):
            pass
        with self.assertRaises(TypeError):
            f.__defaults_extra__ = []
        with self.assertRaises(TypeError):
            f.__kwdefaults_extra__ = ()
        del f.__defaults_extra__
        self.assertIsNone(f.__defaults_extra__)

    def test_ast_and_unparse(self):
        source = 'def f(a=>[], /, b=1, *, c=>len(a)):\n    return a, b, c'
        tree = ast.parse(source)
        args = tree.body[0].args
        self.assertEqual(args.posonlyargs[0].late_default, '[]')
        self.assertIsNone(args.args[0].late_default)
        self.assertEqual(args.kwonlyargs[0].late_default, 'len(a)')
        namespace = {}
        exec(compile(tree, '<ast>', 'exec'), namespace)
        self.assertEqual(namespace['f'](), ([], 1, 0))
        unparsed = ast.unparse(tree)
        self.assertIn('a=>[]', unparsed)
        self.assertIn('c=>len(a)', unparsed)
        self.assertTrue(ast.compare(tree, ast.parse(unparsed)))
        expression = ast.parse('lambda x=>[]: x', mode='eval')
        self.assertEqual(ast.unparse(expression), 'lambda x=>[]: x')

    def test_invalid_ast(self):
        tree = ast.parse('def f(x): pass')
        tree.body[0].args.args[0].late_default = '[]'
        with self.assertRaisesRegex(ValueError, 'no default'):
            compile(tree, '<ast>', 'exec')
        tree = ast.parse('def f(*args): pass')
        tree.body[0].args.vararg.late_default = '[]'
        with self.assertRaisesRegex(ValueError, 'variadic'):
            compile(tree, '<ast>', 'exec')

    def test_source_survives_constant_folding(self):
        def f(x=>1 + 2, *, y=>3 * 4):
            return x, y
        self.assertEqual(f(), (3, 12))
        self.assertEqual(f.__defaults_extra__, ('1 + 2',))
        self.assertEqual(f.__kwdefaults_extra__, {'y': '3 * 4'})

    def test_metadata_on_ordinary_function(self):
        def f(x=..., *, y=...):
            return x, y
        f.__defaults_extra__ = ('[]',)
        f.__kwdefaults_extra__ = {'y': '{}'}
        self.assertEqual(f(), (..., ...))

    def test_code_replacement(self):
        def f(x=>[]):
            return x
        def g(x):
            return x
        f.__code__ = g.__code__
        self.assertIs(f(), Ellipsis)

    def test_type_comment(self):
        tree = ast.parse('def f(x=>[], # type: list\n): pass', type_comments=True)
        self.assertEqual(tree.body[0].args.args[0].late_default, '[]')
        self.assertEqual(tree.body[0].args.args[0].type_comment, 'list')

    def test_invalid_syntax(self):
        for source in ['def f(x=>): pass', 'def f(x=>1, y): pass',
                       'def f(*args=>[]): pass', 'def f(**kw=>{}): pass',
                       'lambda x=>: x', 'lambda x=>1, y: x',
                       'lambda x: int=>1, y: x']:
            with self.subTest(source=source), self.assertRaises(SyntaxError):
                compile(source, '<syntax>', 'exec')

    def test_marshal_roundtrip(self):
        namespace = {}
        code = compile('def f(x=>[]): return x', '<marshal>', 'exec')
        exec(marshal.loads(marshal.dumps(code)), namespace)
        a, b = namespace['f'](), namespace['f']()
        self.assertEqual(a, [])
        self.assertIsNot(a, b)

    def test_concurrent_calls(self):
        from test.support import threading_helper
        threading_helper.requires_working_threading()(lambda: None)()
        import threading
        results = []
        def f(x=>[], *, y=>{}):
            return x, y
        def worker():
            results.append([f() for _ in range(100)])
        threads = [threading.Thread(target=worker) for _ in range(4)]
        with threading_helper.start_threads(threads):
            pass
        values = [value for group in results for pair in group for value in pair]
        self.assertEqual(len(values), 800)
        self.assertEqual(len({id(value) for value in values}), 800)

    def test_gc_cycle(self):
        def create():
            def f(x=>f):
                return x
            return weakref.ref(f)
        ref = create()
        gc.collect()
        self.assertIsNone(ref())


if __name__ == '__main__':
    unittest.main()
