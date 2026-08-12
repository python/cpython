from email import errors
from test.test_email import TestEmailBase, for_each_character
from test.test_email.params import C, params_map, params, Params

class TestAssertDefectsMatch(TestEmailBase):

    # The code should behave the same whether the pattern comes direct or
    # out of a callable.
    @params_map
    def direct_and_callable(actual, expected, *args):
        yield 'direct', C(actual, expected, *args)
        expected = [(lambda x: x, x) for x in expected]
        yield 'callable', C(actual, expected, *args)

    @params
    def test_success(self, actual, expected):
        self.assertDefectsMatch(actual, expected)

    np_checker = lambda s: (errors.NonPrintableDefect, f'.*non-printable.*{s}')

    params_test_success = direct_and_callable(

        no_defects = C([], []),

        one_defect_by_class = C(
            [errors.InvalidHeaderDefect('foo')],
            [errors.InvalidHeaderDefect],
            ),

        one_defect_by_regex = C(
            [errors.InvalidHeaderDefect('This is a message')],
            [(errors.InvalidHeaderDefect, '.*is a')],
            ),

        multiple_defects_by_class = C(
            [
                errors.InvalidHeaderDefect('This is a message'),
                errors.InvalidHeaderDefect('This is a different message'),
                errors.InvalidHeaderDefect('This is the same message'),
                errors.InvalidHeaderDefect('This is the same message'),
                ],
            [*[errors.InvalidHeaderDefect] * 4],
            ),

        multiple_defects_by_regex = C(
            [
                errors.InvalidHeaderDefect('This is a message'),
                errors.InvalidHeaderDefect('This is a different message'),
                errors.InvalidHeaderDefect('This is the same message'),
                errors.InvalidHeaderDefect('This is the same message'),
                ],
            [
                (errors.InvalidHeaderDefect, '.*the same'),
                (errors.InvalidHeaderDefect, '.*is a'),
                (errors.InvalidHeaderDefect, '.*different'),
                (errors.InvalidHeaderDefect, '.*the same'),
                ],
            ),

        multiple_different_defects_by_class = C(
            [
                errors.InvalidHeaderDefect('This is a message'),
                errors.ObsoleteHeaderDefect('This is a different message'),
                errors.NonPrintableDefect('abc'),
                ],
            [
                errors.InvalidHeaderDefect,
                errors.ObsoleteHeaderDefect,
                errors.NonPrintableDefect,
                ],
            ),

        multiple_different_defects_by_regex = C(
            [
                errors.InvalidHeaderDefect('This is a message'),
                errors.ObsoleteHeaderDefect('This is a different message'),
                errors.NonPrintableDefect('abc'),
                ],
            [
                (errors.ObsoleteHeaderDefect, '.*different'),
                (errors.NonPrintableDefect, '.*non-printable.*abc'),
                (errors.InvalidHeaderDefect, '.*is a'),
                ],
            ),

        )

    @params
    def test_failure(self, actual, expected, msg):
        with self.assertRaisesRegex(AssertionError, msg):
            self.assertDefectsMatch(actual, expected)

    params_test_failure = direct_and_callable(

        one_extra_defect_expecting_none = C(
            [errors.InvalidHeaderDefect('foo')],
            [],
            r'(?i)0.*matched.*1.*extra',
            ),

        two_extra_defects_expecting_none = C(
            [
                errors.InvalidHeaderDefect('foo'),
                errors.InvalidHeaderDefect('bar'),
                ],
            [],
            r'(?i)0.*matched.*2.*extra',
            ),

        two_extra_defects_expecting_one = C(
            [
                errors.InvalidHeaderDefect('foo'),
                errors.InvalidHeaderDefect('bar'),
                ],
            [(errors.InvalidHeaderDefect, 'bar')],
            r'(?i)1.*matched.*1.*extra',
            ),

        three_extra_defects_expecting_one = C(
            [
                errors.InvalidHeaderDefect('foo'),
                *[errors.InvalidHeaderDefect('bar')]*3,
                ],
            [(errors.InvalidHeaderDefect, 'bar')],
            r'(?is)1.*matched.*3.*extra(?=.*foo)(?=.*bar.*bar)',
            ),

        one_missing_defect_expecting_one = C(
            [],
            [(errors.InvalidHeaderDefect, 'bar')],
            r'(?is)0.*matched.*1.*missing.*bar',
            ),

        two_missing_defects_expecting_two = C(
            [
                errors.InvalidHeaderDefect('bar'),
                errors.InvalidHeaderDefect('bing'),
                ],
            [
                (errors.InvalidHeaderDefect, 'foo'),
                (errors.InvalidHeaderDefect, 'bird'),
                ],
            r'(?is)0.*matched.*2.*did not match'
                r'(?=.*foo)(?=.*bird)(?=.*bar)(?=.*bing)',
            ),

        two_missing_defects_expecting_four = C(
            [
                errors.InvalidHeaderDefect('bar'),
                errors.InvalidHeaderDefect('bing'),
                ],
            [
                (errors.InvalidHeaderDefect, 'bar'),
                (errors.InvalidHeaderDefect, 'foo'),
                (errors.InvalidHeaderDefect, 'bing'),
                (errors.InvalidHeaderDefect, 'bird'),
                ],
            r'(?is)2.*matched.*2.*missing(?=.*foo)(?=.*bird)',
            ),

        two_extra_defects_expecting_two = C(
            [
                errors.InvalidHeaderDefect('foo'),
                errors.InvalidHeaderDefect('bar'),
                errors.InvalidHeaderDefect('bing'),
                errors.InvalidHeaderDefect('bar'),
                ],
            [
                (errors.InvalidHeaderDefect, 'bar'),
                (errors.InvalidHeaderDefect, 'bing'),
                ],
            r'(?is)2.*matched.*2.*extra(?=.*foo)(?=.*bar)',
            ),

        two_extra_defects_one_missing_expecting_three = C(
            [
                errors.InvalidHeaderDefect('foo'),
                errors.InvalidHeaderDefect('bar'),
                errors.InvalidHeaderDefect('bing'),
                errors.InvalidHeaderDefect('bar'),
                ],
            [
                (errors.InvalidHeaderDefect, 'bar'),
                (errors.InvalidHeaderDefect, 'bing'),
                (errors.InvalidHeaderDefect, 'bing'),
                ],
            r'(?is)2.*matched(?=.*2.*extra)(?=.*1.*missing)'
                r'(?=.*foo)(?=.*bar)(?=.*bing)',
            ),

        actual_is_string = C(
            ['foo'],
            [],
            r'(?is)0.*matched.*1.*extra(?=.*str.*foo)',
            ),

        actual_is_tuple = C(
            [(errors.InvalidHeaderDefect, 'foo', 'bar')],
            [],
            r'(?is)0.*matched.*1.*extra(?=.*tuple.*InvalidHeaderDefect.*foo)',
            ),

        )

    @params
    def test_bad_expected_patterns(self, actual, expected, msg):
        with self.assertRaisesRegex((ValueError, TypeError), msg):
            self.assertDefectsMatch(actual, expected)

    params_test_bad_expected_patterns = direct_and_callable(

        not_subscriptable = C(
            [],
            [1],
            r'(?i)(?=.*invalid).*1',
            ),

        string = C(
            [],
            ['foo'],
            r'(?i)(?=.*invalid).*foo',
            ),

        triple = C(
            [],
            [(errors.InvalidHeaderDefect, 'foo', 'bar')],
            r'(?i)too many values',
            ),

        singleton = C(
            [],
            [(errors.InvalidHeaderDefect,)],
            '(?i)not enough values',
            ),

        # This only happens if a comparison is made.  Which will happen.
        regex_is_not_string = C(
            [errors.InvalidHeaderDefect('foo')],
            [(errors.InvalidHeaderDefect, 200)],
            r'(?i)must be string',
            ),

        backwards_expected_entry = C(
            [],
            [('foo', errors.InvalidHeaderDefect)],
            r'(?i)(?=.*invalid).*foo.*InvalidHeaderDefect',
            ),

        )

    @params(
        multiple_args = C(
            [(lambda x, y, z: (errors.InvalidHeaderDefect, z), 'x', 1, 'foo')],
            ),
        no_args = C([(lambda: errors.InvalidHeaderDefect,)]),
        )
    def test_callable_success(self, expected):
        self.assertDefectsMatch([errors.InvalidHeaderDefect('foo')], expected)

    @params(
        no_args_bad_result = C([(lambda: 'bad value',)], r'(?i)bad value'),
        wrong_number_of_args = C([(lambda: 'x', 1)], r'(?i)arguments'),
        )
    def test_callable_failure(self, expected, msg):
        with self.assertRaisesRegex((ValueError, TypeError), msg):
            self.assertDefectsMatch([], expected)


class TestForEachCharacter(TestEmailBase):

    @params
    def test_for_each_character(self, callspec, chars, expected):
        callspecs = Params(test=callspec)
        expected = Params(**{f'test__{c}': v for c, v in expected.items()})
        self.assertEqual(for_each_character(chars)(callspecs), expected)

    @params_map
    def for_each_value_type(callspec, chars, expected):
        yield '', C(callspec, chars, expected)
        yield 'in_list', C(
            C(foo=['bar', callspec.args[0], 'z']),
            chars=chars,
            expected={
                n: C(foo=['bar', v.args[0], 'z']) for n, v in expected.items()},
            )
        yield 'in_tuple', C(
            C(foo=('bar', callspec.args[0], 'z')),
            chars=chars,
            expected={
                n: C(foo=('bar', v.args[0], 'z')) for n, v in expected.items()},
            )
        yield 'in_dict', C(
            C(foo=dict(a=callspec.args[0], z=1)),
            chars=chars,
            expected={
                n: C(foo=dict(a=v.args[0], z=1)) for n, v in expected.items()},
            )

    params_test_for_each_character = for_each_value_type(

        no_subs = C(
            C('no subs'),
            chars='./',
            expected=dict(full_stop=C('no subs'), solidus=C('no subs')),
            ),

        one_sub = C(
            C('one{char}sub'),
            chars='./',
            expected=dict(full_stop=C('one.sub'), solidus=C('one/sub')),
            ),

        all_three_sub_types = C(
            C('plain {char} escaped {echar} escaped repr {erchar}.'),
            chars='\t',
            expected=dict(HT=C('plain \t escaped \\\t escaped repr \\\\t.')),
            ),

        a_list = C(
            C(['a', '{char}', '{echar}']),
            chars='/',
            expected=dict(solidus=C(['a', '/', '/'])),
            ),

        a_tuple = C(
            C(('{char}{echar}', '{erchar}')),
            chars='.',
            expected=dict(full_stop=C((r'.\.', r'\.'))),
            ),

        a_dict = C(
            C(dict(a='{char}', b='{erchar}')),
            chars='a',
            expected=dict(latin_small_letter_a=C(dict(a='a', b='a'))),
            ),

        )

    def test_for_each_character_complex_input(self):
        callspecs = Params(
            two_positional=C('pos {char} 1', 'pos {echar} 2'),
            all_value_types=C(
                '{char}',
                d=dict(a='a {char}', r='{erchar}'),
                l=['a', '{char}', 'b'],
                t=('{char}', ('{char}', 1)),
                ),
            dummy=C('no subs'),
            )
        chars = 'X\t.'
        expected = Params(
            two_positional__latin_capital_letter_x=C('pos X 1', 'pos X 2'),
            two_positional__full_stop=C('pos . 1', r'pos \. 2'),
            two_positional__HT=C('pos \t 1', 'pos \\\t 2'),
            all_value_types__latin_capital_letter_x=C(
                'X',
                d=dict(a='a X', r='X'),
                l=['a', 'X', 'b'],
                t=('X', ('X', 1)),
                ),
            all_value_types__HT=C(
                '\t',
                d=dict(a='a \t', r='\\\\t'),
                l=['a', '\t', 'b'],
                t=('\t', ('\t', 1)),
                ),
            all_value_types__full_stop=C(
                '.',
                d=dict(a='a .', r='\\.'),
                l=['a', '.', 'b'],
                t=('.', ('.', 1)),
                ),
            dummy__latin_capital_letter_x=C('no subs'),
            dummy__full_stop=C('no subs'),
            dummy__HT=C('no subs'),
            )
        self.assertEqual(for_each_character(chars)(callspecs), expected)
