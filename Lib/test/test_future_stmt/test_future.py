# Test various flavors of legal and illegal future statements

import __future__
import ast
import unittest
from test.support import import_helper
from test.support.script_helper import spawn_python, kill_python
from textwrap import dedent
import os
import re
import sys

TOP_LEVEL_MSG = 'from __future__ imports must occur at the beginning of the file'

rx = re.compile(r'\((\S+).py, line (\d+)')

def get_error_location(msg):
    mo = rx.search(str(msg))
    return mo.group(1, 2)

class FutureTest(unittest.TestCase):

    def check_syntax_error(self, err, basename,
                           *,
                           lineno,
                           message=TOP_LEVEL_MSG, offset=1):
        if basename != '<string>':
            basename += '.py'

        self.assertEqual(f'{message} ({basename}, line {lineno})', str(err))
        self.assertEqual(os.path.basename(err.filename), basename)
        self.assertEqual(err.lineno, lineno)
        self.assertEqual(err.offset, offset)

    def assertSyntaxError(self, code,
                          *,
                          lineno=1,
                          message=TOP_LEVEL_MSG, offset=1,
                          parametrize_docstring=True):
        code = dedent(code.lstrip('\n'))
        for add_docstring in ([False, True] if parametrize_docstring else [False]):
            with self.subTest(code=code, add_docstring=add_docstring):
                if add_docstring:
                    code = '"""Docstring"""\n' + code
                    lineno += 1
                with self.assertRaises(SyntaxError) as cm:
                    exec(code)
                self.check_syntax_error(cm.exception, "<string>",
                                        lineno=lineno,
                                        message=message,
                                        offset=offset)

    def test_import_nested_scope_twice(self):
        # Import the name nested_scopes twice to trigger SF bug #407394
        with import_helper.CleanImport(
            'test.test_future_stmt.import_nested_scope_twice',
        ):
            from test.test_future_stmt import import_nested_scope_twice
        self.assertEqual(import_nested_scope_twice.result, 6)

    def test_nested_scope(self):
        with import_helper.CleanImport('test.test_future_stmt.nested_scope'):
            from test.test_future_stmt import nested_scope
        self.assertEqual(nested_scope.result, 6)

    def test_future_single_import(self):
        with import_helper.CleanImport(
            'test.test_future_stmt.test_future_single_import',
        ):
            from test.test_future_stmt import test_future_single_import  # noqa: F401

    def test_future_multiple_imports(self):
        with import_helper.CleanImport(
            'test.test_future_stmt.test_future_multiple_imports',
        ):
            from test.test_future_stmt import test_future_multiple_imports  # noqa: F401

    def test_future_multiple_features(self):
        with import_helper.CleanImport(
            "test.test_future_stmt.test_future_multiple_features",
        ):
            from test.test_future_stmt import test_future_multiple_features  # noqa: F401

    def test_unknown_future_flag(self):
        code = """
            from __future__ import nested_scopes
            from __future__ import rested_snopes  # typo error here: nested => rested
        """
        self.assertSyntaxError(
            code, lineno=2,
            message='future feature rested_snopes is not defined', offset=24,
        )

    def test_future_import_not_on_top(self):
        code = """
            import some_module
            from __future__ import annotations
        """
        self.assertSyntaxError(code, lineno=2)

        code = """
            import __future__
            from __future__ import annotations
        """
        self.assertSyntaxError(code, lineno=2)

        code = """
            from __future__ import absolute_import
            "spam, bar, blah"
            from __future__ import print_function
        """
        self.assertSyntaxError(code, lineno=3)

    def test_future_import_with_extra_string(self):
        code = """
            '''Docstring'''
            "this isn't a doc string"
            from __future__ import nested_scopes
        """
        self.assertSyntaxError(code, lineno=3, parametrize_docstring=False)

    def test_multiple_import_statements_on_same_line(self):
        # With `\`:
        code = """
            from __future__ import nested_scopes; import string; from __future__ import \
        nested_scopes
        """
        self.assertSyntaxError(code, offset=54)

        # Without `\`:
        code = """
            from __future__ import nested_scopes; import string; from __future__ import  nested_scopes
        """
        self.assertSyntaxError(code, offset=54)

    def test_future_import_star(self):
        code = """
            from __future__ import *
        """
        self.assertSyntaxError(code, message='future feature * is not defined', offset=24)

    def test_future_import_braces(self):
        code = """
            from __future__ import braces
        """
        # Congrats, you found an easter egg!
        self.assertSyntaxError(code, message='not a chance', offset=24)

        code = """
            from __future__ import nested_scopes, braces
        """
        self.assertSyntaxError(code, message='not a chance', offset=39)

    def test_module_with_future_import_not_on_top(self):
        with self.assertRaises(SyntaxError) as cm:
            from test.test_future_stmt import badsyntax_future  # noqa: F401
        self.check_syntax_error(cm.exception, "badsyntax_future", lineno=3)

    def test_ensure_flags_dont_clash(self):
        # bpo-39562: test that future flags and compiler flags doesn't clash

        # obtain future flags (CO_FUTURE_***) from the __future__ module
        flags = {
            f"CO_FUTURE_{future.upper()}": getattr(__future__, future).compiler_flag
            for future in __future__.all_feature_names
        }
        # obtain some of the exported compiler flags (PyCF_***) from the ast module
        flags |= {
            flag: getattr(ast, flag)
            for flag in dir(ast) if flag.startswith("PyCF_")
        }
        self.assertCountEqual(set(flags.values()), flags.values())

    def test_unicode_literals_exec(self):
        scope = {}
        exec("from __future__ import unicode_literals; x = ''", {}, scope)
        self.assertIsInstance(scope["x"], str)

    def test_syntactical_future_repl(self):
        p = spawn_python('-i')
        p.stdin.write(b"from __future__ import barry_as_FLUFL\n")
        p.stdin.write(b"2 <> 3\n")
        out = kill_python(p)
        self.assertNotIn(b'SyntaxError: invalid syntax', out)

    def test_future_dotted_import(self):
        with self.assertRaises(ImportError):
            exec("from .__future__ import spam")

        code = dedent(
            """
            from __future__ import print_function
            from ...__future__ import ham
            """
        )
        with self.assertRaises(ImportError):
            exec(code)

        code = """
            from .__future__ import nested_scopes
            from __future__ import barry_as_FLUFL
        """
        self.assertSyntaxError(code, lineno=2)

class AnnotationsFutureTestCase(unittest.TestCase):
    template = dedent(
        """
        from __future__ import annotations
        def f() -> {ann}:
            ...
        def g(arg: {ann}) -> None:
            ...
        async def f2() -> {ann}:
            ...
        async def g2(arg: {ann}) -> None:
            ...
        class H:
            var: {ann}
            object.attr: {ann}
        var: {ann}
        var2: {ann} = None
        object.attr: {ann}
        """
    )

    def getActual(self, annotation):
        scope = {}
        exec(self.template.format(ann=annotation), {}, scope)
        func_ret_ann = scope['f'].__annotations__['return']
        func_arg_ann = scope['g'].__annotations__['arg']
        async_func_ret_ann = scope['f2'].__annotations__['return']
        async_func_arg_ann = scope['g2'].__annotations__['arg']
        var_ann1 = scope['__annotations__']['var']
        var_ann2 = scope['__annotations__']['var2']
        self.assertEqual(func_ret_ann, func_arg_ann)
        self.assertEqual(func_ret_ann, async_func_ret_ann)
        self.assertEqual(func_ret_ann, async_func_arg_ann)
        self.assertEqual(func_ret_ann, var_ann1)
        self.assertEqual(func_ret_ann, var_ann2)
        return func_ret_ann

    def assertAnnotationEqual(
        self, annotation, expected=None, drop_parens=False, is_tuple=False,
    ):
        actual = self.getActual(annotation)
        if expected is None:
            expected = annotation if not is_tuple else annotation[1:-1]
        if drop_parens:
            self.assertNotEqual(actual, expected)
            actual = actual.replace("(", "").replace(")", "")

        self.assertEqual(actual, expected)

    def _exec_future(self, code):
        scope = {}
        exec(
            "from __future__ import annotations\n"
            + code, scope
        )
        return scope

    def test_annotations(self):
        eq = self.assertAnnotationEqual
        eq('...')
        eq("'some_string'")
        eq("u'some_string'")
        eq("b'\\xa3'")
        eq('Name')
        eq('None')
        eq('True')
        eq('False')
        eq('1')
        eq('1.0')
        eq('1j')
        eq('True or False')
        eq('True or False or None')
        eq('True and False')
        eq('True and False and None')
        eq('Name1 and Name2 or Name3')
        eq('Name1 and (Name2 or Name3)')
        eq('Name1 or Name2 and Name3')
        eq('(Name1 or Name2) and Name3')
        eq('Name1 and Name2 or Name3 and Name4')
        eq('Name1 or Name2 and Name3 or Name4')
        eq('a + b + (c + d)')
        eq('a * b * (c * d)')
        eq('(a ** b) ** c ** d')
        eq('v1 << 2')
        eq('1 >> v2')
        eq('1 % finished')
        eq('1 + v2 - v3 * 4 ^ 5 ** v6 / 7 // 8')
        eq('not great')
        eq('not not great')
        eq('~great')
        eq('+value')
        eq('++value')
        eq('-1')
        eq('~int and not v1 ^ 123 + v2 | True')
        eq('a + (not b)')
        eq('lambda: None')
        eq('lambda arg: None')
        eq('lambda a=True: a')
        eq('lambda a, b, c=True: a')
        eq("lambda a, b, c=True, *, d=1 << v2, e='str': a")
        eq("lambda a, b, c=True, *vararg, d, e='str', **kwargs: a + b")
        eq("lambda a, /, b, c=True, *vararg, d, e='str', **kwargs: a + b")
        eq('lambda x, /: x')
        eq('lambda x=1, /: x')
        eq('lambda x, /, y: x + y')
        eq('lambda x=1, /, y=2: x + y')
        eq('lambda x, /, y=1: x + y')
        eq('lambda x, /, y=1, *, z=3: x + y + z')
        eq('lambda x=1, /, y=2, *, z=3: x + y + z')
        eq('lambda x=1, /, y=2, *, z: x + y + z')
        eq('lambda x=1, y=2, z=3, /, w=4, *, l, l2: x + y + z + w + l + l2')
        eq('lambda x=1, y=2, z=3, /, w=4, *, l, l2, **kwargs: x + y + z + w + l + l2')
        eq('lambda x, /, y=1, *, z: x + y + z')
        eq('lambda x: lambda y: x + y')
        eq('1 if True else 2')
        eq('str or None if int or True else str or bytes or None')
        eq('str or None if (1 if True else 2) else str or bytes or None')
        eq("0 if not x else 1 if x > 0 else -1")
        eq("(1 if x > 0 else -1) if x else 0")
        eq("{'2.7': dead, '3.7': long_live or die_hard}")
        eq("{'2.7': dead, '3.7': long_live or die_hard, **{'3.6': verygood}}")
        eq("{**a, **b, **c}")
        eq("{'2.7', '3.6', '3.7', '3.8', '3.9', '4.0' if gilectomy else '3.10'}")
        eq("{*a, *b, *c}")
        eq("({'a': 'b'}, True or False, +value, 'string', b'bytes') or None")
        eq("()")
        eq("(a,)")
        eq("(a, b)")
        eq("(a, b, c)")
        eq("(*a, *b, *c)")
        eq("[]")
        eq("[1, 2, 3, 4, 5, 6, 7, 8, 9, 10 or A, 11 or B, 12 or C]")
        eq("[*a, *b, *c]")
        eq("{i for i in (1, 2, 3)}")
        eq("{i ** 2 for i in (1, 2, 3)}")
        eq("{i ** 2 for i, _ in ((1, 'a'), (2, 'b'), (3, 'c'))}")
        eq("{i ** 2 + j for i in (1, 2, 3) for j in (1, 2, 3)}")
        eq("[i for i in (1, 2, 3)]")
        eq("[i ** 2 for i in (1, 2, 3)]")
        eq("[i ** 2 for i, _ in ((1, 'a'), (2, 'b'), (3, 'c'))]")
        eq("[i ** 2 + j for i in (1, 2, 3) for j in (1, 2, 3)]")
        eq("(i for i in (1, 2, 3))")
        eq("(i ** 2 for i in (1, 2, 3))")
        eq("(i ** 2 for i, _ in ((1, 'a'), (2, 'b'), (3, 'c')))")
        eq("(i ** 2 + j for i in (1, 2, 3) for j in (1, 2, 3))")
        eq("{i: 0 for i in (1, 2, 3)}")
        eq("{i: j for i, j in ((1, 'a'), (2, 'b'), (3, 'c'))}")
        eq("[(x, y) for x, y in (a, b)]")
        eq("[(x,) for x, in (a,)]")
        eq("Python3 > Python2 > COBOL")
        eq("Life is Life")
        eq("call()")
        eq("call(arg)")
        eq("call(kwarg='hey')")
        eq("call(arg, kwarg='hey')")
        eq("call(arg, *args, another, kwarg='hey')")
        eq("call(arg, another, kwarg='hey', **kwargs, kwarg2='ho')")
        eq("lukasz.langa.pl")
        eq("call.me(maybe)")
        eq("1 .real")
        eq("1.0.real")
        eq("....__class__")
        eq("list[str]")
        eq("dict[str, int]")
        eq("set[str,]")
        eq("tuple[()]")
        eq("tuple[str, ...]")
        eq("tuple[str, *types]")
        eq("tuple[str, int, (str, int)]")
        eq("tuple[*int, str, str, (str, int)]")
        eq("tuple[str, int, float, dict[str, int]]")
        eq("slice[0]")
        eq("slice[0:1]")
        eq("slice[0:1:2]")
        eq("slice[:]")
        eq("slice[:-1]")
        eq("slice[1:]")
        eq("slice[::-1]")
        eq("slice[:,]")
        eq("slice[1:2,]")
        eq("slice[1:2:3,]")
        eq("slice[1:2, 1]")
        eq("slice[1:2, 2, 3]")
        eq("slice[()]")
        # Note that `slice[*Ts]`, `slice[*Ts,]`, and `slice[(*Ts,)]` all have
        # the same AST, but only `slice[*Ts,]` passes this test, because that's
        # what the unparser produces.
        eq("slice[*Ts,]")
        eq("slice[1, *Ts]")
        eq("slice[*Ts, 2]")
        eq("slice[1, *Ts, 2]")
        eq("slice[*Ts, *Ts]")
        eq("slice[1, *Ts, *Ts]")
        eq("slice[*Ts, 1, *Ts]")
        eq("slice[*Ts, *Ts, 1]")
        eq("slice[1, *Ts, *Ts, 2]")
        eq("slice[1:2, *Ts]")
        eq("slice[*Ts, 1:2]")
        eq("slice[1:2, *Ts, 3:4]")
        eq("slice[a, b:c, d:e:f]")
        eq("slice[(x for x in a)]")
        eq('str or None if sys.version_info[0] > (3,) else str or bytes or None')
        eq("f'f-string without formatted values is just a string'")
        eq("f'{{NOT a formatted value}}'")
        eq("f'some f-string with {a} {few():.2f} {formatted.values!r}'")
        eq('''f"{f'{nested} inner'} outer"''')
        eq("f'space between opening braces: { {a for a in (1, 2, 3)}}'")
        eq("f'{(lambda x: x)}'")
        eq("f'{(None if a else lambda x: x)}'")
        eq("f'{x}'")
        eq("f'{x!r}'")
        eq("f'{x!a}'")
        eq('[x for x in (a if b else c)]')
        eq('[x for x in a if (b if c else d)]')
        eq('f(x for x in a)')
        eq('f(1, (x for x in a))')
        eq('f((x for x in a), 2)')
        eq('(((a)))', 'a')
        eq('(((a, b)))', '(a, b)')
        eq("1 + 2 + 3")
        eq("t''")
        eq("t'{a    +  b}'")
        eq("t'{a!s}'")
        eq("t'{a:b}'")
        eq("t'{a:b=}'")

    def test_fstring_debug_annotations(self):
        # f-strings with '=' don't round trip very well, so set the expected
        # result explicitly.
        self.assertAnnotationEqual("f'{x=!r}'", expected="f'x={x!r}'")
        self.assertAnnotationEqual("f'{x=:}'", expected="f'x={x:}'")
        self.assertAnnotationEqual("f'{x=:.2f}'", expected="f'x={x:.2f}'")
        self.assertAnnotationEqual("f'{x=!r}'", expected="f'x={x!r}'")
        self.assertAnnotationEqual("f'{x=!a}'", expected="f'x={x!a}'")
        self.assertAnnotationEqual("f'{x=!s:*^20}'", expected="f'x={x!s:*^20}'")

    def test_infinity_numbers(self):
        inf = "1e" + repr(sys.float_info.max_10_exp + 1)
        infj = f"{inf}j"
        self.assertAnnotationEqual("1e1000", expected=inf)
        self.assertAnnotationEqual("1e1000j", expected=infj)
        self.assertAnnotationEqual("-1e1000", expected=f"-{inf}")
        self.assertAnnotationEqual("3+1e1000j", expected=f"3 + {infj}")
        self.assertAnnotationEqual("(1e1000, 1e1000j)", expected=f"({inf}, {infj})")
        self.assertAnnotationEqual("'inf'")
        self.assertAnnotationEqual("('inf', 1e1000, 'infxxx', 1e1000j)", expected=f"('inf', {inf}, 'infxxx', {infj})")
        self.assertAnnotationEqual("(1e1000, (1e1000j,))", expected=f"({inf}, ({infj},))")

    def test_annotation_with_complex_target(self):
        with self.assertRaises(SyntaxError):
            exec(
                "from __future__ import annotations\n"
                "object.__debug__: int"
            )

    def test_annotations_symbol_table_pass(self):
        namespace = self._exec_future(dedent("""
        from __future__ import annotations

        def foo():
            outer = 1
            def bar():
                inner: outer = 1
            return bar
        """))

        foo = namespace.pop("foo")
        self.assertIsNone(foo().__closure__)
        self.assertEqual(foo.__code__.co_cellvars, ())
        self.assertEqual(foo().__code__.co_freevars, ())

    def test_annotations_forbidden(self):
        with self.assertRaises(SyntaxError):
            self._exec_future("test: (yield)")

        with self.assertRaises(SyntaxError):
            self._exec_future("test.test: (yield a + b)")

        with self.assertRaises(SyntaxError):
            self._exec_future("test[something]: (yield from x)")

        with self.assertRaises(SyntaxError):
            self._exec_future("def func(test: (yield from outside_of_generator)): pass")

        with self.assertRaises(SyntaxError):
            self._exec_future("def test() -> (await y): pass")

        with self.assertRaises(SyntaxError):
            self._exec_future("async def test() -> something((a := b)): pass")

        with self.assertRaises(SyntaxError):
            self._exec_future("test: await some.complicated[0].call(with_args=True or 1 is not 1)")

        with self.assertRaises(SyntaxError):
            self._exec_future("test: f'{(x := 10):=10}'")

        with self.assertRaises(SyntaxError):
            self._exec_future(dedent("""\
            def foo():
                def bar(arg: (yield)): pass
            """))

    def test_get_type_hints_on_func_with_variadic_arg(self):
        # `typing.get_type_hints` might break on a function with a variadic
        # annotation (e.g. `f(*args: *Ts)`) if `from __future__ import
        # annotations`, because it could try to evaluate `*Ts` as an expression,
        # which on its own isn't value syntax.
        namespace = self._exec_future(dedent("""\
        class StarredC: pass
        class C:
          def __iter__(self):
            yield StarredC()
        c = C()
        def f(*args: *c): pass
        import typing
        hints = typing.get_type_hints(f)
        """))

        hints = namespace.pop('hints')
        self.assertIsInstance(hints['args'], namespace['StarredC'])


if __name__ == "__main__":
    unittest.main()
