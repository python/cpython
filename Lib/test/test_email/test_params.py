import re
import unittest
from contextlib import contextmanager
from test.support import captured_stdout
from test.test_email.params import (
    add_label,
    as_value,
    C,
    for_each_function,
    for_each_name,
    fmt,
    fmtall,
    include_if,
    NameList,
    include_unless,
    only,
    params,
    Params,
    params_map,
    ParamsMixin,
    with_names,
    )
from textwrap import dedent

TYPED_VALUES = with_names(int=1, dict=dict(a=1), C=C(C(1)), tuple=(1, 2))

class AssertMixin:

    @contextmanager
    def assertRaisesRegexEx(self, ex, re, cause_ex=None, cause_re=None):
        with super().assertRaisesRegex(ex, re) as cm:
            yield
        if cause_ex:
            self.assertIsNotNone(cm.exception.__cause__)
            self.assertIsInstance(cm.exception.__cause__, cause_ex)
            self.assertRegex(str(cm.exception.__cause__), cause_re)


# We eat our own dogfood here, which could make bugs a bit confusing to sort
# out.  But it exercises the machinery pretty well, and demonstrates the power
# of the framework.  And we get much better test coverage out of it.


class TestFmt(ParamsMixin, unittest.TestCase):

    test_strings = (
        ('',                '',                 ''              ),
        ('no sub point',    'no sub point',     'no sub point'  ),
        ('sub an {a}',      'sub an a',         'sub an a'      ),
        ('{a} and {foo!r}', "a and {foo!r}",    "a and 'foo'"   ),
        ('{x} {y} {z:02}',  '{x} 2 {z:02}',     '1 2 03'        ),
        )
    sub = (dict(),          dict(a='a', y=2),   dict(foo='foo', x=1, z=3))
    expected = list(zip(*test_strings))
    make_list = lambda v: [C(1), *v, [1, 2, 3]]
    make_dict = lambda v: dict(
        {f'v{i}': v for i, v in enumerate(v)}, foo=C(1), bar=[1, 2, 3],
        )

    substitution_cases = Params(
        no_subs =   C(expected[0],  dict(),                     expected[0]),
        non_subs =  C(expected[0],  dict(zzz='foo', yyy='bar'), expected[0]),
        one_sub =   C(expected[0],  sub[1],                     expected[1]),
        all_sub =   C(expected[0],  sub[1] | sub[2],            expected[2]),
        )

    @params(
        for_each_function(fmt, fmtall)(
            params_map(lambda obj, subs, expected, ml=make_list, md=make_dict:
                [
                    ('list', C(ml(obj), subs, ml(expected))),
                    ('dict', C(md(obj), subs, md(expected))),
                    ]
                )(substitution_cases),
            )
        )
    def test(self, fmter, obj, subs, expected):
        self.assertEqual(fmter(obj, subs), expected)

    @params(
        fmt_list =      C(fmt,      make_list),
        fmtall_list =   C(fmtall,   make_list),
        fmt_dict =      C(fmt,      make_dict),
        fmtall_dict =   C(fmtall,   make_dict),
        )
    def test_multiple_passes(self, fmter, maker):
        unmodified = maker(self.expected[0])
        v0 = fmter(unmodified, dict())
        self.assertEqual(v0, unmodified)
        v1 = fmter(v0, dict(foobar=99) | self.sub[1])
        self.assertEqual(v1, maker(self.expected[1]))
        v2 = fmter(v1, dict(foobar=99, y=9) | self.sub[2])
        self.assertEqual(v2, maker(self.expected[2]))

    nested = [('{a}', '{b}'), dict(a='{a}', b=['{b}']), [[1, ['{b}']]]]
    nested_subs = dict(a=1, b=2)
    nested_subbed = [('1', '2'), dict(a='1', b=['2']), [[1, ['2']]]]

    def test_fmt_does_not_recurse(self):
        self.assertEqual(fmt(self.nested, self.nested_subs), self.nested)

    def test_fmtall_recurses(self):
        self.assertEqual(
            fmtall(self.nested, self.nested_subs),
            self.nested_subbed,
            )


class TestC(ParamsMixin, unittest.TestCase):

    def test_empty_C(self):
        p = C()
        self.assertEqual(p.args, tuple())
        self.assertEqual(p.kw, {})

    def test_args_only(self):
        p = C('a', 2)
        self.assertEqual(p.args, ('a', 2))
        self.assertEqual(p.kw, {})

    def test_kw_only(self):
        p = C(b='a', n=2)
        self.assertEqual(p.args, tuple(), p.args)
        self.assertEqual(p.kw, dict(b='a', n=2))

    def test_args_and_kw(self):
        p = C(1, 2, b='a', n=2)
        self.assertEqual(p.args, (1, 2))
        self.assertEqual(p.kw, dict(b='a', n=2))

    def test_callable(self):
        p = C(1, 2, b='a', n=2)
        res = []
        def tester(arg1, arg2, b=None, n=None):
            res.extend([arg1, arg2, b, n])
        p(tester)
        self.assertEqual(res, [1, 2, 'a', 2])

    @params(
        missing_arg =   C(lambda a: 1, C(),    "(?i)(?=.*'a')(?=.*missing)"),
        extra_arg =     C(lambda: 1, C(1),     "(?i)(?=.*0)(?=.*positional)"),
        missing_kw =    C(lambda *, x: 1, C(), "(?i)(?=.*'x')(?=.*missing)"),
        extra_kw =      C(lambda: 1, C(x=1),   "(?i)(?=.*'x')(?=.*unexpected)"),
        )
    def test_arguments_mismatch(self, f, cs, msg):
        with self.assertRaisesRegex(TypeError, msg):
            cs(f)

    expected_reprs = Params(
        no_arg =        C(C(),                  "{fn}()"),
        one_arg =       C(C(1),                 "{fn}(1)"),
        two_args =      C(C(1, 2),              "{fn}(1, 2)"),
        two_str_args =  C(C('1', '2'),          "{fn}('1', '2')"),
        one_kw =        C(C(a=1),               "{fn}(a=1)"),
        two_kw =        C(C(a=1, b=2),          "{fn}(a=1, b=2)"),
        two_str_kw =    C(C(a='1', b='2'),      "{fn}(a='1', b='2')"),
        one_each =      C(C(1, a='1'),          "{fn}(1, a='1')"),
        two_each =      C(C(1, 2, a='1', b=3),  "{fn}(1, 2, a='1', b=3)"),
        )

    @params(expected_reprs)
    def test_repr(self, callspec, expected_repr):
        self.assertEqual(repr(callspec), expected_repr.format(fn='C'))

    @params(expected_reprs)
    def test_repr_call(self, callspec, expected_repr):
        f = lambda: 1
        self.assertEqual(
            callspec.repr_call(f),
            expected_repr.format(fn='<lambda>'),
            )

    @params(
        params_map(lambda cs, _: only(C(cs, cs)))(expected_reprs),
        dict_reversed = C(C(dict(a=1, b=2)), C(dict(b=2, a=1))),
        kws_reversed =  C(C(a=1, b=2, c=3),  C(c=3, b=2, a=1)),
        )
    def test_eq(self, callspec1, callspec2):
        self.assertEqual(callspec1, callspec2)

    @params(
        one_arg_value_mismatch =    C(C(1),             C(2)),
        two_arg_value_mismatch =    C(C(1, a='2'),      C(2, a='1')),
        two_arg_value_mismatch2 =   C(C(1, a='2'),      C(1, a='1')),
        arg_count_mismatch =        C(C(1, 2),          C(1)),
        arg_type_mismatch =         C(C('1', '2'),      C(1, 2)),
        kw_name_mismatch =          C(C(a=1),           C(b=1)),
        kw_count_mismatch =         C(C(a=1, b=2),      C(a=1)),
        kw_type_mismatch =          C(C(a='1', b='2'),  C(a=1, b=2)),
        non_callspec =              C(C(1),             1),
        )
    def test_neq(self, callspec1, callspec2):
        self.assertNotEqual(callspec1, callspec2)

    def test_args_is_settable(self):
        cs = C('a', 'b')
        cs.args = ('c', 'd')
        self.assertEqual(cs(lambda *args: args), ('c', 'd'))

    def test_kw_is_settable(self):
        cs = C(b=1, c=2)
        cs.kw = dict(c=3, d=3)
        self.assertEqual(cs(lambda *_, **kw: kw), dict(c=3, d=3))

    def test_kw_is_mutable(self):
        cs = C(b=1, c=2)
        cs.kw['b'] = 2
        cs.kw.update(z=7)
        self.assertEqual(cs(lambda *_, **kw: kw), dict(b=2, c=2, z=7))

    @params(for_each_name('fmt', 'fmtall')(TestFmt.substitution_cases))
    def test_fmt(self, fmtname, unmodified, subs, expected):
        cs = C(*TestFmt.make_list(unmodified), **TestFmt.make_dict(unmodified))
        exp = C(*TestFmt.make_list(expected), **TestFmt.make_dict(expected))
        self.assertEqual(getattr(cs, fmtname)(**subs), exp)

    def test_fmt_does_not_recurse(self):
        unmodified = C(*TestFmt.nested)
        self.assertEqual(unmodified.fmt(**TestFmt.nested_subs), unmodified)

    def test_fmtall_recurses(self):
        self.assertEqual(
            C(*TestFmt.nested).fmtall(**TestFmt.nested_subs),
            C(*TestFmt.nested_subbed),
            )


class TestParams(ParamsMixin, unittest.TestCase):

    @params(
        ints=C(
            C(a=1, b=2, c=3),
            expected=dict(a=C(1), b=C(2), c=C(3)),
            ),
        cs=C(
            C(a=C(1), b=C(2), c=C(3)),
            expected=dict(a=C(1), b=C(2), c=C(3)),
            ),
        dict_and_kw=C(
            C(z=dict(a=C(1), b=2), c=C(3)),
            expected=dict(z=C(dict(a=C(1), b=2)), c=C(3)),
            ),
        params_and_kw=C(
            C(Params(z=dict(a=C(1), b=2)), c=C(3)),
            expected=dict(z=C(dict(a=C(1), b=2)), c=C(3)),
            ),
        params_as_c_arg=C(
            C(z=C(Params(y=dict(a=C(1), b=2)))),
            expected=dict(z=C(Params(y=C(dict(a=C(1), b=2))))),
            ),
        params_as_kw=C(
            C(z=Params(y=dict(a=C(1), b=2))),
            expected=dict(z=C(Params(y=C(dict(a=C(1), b=2))))),
            ),
        )
    def test_valid_data(self, callspec, expected):
        result = callspec(Params)
        self.assertEqual(dict(result), expected)
        self.assertIsInstance(result, Params)

    def test_repr(self):
        self.assertEqual(
            repr(Params(a=1, b=C(3), z=dict(a=1, b=2))),
            "Params(a=C(1), b=C(3), z=C({'a': 1, 'b': 2}))",
            )

    @params(int=1, dict=dict(a=1), params=Params(a=1), c=C(1), d=C(Params(z=1)))
    def test_setitem_results_in_c(self, value):
        p = Params()
        p['foo'] = value
        self.assertEqual(p['foo'], value if isinstance(value, C) else C(value))

    @params_map
    def for_init_and_update(cs, msg):
        yield 'init', C(Params, cs, msg)
        yield 'update', C(Params().update, cs, msg)

    @params(
        for_init_and_update(
            params_kw = C(C(Params(a=1), a=7), msg=r'a=7'),
            two_params = C(C(Params(a=1), Params(a=7)), msg=r'a=C\(7\)'),
            ),
        setitem_duplicate = C(Params(a=1).__setitem__, C('a', 7), msg='a=7'),
        setitem_non_identifier = C(Params().__setitem__, C('0', 7), msg='0'),
        )
    def test_duplicate_keys_disallowed(self, meth, callspec, msg):
        with self.assertRaisesRegex(ValueError, msg):
            callspec(meth)

    @params(for_init_and_update(TYPED_VALUES))
    def test_invalid_data_types(self, meth, typ, val):
        msg = f'(?=.*1)(?=.*Params)(?=.*{typ})'
        with self.assertRaisesRegex(TypeError, msg):
            meth(val)


class TestParameterizingTests(AssertMixin, ParamsMixin, unittest.TestCase):

    def _test_success(self, testcase, testname):
        res = unittest.TestResult()
        testcase(methodName=testname).run(res)
        self.assertEqual(res.testsRun, 1)
        self.assertEqual(res.failures, [])
        self.assertEqual(res.errors, [])
        self.assertTrue(res.wasSuccessful)

    def _test_error(self, testcase, testname, expected_error_regex):
        res = unittest.TestResult()
        testcase(methodName=testname).run(res)
        self.assertEqual(res.testsRun, 1)
        self.assertEqual(res.failures, [])
        self.assertEqual(len(res.errors), 1, "wrong number of errors raised")
        self.assertRegex(res.errors[0][1], expected_error_regex)
        self.assertFalse(res.wasSuccessful())

    def test_normal_tests_run(self):
        check = []
        class Test(ParamsMixin, unittest.TestCase):
            def test_normal_tests_run(self):
                check.append(1)
        self._test_success(Test, 'test_normal_tests_run')
        self.assertEqual(check, [1])

    class ParameterizeFixture(ParamsMixin, unittest.TestCase):

        @params(a=C(1), b=C(2), c=C(3))
        def test_kw(self, value):
            self.check.append(value)

        @params(Params(a=C(1), b=C(2), c=C(3)))
        def test_arg(self, value):
            self.check.append(value)

        @params
        def test_params(self, value):
            self.check.append(value)
        params_test_params = Params(a=C(1), b=C(2), c=C(3))

        @params(Params(a=C(1)), b=C(2))
        def test_multiple_sources(self, value):
            self.check.append(value)
        params_test_multiple_sources = Params(c=C(3))

        @params(Params(a=C(1), b=C(2)), Params(c=C(3), d=C(4)), e=C(5), f=C(6))
        def test_multiple_multiple(self, value):
            self.check.append(value)
        params_test_multiple_multiple = Params(g=C(7), h=C(8))
        params_test_multiple_multiple__more = Params(i=C(9), h=C(10))

        expected_names = [
            *[
                f'test_{n}__{k}'
                for k in 'abc'
                for n in ('kw', 'arg', 'params', 'multiple_sources')
                ],
            *[f'test_multiple_multiple__{k}' for k in 'abcdefgh'],
            *[f'test_multiple_multiple__more__{k}' for k in 'ih'],
            ]

    @params(
        as_value('kw', 'arg', 'params', 'multiple_sources'),
        multiple_multiple=C(
            'multiple_multiple',
            expected={c: n for n, c in enumerate('abcdefgh', 1)},
            ),
        multiple_multiple_more=C(
            'multiple_multiple__more',
            expected={c: n for n, c in enumerate('ih', 9)},
            )
        )
    def test_parameterization(self, name, expected=dict(a=1, b=2, c=3)):
        self.ParameterizeFixture.check = []
        values = []
        for k, v in expected.items():
            self._test_success(self.ParameterizeFixture, f'test_{name}__{k}')
            values.append(v)
            self.assertEqual(self.ParameterizeFixture.check, values)
        with self.assertRaisesRegex(
                ValueError,
                r'(?i)(?=.*no.*test.*method)(?=.*test_{name}__bad)',
            ):
            self.ParameterizeFixture('test_{name}__bad')

    class RawValueFixture(ParamsMixin, unittest.TestCase):

        expected = dict(a=1, b=(2, 3), c=dict(z=4))

        @params(**expected)
        def test_kw(self, value):
            self.check.append(value)

        @params(Params(**expected))
        def test_arg(self, value):
            self.check.append(value)

        @params
        def test_params(self, value):
            self.check.append(value)
        params_test_params = Params(**expected)

        @params_map
        def identity(v):
            yield '', v
        @params
        def test_params_map(self, value):
            self.check.append(value)
        params_test_params_map = identity(**expected)

        expected_names = [
            f'test_{n}__{k}'
            for k in expected
            for n in ('kw', 'arg', 'params', 'params_map')
            ]

    @params(as_value('kw', 'arg', 'params', 'params_map'))
    def test_raw_values_are_handled(self, name):
        self.RawValueFixture.check = []
        values = []
        for k, v in self.RawValueFixture.expected.items():
            self._test_success(self.RawValueFixture, f'test_{name}__{k}')
            values.append(v)
            self.assertEqual(self.RawValueFixture.check, values)

    class ParamsAttributeFixture(ParamsMixin, unittest.TestCase):

        @params
        def test(self, n):
            self.check.append(('test', n))
        params_test = Params(a='test')
        params_test__more = Params(a='test')
        params_test__more__still = Params(a='test')

        @params
        def test_(self, n):
            self.check.append(('test', n))
        params_test_ = Params(a='test')
        params_test___more = Params(a='test')
        params_test___more__still = Params(a='test')

        @params
        def test__(self, n):
            self.check.append(('test__', n))
        params_test__ = Params(a='test__')
        params_test____more = Params(a='test__')
        params_test____more__still = Params(a='test__')

        @params
        def test___(self, n):
            self.check.append(('test___', n))
        params_test___ = Params(a='test___')
        params_test_____more = Params(a='test___')
        params_test_____more__still = Params(a='test___')

        @params
        def test____(self, n):
            self.check.append(('test____', n))
        params_test____ = Params(a='test____')
        params_test______more = Params(a='test____')
        params_test______more__still = Params(a='test____')

        @params
        def test__foo(self, n):
            self.check.append(('test__foo', n))
        params_test__foo = Params(a='test__foo')
        params_test__foo__more = Params(a='test__foo')
        params_test__foo__more__still = Params(a='test__foo')

        @params
        def test___foo(self, n):
            self.check.append(('test___foo', n))
        params_test___foo = Params(a='test___foo')
        params_test___foo__more = Params(a='test___foo')
        params_test___foo__more__still = Params(a='test___foo')

        @params
        def test____foo(self, n):
            self.check.append(('test____foo', n))
        params_test____foo = Params(a='test____foo')
        params_test____foo__more = Params(a='test____foo')
        params_test____foo__more__still = Params(a='test____foo')

        @params
        def test_foo__bar(self, n):
            self.check.append(('test_foo__bar', n))
        params_test_foo__bar = Params(a='test_foo__bar')
        params_test_foo__bar__more = Params(a='test_foo__bar')
        params_test_foo__bar__more__still = Params(a='test_foo__bar')

        @params
        def test_foo__bar__baz(self, n):
            self.check.append(('test_foo__bar__baz', n))
        params_test_foo__bar__baz = Params(a='test_foo__bar__baz')
        params_test_foo__bar__baz__more = Params(a='test_foo__bar__baz')
        params_test_foo__bar__baz__more__still = Params(a='test_foo__bar__baz')

        expected_names = [
            'test__a',
            'test__more__a',
            'test__more__still__a',
            'test___a',
            'test___more__a',
            'test___more__still__a',
            'test____a',
            'test____more__a',
            'test____more__still__a',
            'test_____a',
            'test_____more__a',
            'test_____more__still__a',
            'test______a',
            'test______more__a',
            'test______more__still__a',
            'test__foo__a',
            'test__foo__more__a',
            'test__foo__more__still__a',
            'test___foo__a',
            'test___foo__more__a',
            'test___foo__more__still__a',
            'test____foo__a',
            'test____foo__more__a',
            'test____foo__more__still__a',
            'test_foo__bar__a',
            'test_foo__bar__more__a',
            'test_foo__bar__more__still__a',
            'test_foo__bar__baz__a',
            'test_foo__bar__baz__more__a',
            'test_foo__bar__baz__more__still__a',
            ]

    @params(as_value(*ParamsAttributeFixture.expected_names))
    def test_params_attach_to_correct_tests(self, name):
        self.ParamsAttributeFixture.check = []
        self._test_success(self.ParamsAttributeFixture, name)
        self.assertEqual(*self.ParamsAttributeFixture.check[0])

    @params(
        for_each_function(
            ParameterizeFixture,
            RawValueFixture,
            ParamsAttributeFixture
            )(C()),
        )
    def test_names_are_as_expected(self, fixture):
       test_names = [x for x in dir(fixture) if x.startswith('test_')]
       self.assertEqual(sorted(test_names), sorted(fixture.expected_names))

    def test_empty_parameters_is_an_error_by_default(self):
        msg = r"(?i)(?=.*'test_foo')(?=.*no.*param)"
        with self.assertRaisesRegex(ValueError, msg):
            class Test(ParamsMixin, unittest.TestCase):
                @params()
                def test_foo(self):
                    pass
                params_test_foo = Params()

    def test_empty_decorator_is_ok_when_check_disabled(self):
        class Test(ParamsMixin, unittest.TestCase):
            paramsRequired = False
            @params()
            def test_foo(self):
                pass

    def test_empty_parameters_is_ok_when_check_disabled(self):
        class Test(ParamsMixin, unittest.TestCase):
            paramsRequired = False
            @params
            def test_foo(self):
                pass
            params_test_foo = Params()

    def test_no_parameter_sets_is_an_error(self):
        msg = r"(?i)(?=.*no.*param)(?=.*'test_foo')"
        with self.assertRaisesRegex(ValueError, msg):
            class Test(ParamsMixin, unittest.TestCase):
                paramsRequired = False
                @params
                def test_foo(self):
                    pass

    def test_no_parameter_sets_is_an_error_even_when_check_disabled(self):
        msg = r"(?i)(?=.*no.*param)(?=.*'test_foo')"
        with self.assertRaisesRegex(ValueError, msg):
            class Test(ParamsMixin, unittest.TestCase):
                paramsRequired = False
                @params
                def test_foo(self):
                    pass

    def test_params_and_no_decorator_is_an_error(self):
        with self.assertRaisesRegex(
                ValueError, r'(?i)(?=.*params_test_foo)(?=.*no.*test)',
            ):
            class Test(ParamsMixin, unittest.TestCase):
                def test_foo(self):
                    pass
                params_test_foo = Params()

    def test_params_with_no_exactly_matching_test_is_an_error(self):
        with self.assertRaisesRegex(
                ValueError, r'(?i)(?=.*params_test_foo_bar)(?=.*no.*test)',
            ):
            class Test(ParamsMixin, unittest.TestCase):
                @params
                def test_foo(self):
                    pass
                params_test_foo_bar = Params()

    def test_params_args_keys_must_differ(self):
        with self.assertRaisesRegex(ValueError, r'ggg=.*6'):
            class Test(ParamsMixin, unittest.TestCase):
                @params(Params(xzy=1, b=2, ggg=3), Params(ggg=6, xzy=7))
                def test_foo(self):
                    pass

    def test_params_args_keys_must_differ_from_kws(self):
        with self.assertRaisesRegex(ValueError, r'ggg=.*6'):
            class Test(ParamsMixin, unittest.TestCase):
                @params(Params(xzy=1, b=2, ggg=3), ggg=6, xzy=7)
                def test_foo(self):
                    pass

    def test_params_args_keys_must_differ_from_params_attr_keys(self):
        with self.assertRaisesRegexEx(
                ValueError, r'params_test_foo',
                ValueError, r'ggg=.*6',
            ):
            class Test(ParamsMixin, unittest.TestCase):
                @params(Params(xzy=1, b=2, ggg=3))
                def test_foo(self):
                    pass
                params_test_foo = Params(ggg=6, xzy=7)

    def test_kws_must_differ_from_params_attr_keys(self):
        with self.assertRaisesRegexEx(
                ValueError, r'params_test_foo',
                ValueError, r'ggg=.*6',
            ):
            class Test(ParamsMixin, unittest.TestCase):
                @params(xzy=1, b=2, ggg=3)
                def test_foo(self):
                    pass
                params_test_foo = Params(ggg=6, xzy=7)

    def test_params_attr_keys_must_differ(self):
        with self.assertRaisesRegexEx(
                ValueError, r"'params_test_bar__foo'",
                ValueError, r'ggg=.*6',
            ):
            class Test(ParamsMixin, unittest.TestCase):
                @params
                def test_bar(self):
                    pass
                params_test_bar = Params(foo__ggg=6, xzy=7)
                params_test_bar__foo = Params(ggg=6, xzy=7)

    @params(TYPED_VALUES)
    def test_non_params_arg_to_decorator_is_invalid(self, typ, val):
        msg = fr'(?=.*1)(?=.*Params)(?=.*{typ})'
        with self.assertRaisesRegex(TypeError, msg):
            class Test(ParamsMixin, unittest.TestCase):
                # we have to have a dummy argument here because unlike any
                # normal call we'd otherwise only be passing one argument, and
                # when we pass params exactly one callable it will think it is
                # supposed to wrap it.  Which is what it should do, but not
                # what we are testing here.
                @params(Params(dummy=1), val)
                def test_bad_arg(self):
                    pass

    @params(TYPED_VALUES)
    def test_non_params_value_for_params_attr_is_invalid(self, typ, val):
        msg = fr'(?i)(?=.*params_test_bad_value)(?=.*not.*{typ})'
        with self.assertRaisesRegex(ValueError, msg):
            class Test(ParamsMixin, unittest.TestCase):
                params_test_bad_value = val

    def test_debug(self):
        with captured_stdout() as stdout:
            class Test(ParamsMixin, unittest.TestCase):
                paramsDebug = True
                paramsRequired = False
                def test_dummy(): pass
                @params
                def test_foo(self, a): pass
                params_test_foo = Params(x=7, y=3)
                @params(a=1, b=2)
                def test_bar(self, z): pass
                params_test_bar = Params(c=4, d=6)
        self.assertEqual(
            stdout.getvalue(),
            # Making this an exact match means any change to the debug
            # output requires a change here.  On the other hand, that also
            # means that temporary changes to the debug output during bug
            # fixing in params_map itself will be caught by this test so
            # they don't sneak in to production code unintentionally.
            dedent("""\
                @params method 'test_foo'
                params_ attribute 'params_test_foo'
                @params method 'test_bar'
                params_ attribute 'params_test_bar'
                'test_foo' has no decorator params and 1 params_ attribute
                generated test_foo__x(7)
                generated test_foo__y(3)
                'test_bar' has decorator params and 1 params_ attribute
                generated test_bar__a(1)
                generated test_bar__b(2)
                generated test_bar__c(4)
                generated test_bar__d(6)
                """)
            )


class Test_params_map(AssertMixin, ParamsMixin, unittest.TestCase):

    @params
    def test(self, callspec, expected):
        i = 0
        @params_map
        def numbered_params(*args, **kw):
            nonlocal i
            # With this 'if' we test params_map handling being handed raw data
            yield f't{i}', args[0] if len(args) == 1 else C(*args, **kw)
            i += 1
        result = callspec(numbered_params)
        self.assertEqual(dict(result), expected)
        self.assertIsInstance(result, Params)

    params_test__value_wrapping = Params(
        string = C(     C('abc'),          dict(t0=C('abc'))                ),
        char = C(       C(C('a')),         dict(t0=C('a'))                  ),
        tuple = C(      C(('a', 2)),       dict(t0=C(('a', 2)))             ),
        list = C(       C(['b', 7]),       dict(t0=C(['b', 7]))             ),
        dict = C(       C(dict(a=1, b=2)), dict(t0=C(dict(a=1, b=2)))       ),
        multiple = C(   C(4, 7, 9),        dict(t0=C(4), t1=C(7), t2=C(9))  ),
        kw_only = C(    C(x=1, y=C(7)),    dict(x__t0=C(1), y__t1=C(7))     ),
        mixed = C(
            C(1, (3, 5), z=[0, 1]),
            dict(t0=C(1), t1=C((3, 5)), z__t2=C([0, 1])),
            ),
        mixed2 = C(
            C(4, z=7, b=9),
            dict(t0=C(4), z__t1=C(7), b__t2=C(9)),
            ),
        )

    params_test__flattening = Params(
        one_pset = C(
            C(Params(a=1, b=2)),
            dict(a__t0=C(1), b__t1=C(2)),
            ),
        pset_and_duplicator = C(
            C(
                Params(x=1, y=2, z=3),
                params_map(lambda v: [('z', v), ('x', v)])(
                    Params(a='a', b='b'),
                    ),
                ),
            dict(
                x__t0=C(1),
                y__t1=C(2),
                z__t2=C(3),
                a__z__t3=C('a'),
                a__x__t4=C('a'),
                b__z__t5=C('b'),
                b__x__t6=C('b'),
                ),
            ),
        two_psets_and_kewords = C(
            C(Params(a=0, b=1), Params(c=2, d=3), e=4, f=5),
            {f"{chr(ord('a')+i)}__t{i}": C(i) for i in range(6)},
            ),
        )

    params_test__only = Params(
        no_extra_name = C(
            C(params_map(lambda v: only(C(v+1)))(a=1, b=2)),
            dict(a__t0=C(2), b__t1=C(3)),
            ),
        adds_name = C(
            C(params_map(lambda v: only('z', C(v+1)))(a=1, b=2)),
            dict(a__z__t0=C(2), b__z__t1=C(3)),
            ),
        generates_name = C(
            C(params_map(lambda v: only(chr(ord('b') + v), C(v+1)))(1, 2)),
            dict(c__t0=C(2), d__t1=C(3)),
            ),
        )

    def test_output_can_be_zero_or_many(self):
        @params_map(with_name=True)
        def zero_or_many(name, *args, **kw):
            if name == 'skip':
                return
            if name == 'dup':
                yield '1', C(*args, **kw)
                yield '2', C(*args, **kw)
                yield '3', C(*args, **kw)
            else:
                yield '', C(*args, **kw)
        self.assertEqual(
            zero_or_many(dup=C(1), skip=C(2), other=C(3)),
            dict(dup__1=C(1), dup__2=C(1), dup__3=C(1), other=C(3)),
            )

    def test_composing_maps(self):
        @params_map
        def add_args(foo, bar):
            yield foo, C(foo + bar)
        @params_map
        def no_zed(v):
            yield '', C(v.removesuffix('zed'))
        round1 = add_args(a=C('abc', 'de'), b=C('x', 'zed'))
        self.assertEqual(round1, dict(a__abc=C('abcde'), b__x=C('xzed')))
        self.assertEqual(no_zed(round1), dict(a__abc=C('abcde'), b__x=C('x')))

    @params(
        repeated_name = C(      C('a', ('a', 'a')),      err="'a'"          ),
        colliding_names = C(    C('a__a', a=C('a')),     err=r"a=C\('a'\)"  ),
        null_name = C(          C(''),                   err="''"           ),
        good_before_dup = C(    C('a', 'b', 'c', 'c'),   err="'c'"          ),
        empty_in_middle = C(    C('a', 'b', '', 'c'),    err="''"           ),
        )
    def test_names_must_be_unique(self, callspec, err):
        @params_map
        def yield_name(*args, **kw):
            yield args[0], "doesn't matter"
        with self.assertRaisesRegex(ValueError, err):
            callspec(yield_name)

    @params
    def test_with_name(self, callspec, expected):
        @params_map(with_name=True)
        def use_first_arg_if_no_name(n, *args, **kw):
            label = '' if n else args[0]
            yield label, C(*args, **kw)
        self.assertEqual(callspec(use_first_arg_if_no_name), expected)

    params_test_with_name = Params(
        named = C(  C(a=1, b=2), expected=Params(a=C(1), b=C(2))),
        noname = C( C('a', 'b'), expected=Params(a=C('a'), b=C('b'))),
        mixed = C(  C('a', b=2), expected=Params(a=C('a'), b=C(2))),
        )

    @params(
        params_test_with_name,
        cskip = C(  C('z', c=1),        expected=Params(z=C('z'))),
        xyskip = C( C(b=1, x__y='a'),   expected=Params(b=C(1))),
        noxskip = C(C('m', x__b='a'),   expected=Params(m=C('m'), x__b='a')),
        )
    def test_with_namelist(self, callspec, expected):
        @params_map(with_namelist=True)
        def use_first_arg_or_skip_on_c_d_xy(nl, *args, **kw):
            self.assertIsInstance(nl, NameList)
            if nl.has_any('c', 'd') or nl.has_all('x', 'y'):
                return
            label = '' if nl else args[0]
            yield label, C(*args, **kw)
        self.assertEqual(callspec(use_first_arg_or_skip_on_c_d_xy), expected)

    def test_with_name_and_with_namelist_cannot_both_be_true(self):
        with self.assertRaisesRegex(ValueError, "(?i)(?=.*both)(?=.*True)"):
            @params_map(with_name=True, with_namelist=True)
            def foo():
                pass

    @params(
        on_call =           C(lambda *_: 1/0),
        on_bad_value =      C(lambda *a: [('x', 0/a[-1])], on_0=True),
        in_generator = C(
            lambda *a: [(str(x), 0/x) for x in (a[-1]+1, a[-1])],
            on_0=True,
            ),
        too_many_values =   C(lambda _: [('', 2, 3)]),
        too_few_values =    C(lambda _: [('',)]),
        non_iterable =      C(lambda _: [1]),
        non_string_name =   C(lambda _: [(1, 1)]),
        non_identifier =    C(lambda _: [('.', 1)]),
        )
    def test_errors_in_wrapped_function(self, func, on_0=False):
        test_params_map = params_map(func)
        expected = 'zero=C(0)' if on_0 else 'one=C(1)'
        with self.assertRaisesRegex(ValueError, re.escape(expected)):
            test_params_map(one=1, zero=0)

    @params(
        no_argument =   C(C(),                  TypeError, 'missing'),
        extra_args =    C(C(lambda: 1, 'bar'),  TypeError, '2 were given'),
        non_func_arg =  C(C(1),                 TypeError, 'int'),
        bad_keyword =   C(C(bad_key=True),      TypeError, 'bad_key'),
        )
    def test_bad_arguments(self, callspec, ex, msg):
        with self.assertRaisesRegex(ex, msg):
            # For the keyword case we get back a wreapper and need to call
            # it to see the error. Otherwise the error happens before the call.
            callspec(params_map)('foo')

    def test_debug(self):
        @params_map(debug=True)
        def test_map(anarg, k=None):
            yield 'arg', C(anarg)
            yield 'kw', C(k=k)
        with captured_stdout() as stdout:
            test_map(x=C('a', k='b'), y=C('d', k='e'))
            self.assertEqual(
                stdout.getvalue(),
                # Making this an exact match means any change to the debug
                # output requires a change here.  On the other hand, that also
                # means that temporary changes to the debug output during bug
                # fixing in params_map itself will be caught by this test so
                # they don't sneak in to production code unintentionally.
                dedent("""\
                    flattening using test_map
                    an='x' av=C('a', k='b')
                    n='arg' v=C('a') name='x__arg'
                    n='kw' v=C(k='b') name='x__kw'
                    an='y' av=C('d', k='e')
                    n='arg' v=C('d') name='y__arg'
                    n='kw' v=C(k='e') name='y__kw'
                    """)
                )

    # The exact causes here are not part of the fixed expecations (just
    # that there *is* a cause), but if they change it is worth noticing.

    utility_maps = params_map(lambda f, **k: only(f.__name__, C(f, **k)))(
        C(
            with_names,
            as_map=with_names,
            ),
        C(
            as_value,
            as_map=as_value,
            unnamed_cause_msg=r'(?i)(?=.*invalid)(?=.*1)'
            ),
        C(
            add_label,
            as_map=add_label('xxx'),
            unnamed_cause_msg=r'x=C\(1\)',
            badarg_cause_ex=ValueError,
            badarg_cause_msg=r'(?i)(?=.*invalid)(?=.*1)'
            ),
        C(
            include_if,
            as_map=include_if(lambda *_: True),
            badarg_cause_ex=TypeError,
            badarg_cause_msg=r"'int' object is not callable",
            ),
        C(
            include_unless,
            as_map=include_unless(lambda *_: False),
            badarg_cause_ex=TypeError,
            badarg_cause_msg=r"'int' object is not callable",
            ),
        C(
            for_each_name,
            as_map=for_each_name('aname'),
            unnamed_cause_msg=r"aname=(?=.*exists)(?=.*C\('aname', 1\))",
            badarg_cause_ex=ValueError,
            badarg_cause_msg=r"(?i)(?=.*invalid label)(?=.*1)",
            ),
        C(
            for_each_function,
            as_map=for_each_function(int),
            unnamed_cause_msg=r'int=C\(.*int.*1\)',
            badarg_cause_ex=AttributeError,
            badarg_cause_msg=r"'int'.*__name__",
            ),
        )

    @params
    def test_utility_maps(self, utility, callspec, expected):
        self.assertEqual(callspec(utility), expected)

    params_test_utility_maps = Params(

        params_map(
            lambda *a, as_map, **k: only('no_args', C(as_map, C(), dict()))
            )(utility_maps),

        with_names = C(
            with_names,
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a=C('a', 1), z=C('z', 'a'), foo=C('foo', ['bar'])),
            ),

        as_value = C(
            as_value,
            C('a', 'foo', 'bar'),
            dict(a=C('a'), foo=C('foo'), bar=C('bar')),
            ),

        add_label = C(
            add_label('xxx'),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a__xxx=C(1), z__xxx=C('a'), foo__xxx=C(['bar'])),
            ),

        include_if__include_all = C(
            include_if(lambda *_: True),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a=C(1), z=C('a'), foo=C(['bar'])),
            ),

        include_if__include_none = C(
            include_if(lambda *_: False),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(),
            ),

        include_unless__omit_all = C(
            include_unless(lambda *_: True),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(),
            ),

        include_unless__omit_none = C(
            include_unless(lambda *_: False),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a=C(1), z=C('a'), foo=C(['bar'])),
            ),

        include_if__include_one_letters = C(
            include_if(lambda n, v: n.has_any('a', 'z')),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a=C(1), z=C('a')),
            ),

        include_unless__omit_one_letters = C(
            include_unless(lambda n, v: n.has_any('a', 'z')),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(foo=C(['bar'])),
            ),

        include_if__include_int_values = C(
            include_if(lambda n, v: type(v) == int),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a=C(1)),
            ),

        include_unless__omit_int_values = C(
            include_unless(lambda n, v: type(v) == int),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(z=C('a'), foo=C(['bar'])),
            ),

        include_if__include_int_values_with_label = C(
            include_if(lambda n, v: type(v) == int, label='int'),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(a__int=C(1)),
            ),

        include_unless__omit_int_values_with_label = C(
            include_unless(lambda n, v: type(v) == int, label='non_int'),
            C(Params(a=1), Params(z='a', foo=['bar'])),
            dict(z__non_int=C('a'), foo__non_int=C(['bar'])),
            ),

        for_each_name = C(
            for_each_name('some', 'names'),
            C(42, a=1, b=C(2, z=7)),
            dict(
                some=C('some', 42),
                names=C('names', 42),
                a__some=C('some', 1),
                a__names=C('names', 1),
                b__some=C('some', 2, z=7),
                b__names=C('names', 2, z=7),
                ),
            ),

        for_each_function = C(
            for_each_function(as_value, add_label),
            C(42, a=1, b=C(2, z=7)),
            dict(
                as_value=C(as_value, 42),
                add_label=C(add_label, 42),
                a__as_value=C(as_value, 1),
                a__add_label=C(add_label, 1),
                b__as_value=C(as_value, 2, z=7),
                b__add_label=C(add_label, 2, z=7),
                ),
            ),

        )

    @params
    def test_utility_map_failures(
            self,
            utility,
            callspec,
            ex,
            msg,
            cause_ex=None,
            cause_msg=None,
        ):
        with self.assertRaisesRegexEx(ex, msg, cause_ex, cause_msg):
            # Some errors only show up when a generated utility is called.
            callspec(utility)(t1=1, t2=2)

    params_test_utility_map_failures = Params(

        params_map(
            lambda f, as_map=None, unnamed_cause_msg=r'missing.*label', **k:
                only('unnamed_input',
                    C(
                        as_map,
                        C(1, 1),
                        ValueError, fr'(?i)(?=.*{as_map.__name__})(?=.*1)',
                        ValueError, unnamed_cause_msg,
                        ),
                    )
            )(utility_maps),

        params_map(
            lambda f, *, badarg_cs=C(1), badarg_cause_ex, badarg_cause_msg, **k:
                only(
                    'bad_map_maker_arg',
                    C(
                        f,
                        badarg_cs,
                        ValueError, fr'(?i)(?=.*{f.__name__})(?=.*t1=C\(1\))',
                        badarg_cause_ex, badarg_cause_msg,
                        )
                    )
            )(
            include_if(lambda n, *a, **k: 'badarg_cause_ex' in k)(utility_maps),
            ),

        )


class TestNameList(ParamsMixin, unittest.TestCase):

    names_to_list_map = dict(
        XonenameX =                     ['XonenameX'],
        Xtwo__namesX =                  ['Xtwo', 'namesX'],
        Xmany__many__names__hereX =     ['Xmany', 'many', 'names', 'hereX'],
        Xnames_with__underscores_tooX = ['Xnames_with', 'underscores_tooX'],
        Xtoo_many___underscores____are_confusingX = [
            'Xtoo_many', '_underscores', '', 'are_confusingX',
            ]
        )

    name_nl_and_list = params_map(with_name=True)(
        lambda n, v: only(C(n, NameList(n), v))
        )(**names_to_list_map)

    @params(name_nl_and_list)
    def test_str_equals_name(self, name, nl, aslist):
        self.assertEqual(name, str(nl))

    @params(name_nl_and_list)
    def test_str_supports_startswith(self, name, nl, aslist):
        self.assertTrue(str(nl).startswith(name[:2]))
        self.assertFalse(str(nl).startswith('notthestart'))

    @params(name_nl_and_list)
    def test_str_supports_endswith(self, name, nl, aslist):
        self.assertTrue(str(nl).endswith(name[-2:]))
        self.assertFalse(str(nl).endswith('nottheend'))

    @params(name_nl_and_list)
    def test_str_supports_in(self, name, nl, aslist):
        self.assertTrue(name[4:6] in str(nl))
        self.assertFalse('notthemiddle' in str(nl))

    def test_empty_string_produces_empty_list(self):
        self.assertEqual(list(NameList('')), [])

    nl_and_list = params_map(with_name=True)(
        lambda n, v: only(C(NameList(n), v))
        )(**names_to_list_map)

    @params(nl_and_list)
    def test_list(self, nl, aslist):
        self.assertIsInstance(nl, list)
        self.assertListEqual(nl, aslist)

    @params(nl_and_list)
    def test_indexing(self, nl, aslist):
        for i in range(len(aslist)):
            self.assertEqual(nl[i], aslist[i])

    @params(nl_and_list)
    def test_contains(self, nl, aslist):
        for name in aslist:
            self.assertTrue(name in nl)

    # Running all of these for all the examples appears to be a bit of
    # overkill, but not only does it exercise the machinery and provide a
    # non-trivial example, it found a bug that failed for only one of the
    # example names.

    @params_map
    def has_all_tests(nl, l):
        yield 'one_name', C(            nl, C(l[0]),                    True)
        yield 'all_names', C(           nl, C(*l),                      True)
        yield 'name_notname', C(        nl, C(l[0], 'notname'),         False)
        yield 'no_name', C(             nl, C(''),                      False)

    @params(has_all_tests(nl_and_list))
    def test_has_all(self, nl, callspec, expected_value):
        self.assertEqual(callspec(nl.has_all), expected_value)

    @params_map
    def has_any_tests(nl, l):
        yield 'one_name', C(            nl, C(l[0]),                    True)
        yield 'one_name_tuple', C(      nl, C((l[0],)),                 True)
        yield 'one_name_list', C(       nl, C([l[0]]),                  True)
        yield 'all_names', C(           nl, C(*l),                      True)
        yield 'all_names_tuple', C(     nl, C(tuple(l)),                True)
        yield 'all_names_list', C(      nl, C(l),                       True)
        yield 'all_names_dict', C(      nl, C({n: n for n in l}),       True)
        yield 'name_notname', C(        nl, C(l[0], 'notname'),         True)
        yield 'no_names', C(            nl, C(),                        False)
        yield 'no_names_list', C(       nl, C([]),                      False)
        yield 'one_notname', C(         nl, C('notname'),               False)
        yield 'two_notnames', C(        nl, C('notname', 'alsonot'),    False)
        yield 'null_str_arg', C(        nl, C(''),                      False)
        yield 'null_str_tuple', C(      nl, C(('',)),                   False)
        yield 'null_str_list', C(       nl, C(['',]),                   False)

    @params(has_any_tests(nl_and_list))
    def test_has_any(self, nl, callspec, expected_value):
        self.assertEqual(callspec(nl.has_any), expected_value)

    def test_has_any_false_if_empty_name(self):
        nl = NameList('')
        self.assertFalse(nl.has_any(''))

    def test_has_any_false_partial_names(self):
        nl = NameList('foo__bar__bird')
        self.assertTrue(nl.has_any('foo', 'bar', 'bird'))
        self.assertFalse(nl.has_any('fo', '_bar', 'ird', 'ir', 'or', ''))


if __name__ == '__main__':
    unittest.main()
