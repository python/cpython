import re
import string
import unittest
from contextlib import ExitStack
from email import _header_value_parser as parser
from email import errors
from email import policy
from importlib import import_module
from random import choices, randint, sample
from test.support.import_helper import CleanImport
from test.test_email import (
    charname,
    check_all_warnings,
    for_each_character,
    TestEmailBase,
    parameterize,
    )
from test.test_email.params import (
    add_label,
    as_value,
    C,
    include_unless,
    only,
    params,
    Params,
    params_map,
    with_names,
    )

# https://datatracker.ietf.org/doc/html/rfc5322#section-2.2
RFC_PRINTABLES = bytes(range(33, 127)).decode('ascii')

# https://datatracker.ietf.org/doc/html/rfc5322#section-2.2
RFC_WSP = chr(32) + chr(9)

# https://datatracker.ietf.org/doc/html/rfc5322#section-2.2
RFC_NONPRINTABLES = bytes([*range(0, 33), 127]).decode('ascii')

# https://datatracker.ietf.org/doc/html/rfc2978#section-2.3
# Except that like
#   https://datatracker.ietf.org/doc/html/rfc8187#section-3.2.1
# we omit the "'" character as otherwise it is difficult to correctly parse
# extended parameters values absent a complete registry.  In any case charset
# names generally do not include special characters in practice.
RFC_CHARSET_CHARS = ''.join((
    string.ascii_letters,
    string.digits,
    "!#$%&+-^_`{}~",
    ))

# https://datatracker.ietf.org/doc/html/rfc5322#section-3.2.3
RFC_ATEXT = ''.join((
    string.ascii_letters,
    string.digits,
    "!#$%&'*+-/=?^_`{|}~",
    ))

# https://datatracker.ietf.org/doc/html/rfc5322#section-3.2.3
RFC_SPECIALS = r'()<>[]:;@\,."'

# This isn't an RFC concept, but it is as useful in tests as it is in the code.
CFWS_LEADER = RFC_WSP + '('

ALL_ASCII = bytes(range(0, 128)).decode('ascii')


# ---> Defect Expectations

undecodable_bytes_defect = (
    errors.UndecodableBytesDefect,
    'Non-ASCII characters found in header token',
    )

def undecodable_bytes_in_ew_defect(chars):
    return (
        errors.UndecodableBytesDefect,
        f"Encoded word contains bytes not decodable using '{chars}' charset",
        )

def nonprintable_defect(chars):
    return (
        errors.NonPrintableDefect,
        'the following ASCII non-printables found in header:'
            f' {re.escape(repr(list(chars)))}',
        )

whitespace_inside_ew_defect = (
    errors.InvalidHeaderDefect,
    'whitespace inside encoded word',
    )

missing_whitespace_before_ew_defect = (
    errors.InvalidHeaderDefect,
    'missing whitespace before encoded word',
    )

missing_whitespace_after_ew_defect = (
    errors.InvalidHeaderDefect,
    'missing trailing whitespace after encoded-word',
    )

def charset_defect(chars):
    return (
        errors.CharsetError,
        f"Unknown charset '{chars}' in encoded word; decoded as unknown bytes",
        )

invalid_base64_padding_defect = (
    errors.InvalidBase64PaddingDefect,
    '',
    )

invalid_base64_characters_defect = (
    errors.InvalidBase64CharactersDefect,
    '',
    )

invalid_base64_length_defect = (
    errors.InvalidBase64LengthDefect,
    '',
    )

end_inside_quoted_string_defect = (
    errors.InvalidHeaderDefect,
    'end of header inside quoted string',
    )

ew_inside_quoted_string_defect = (
    errors.InvalidHeaderDefect,
    'encoded word inside quoted string',
    )

end_inside_comment_defect = (
    errors.InvalidHeaderDefect,
    'end of header inside comment',
    )

period_in_phrase_obs_defect = (
    errors.ObsoleteHeaderDefect,
    "period in 'phrase'",
    )

comment_without_atom_in_phrase_obs_defect = (
    errors.ObsoleteHeaderDefect,
    'comment found without atom',
    )

non_word_phrase_start_defect = (
    errors.InvalidHeaderDefect,
    "phrase does not start with word",
    )

non_dot_atom_local_part_obs_defect = (
    errors.ObsoleteHeaderDefect,
    r'local-part is not a dot-atom \(contains CFWS\)',
    )

not_even_obs_local_part_defect = (
    errors.InvalidHeaderDefect,
    'local-part is not dot-atom, quoted-string, or obs-local-part',
    )

missing_dot_in_local_part_defect = (
    errors.InvalidHeaderDefect,
    "missing '.' between words",
    )

trailing_dot_in_local_part_defect = (
    errors.InvalidHeaderDefect,
    "invalid trailing '.' in local part",
    )

leading_dot_in_local_part_defect = (
    errors.InvalidHeaderDefect,
    "invalid leading '.' in local part",
    )

repeated_dot_in_local_part_defect = (
    errors.InvalidHeaderDefect,
    "invalid repeated '.'",
    )

misplaced_backslash_defect = (
    errors.InvalidHeaderDefect,
    r"'\\' character outside of quoted-string/ccontent",
    )

# ---> End Defect Expectations


# XXX POSTDEP: Delete this test case, from here...

class TestDeprecations(TestEmailBase):

    def test___getattr___attribute_error(self):
        nonsense = 'this_does_not_exist'
        with self.assertRaisesRegex(AttributeError, nonsense):
            getattr(parser, nonsense)

    def test___getattr___deprecation(self):
        with CleanImport(parser.__name__):
            foo = import_module(parser.__name__)
            foo._deprecated_foo = lambda: 42
            foo._deprecated_bar = 1
            with check_all_warnings((
                    r"(?=.*'bar')(?=.*is deprecated)",
                    DeprecationWarning,
                    )):
                self.assertEqual(foo.bar, 1)
            self.assertEqual(foo.foo(), 42)

    def test___getattr___replacement(self):
        with CleanImport(parser.__name__):
            foo = import_module(parser.__name__)
            a_func = lambda: 42
            foo._deprecated_foo = a_func
            foo._REPLACED_NAMES['foo'] = 'bird'
            foo._deprecated_bar = 2
            foo._REPLACED_NAMES['bar'] = 'brain'
            with check_all_warnings((
                    r"(?=.*'foo')(?=.*is deprecated)(?=.*'bird')",
                    DeprecationWarning,
                    )):
                self.assertEqual(foo.foo, a_func)
            self.assertEqual(foo.foo(), 42)
            with check_all_warnings((
                    r"(?=.*'bar')(?=.*is deprecated)(?=.*'brain')",
                    DeprecationWarning,
                    )):
                self.assertEqual(foo.bar, 2)

    def test__replaced_with(self):
        with CleanImport(parser.__name__):
            p = import_module(parser.__name__)
            @p._replaced_with('foo')
            def _deprecated_bar(a):
                return a
            p._deprecated_bar = _deprecated_bar
            with check_all_warnings((
                    r"(?=.*'bar')(?=.*is deprecated)(?=.*'foo')",
                    DeprecationWarning,
                    )):
                self.assertEqual(p.bar(2), 2)

    @params(as_value(
        # XXX XXX make sure this is completely filled in with all the
        # names we expect to be deprecated.
        ))
    def test_deprecated_names(self, name):
        with check_all_warnings((
                rf'(?=.*{name})(?=.*is.*deprecated)',
                DeprecationWarning,
            )):
            getattr(parser, name)

    @params(with_names(
        # XXX XXX make sure this is completely filled in with all the names
        # we've replaced.
        ))
    def test_replaced_names(self, oldname, newname):
        with check_all_warnings((
                rf'(?=.*{oldname!r}.*is deprecated)(?=.*{newname})',
                DeprecationWarning,
            )):
            getattr(parser, oldname)

    @params(
        old_simple = C('foo x', '', res=('f', 'oo x', 9), warn=True),
        old_with_arg = C('foo x', ' ', res=('fo', 'o x', 9), warn=True),
        old_with_kw = C('foo x', '', b=2, res=('foo', ' x', 9), warn=True),
        new_with_zero = C('foo x', 0, '', res=('f', 1, 9)),
        new_with_nonzero = C('foo x', 3, '', res=(' ', 4, 9)),
        new_with_arg = C('foo x', 1, ' ', res=('oo', 3, 9)),
        new_with_kw = C('foo x', 2, '', b=2, res=('o x', 5, 9)),
        )
    def test__deprecate_old_api(self, value, *args, b=0, warn=False, res):
        @parser._deprecate_old_api
        def t(value, start, a, b=0):
            end = start + 1 + len(a) + b
            return value[start:end], end, 9
        warnings = []
        if warn:
            warnings += [(
                r"(?=.*'t')(?=.*API)(?=.*has changed)",
                DeprecationWarning,
                )]
        with check_all_warnings(*warnings):
            self.assertEqual(t(value, *args, b=b), res)

    @params(
        old_api_no_error = C(C('abc')),
        new_api_no_error = C(C('abc', 0)),
        old_api_error = C(C(''), warning=True),
        new_api_error = C(C('', 0), exception=True),
        new_api_no_error_with_non_zero_start = C(C('abc', 2)),
        new_api_error_with_non_zero_start = C(C('abc', 3), exception=True),
        )
    def test__deprecate_old_api_and_lack_of_raise_on_invalid_input(
            self,
            callspec,
            exception=False,
            warning=False,
        ):
        @parser._deprecate_old_api_and_lack_of_raise_on_invalid_input
        def foo(value, start):
            if not value[start:]:
                return parser.TokenList(['']), start, Exception('bar'), 'bird'
            return parser.TokenList([value]), start + len(value)
        value, *start = callspec.args
        warnings = []
        if start == []:
            warnings += [
                (r"(?=.*'foo')(?=.*API)(?=.*has changed)", DeprecationWarning)
                ]
        if warning:
            warnings += [('bird', DeprecationWarning)]
        if exception:
            exceptioncheck = self.assertRaisesRegex(Exception, 'bar')
        else:
            exceptioncheck = ExitStack()
        with exceptioncheck:
            with check_all_warnings(*warnings):
                tl, rest = callspec(foo)
        if exception:
            return
        start = start[0] if start else 0
        self.assertEqual(tl, parser.TokenList([value]))
        rest = (len(value) - len(rest)) if hasattr(rest, 'encode') else rest
        self.assertEqual(rest, start + len(tl[0]))

    # XXX XXX _deprecate will go away by the end of refactoring.

    def test__deprecate_no_arg(self):
        @parser._deprecate
        def t(a, b):
            return a, b
        with self.assertWarnsRegex(
                DeprecationWarning,
                r"(?=.*'t'.*is deprecated)",
            ):
            self.assertEqual(t(1, 2), (1, 2))

    def test__deprecate_with_arg(self):
        @parser._deprecate('t2')
        def t(a, b):
            return a, b
        with self.assertWarnsRegex(
                DeprecationWarning,
                r"(?=.*'t'.*is deprecated)(?=.*t2)",
            ):
            self.assertEqual(t(1, 2), (1, 2))

# XXX POSTDEP: ...to here


class TestTokenList(TestEmailBase):

    @params(
        none_none = C([], []),
        one_none =  C([errors.InvalidHeaderDefect('a')], []),
        none_one =  C([], [errors.InvalidHeaderDefect('b')]),
        one_one =   C(
            [errors.InvalidHeaderDefect('a')],
            [errors.InvalidHeaderDefect('b')],
            ),
        two_two =   C(
            [errors.InvalidHeaderDefect('a'), errors.NonPrintableDefect('y')],
            [errors.NonPrintableDefect('b'), errors.InvalidHeaderDefect('z')],
            ),
        )
    def test_extend_copies_defects(self, existing, new):
        tl1 = parser.TokenList()
        tl1.defects.extend(existing)
        tl2 = parser.TokenList(['fake', 'values'])
        tl2.defects.extend(new)
        tl1.extend(tl2)
        self.assertEqual(tl1.defects, existing + new)

    def test_extend_with_non_token_list_leaves_defects_unchanged(self):
        tl = parser.TokenList()
        defects = [errors.InvalidHeaderDefect('a')]
        tl.defects.extend(defects)
        tl.extend(['fake', 'values'])
        self.assertEqual(tl.defects, defects)


class TestTokens(TestEmailBase):

    # EWWhiteSpaceTerminal

    def test_EWWhiteSpaceTerminal(self):
        x = parser.EWWhiteSpaceTerminal(' \t', 'fws')
        self.assertEqual(x, ' \t')
        self.assertEqual(str(x), '')
        self.assertEqual(x.value, '')
        self.assertEqual(x.token_type, 'fws')


class TestParserMixin:

    def _assert_results(self, tl, rest, string, value, defects, remainder,
                        comments=None):
        self.assertEqual(str(tl), string)
        self.assertEqual(tl.value, value)
        self.assertDefectsEqual(tl.all_defects, defects)
        self.assertEqual(rest, remainder)
        if comments is not None:
            self.assertEqual(tl.comments, comments)

    def _test_get_x(self, method, source, string, value, defects,
                          remainder, comments=None):
        tl, rest = method(source)
        self._assert_results(tl, rest, string, value, defects, remainder,
                             comments=None)
        return tl

    def _test_parse_x(self, method, input, string, value, defects,
                             comments=None):
        tl = method(input)
        self._assert_results(tl, '', string, value, defects, '', comments)
        return tl

    def _test_parse(
            self,
            method,
            callspec,
            stringified=None,
            value=None,
            defects=None,
            remainder='',
            comments=None,
            *,
            commenttree=None,
            exception=None,
            warnings=None,
            test_start=True,
            no_end=False,
            pprint=False,
            ):
        """Call method with callspec, make asserts, and return results of call.

        Expect method to be a parsing method that takes a string as its first
        argument and returns a Terminal or TokenList as its return value,
        possibly followed by an "unparsed remainder" index, and possibly
        additional return values.

        If test_start is true (the default), modify the callspec to add a
        random prefix to its first (string) argument, and add a new parameter
        after it consisting of the length of the added prefix.  If the callspec
        contains a value for 'end', modify that value by adding the prefix
        length.

        If exception has a value, assert that using callspec to call method
        raises the exception that must be the first element of value tuple with
        a string value that matches the regex that must be the second element
        of the value tuple.

        Otherwise use the (possibly modified) callspec to call the method,
        capturing its return value, which should either be a single Terminal or
        TokenList, or a tuple whose first element is a Terminal or TokenList.

        If no_end is True, assert that the return value was not a tuple or its
        second value was not an integer.

        If warnings has a value, use it as the argument value to a
        check_all_warnings assert around the callspec call.

        If pprint is true, call the pprint method of returned object.

        If the return value is not a singleton and the second element of
        the return value is an integer, use it, modified by the length of
        the prefix if test_start s true, to assert that the unparsed
        remainder matches the value of 'remainder'.

        Assert that str called on the returned object matches the value
        of stringified, or the characters from start to end or the end
        of the string if stringified is None.

        Assert that the value attribute of the returned object matches
        value, or stringified is value is None.

        Assert that the comments attribute of the returned object matches
        comments.

        If commenttree is not None, assert that the comment tree of the
        returned object matches it. XXX commenttree is an internal testing
        hack, a real API is needed some day.

        Assert that the defects attribute of the returned object matches
        defects.

        Return whatever the called method returned.

        """
        s, *args = callspec.args
        base = s[:-len(remainder)] if remainder else s
        if test_start:
            # XXX I'm not at sure the overhead of this randomization is worth
            # it.  We do at least need to test having a prefix though...
            prefix_len = randint(1, 20)
            prefix = ''.join(choices(ALL_ASCII, k=prefix_len))
            kw = dict(callspec.kw)
            callspec = C(prefix + s, prefix_len, *args, **kw)
        # XXX POSTDEP: Change this if to do only what's in the else clause.
        if warnings is ...:
            warningscheck = ExitStack()
        else:
            warnings = [(x[1], x[0]) for x in warnings] if warnings else []
            warningscheck = check_all_warnings(*warnings)
        if exception:
            with warningscheck:
                with self.assertRaisesRegex(exception[0], exception[1]):
                    callspec(method)
            return
        stringified = base if stringified is None else stringified
        value = stringified if value is None else value
        comments = [] if comments is None else comments
        defects = [] if defects is None else defects
        with warningscheck:
            result = callspec(method)
        if isinstance(result, (parser.TokenList, parser.Terminal)):
            other = []
        else:
            result, *other = result
        if pprint:
            print(f'\n{result.ppstr()}')
        # XXX POSTDEP: remove str from this 'if'
        if other and isinstance(other[0], (int, str)):
            if no_end:
                self.fail(
                    "It looks like the function incorrectly returned an"
                    " end of parsing pointer"
                    )
            # a get_x method that returns a remainder or pointer.
            actual_remainder, *other = other
            if isinstance(actual_remainder, int):
                if test_start:
                    actual_remainder -= prefix_len
                actual_remainder = s[actual_remainder:]
            self.assertEqual(actual_remainder, remainder)
        self.assertEqual(str(result), stringified)
        if isinstance(result, parser.TokenList):
            self.assertEqual(result.value, value)
            self.assertDefectsMatch(result.all_defects, defects)
            # XXX XXX at the end of the refactor get rid of this conditional.
            if comments != ...:
                self.assertEqual(result.comments, comments)
            if commenttree is not None:
                self.assertEqual(self.ctree(result), commenttree)
        return (result, *other) if other else result

    def verify_terminal_types(self, tl, *text_types):
        """Raise error if token_type of any Terminal is not in text_types."""
        self.assertIsInstance(tl, (parser.Terminal, parser.TokenList))
        if isinstance(tl, parser.Terminal):
            self.assertIn(tl.token_type, text_types, repr(tl))
        elif isinstance(tl, parser.TokenList):
            for t in tl:
                # Some functions return a TokenList, but there should never be
                # a plain TokenList anywhere deeper.  This will catch failures
                # to use 'extend' when consuming returned a TokenList.
                self.assertIsNotNone(t.token_type, t)
                self.verify_terminal_types(t, *text_types)

    def ctree(self, tl, cnt=0):
        """Return a testing-adequate depiction of the nested comments"""
        if isinstance(tl, parser.Comment):
            return self._ctree(tl)
        comments = []
        for t in tl:
            if isinstance(t, parser.Comment):
                comments.append(self._ctree(t))
            elif isinstance(t, parser.TokenList):
                comments.extend(self.ctree(t))
        return comments

    def _ctree(self, tl):
        comments = []
        empty = True
        text = ''
        for t in tl:
            if isinstance(t, parser.Comment):
                if text:
                    comments.append(text)
                    text = ''
                comments.append(self._ctree(t))
                empty = False
            else:
                text += str(t)
        if text or empty:
            comments.append(text)
        return comments


# XXX XXX temporary step-wise refactoring tool, goes away at end of refactor.
@params_map(with_namelist=True)
def old_api_only(nl, *args, **kw):
    if 'newapi' in nl:
        return
    kw['warnings'] = ...  # Ignore pre-refactoring warnings.
    kw.setdefault('test_start', False)
    yield '' if 'oldapi' in nl else 'oldapionly', C(*args, **kw)

# XXX POSTDEP: Delete this params_map and replace calls to it with params_set.
@params_map(with_namelist=True)
def for_each_api(nl, *args, **kw):
    if nl.has_any('oldapi', 'newapi'):
        # Reused tests; they've been through here before.
        yield '', C(*args, **kw)
        return
    yield 'newapi', C(*args, **kw)
    kw['warnings'] = kw.get('warnings', []) + [
        (DeprecationWarning, r'.*API.*has changed')
        ]
    yield 'oldapi', C(*args, **kw, test_start=False)


class TestParser(TestParserMixin, TestEmailBase):

    rfc_printable_ascii = bytes(range(33, 127)).decode('ascii')
    rfc_dtext_chars = rfc_printable_ascii.translate(str.maketrans('','',r'\[]'))

    # _wsp_splitter

    @params
    def test__wsp_splitter(self, s, res):
        self.assertEqual(parser._wsp_splitter(s, 1), res)

    params_test__wsp_splitter = Params(
        one_word = C('foo', ['foo']),
        two_words = C('foo def', ['foo', ' ', 'def']),
        ws_runs = C('foo \t def jik', ['foo', ' \t ', 'def jik']),
        )


    # _make_xtext

    @params
    def test__make_xtext(
            self,
            s,
            terminal_class=parser.ValueTerminal,
            token_type='test',
            **kw,
        ):
        vt = self._test_parse(
            parser._make_xtext,
            C(s, terminal_class, token_type),
            stringified=('' if terminal_class.__name__.startswith('EW')
                            else None),
            value=' ' if terminal_class.__name__.startswith('White') else None,
            test_start=False,
            **kw,
            )
        self.assertEqual(vt.token_type, token_type)

    @params_map
    def for_each_terminal_type(*args, **kw):
        vt_types = (
            parser.ValueTerminal,
            parser.WhiteSpaceTerminal,
            parser.EWWhiteSpaceTerminal,
            )
        for vt_type in vt_types:
            yield vt_type.__name__, C(*args, **kw, terminal_class=vt_type)

    params_test__make_xtext = for_each_terminal_type(

        token_type = C('foo', token_type='bar'),

        # XXX POSTDEP: delete from here...

        )


    # _validate_xtext

    @params
    def test__validate_xtext(self, s, defects=[]):
        vt = parser.ValueTerminal(s, 'test')
        parser._validate_xtext(vt)
        self.assertDefectsMatch(vt.defects, defects)

    params_test__validate_xtext = Params(

        # XXX POSTDEP: ...to here

        valid = C('foo'),

        # Although it looks a bit odd for unicode to be acceptable when we have
        # a non-ascii error, the parser in fact handles unicode.
        unicode = C('föö'),

        # The non-ascii error arises only if the input was supposed to be 7-bit
        # ASCII but in fact had non-ascii in it, in which case those bytes end
        # up as surrogates.  Thus the name of the defect.
        surrogates = C(
            'föö'.encode().decode('ascii', 'surrogateescape'),
            # "Non-ASCII characters found in header token"
            defects=[undecodable_bytes_defect],
            ),

        multiple_nps = C(
            'a\ttab spaces and\rcarriage return',
            defects=[(nonprintable_defect, '\t  \r ')],
            ),

        nps_and_surrogates = C(
            'föö\t'.encode().decode('ascii', 'surrogateescape'),
            defects=[undecodable_bytes_defect, nonprintable_defect('\t')],
            ),

        **for_each_character(RFC_NONPRINTABLES)(
            non_printable = C(
                'f{char}o',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        )

    # XXX POSTDEP: delete from here...
    params_test__make_xtext.update(
        add_label('from_test_validate_xtext')(
            for_each_terminal_type(params_test__validate_xtext),
            ),
        )
    # XXX POSTDEP: ...to here.


    # _get_xtext

    @params
    def test__get_xtext(
            self,
            s,
            # DOTALL allows the LF in our first test set to pass...in the
            # normal use of _get_xtext LF will terminate the matches we use,
            # leaving the LF (which shouldn't normally happen) for later code.
            regex=re.compile('.*', re.DOTALL),
            terminal_class=parser.ValueTerminal,
            token_type='test',
            err=None,
            **kw,
        ):
        vt = self._test_parse(
            parser._get_xtext,
            C(s, regex, terminal_class, token_type, err=err),
            stringified=('' if terminal_class.__name__.startswith('EW')
                            else None),
            value=' ' if terminal_class.__name__.startswith('White') else None,
            **kw,
            )
        if 'exception' in kw:
            return
        self.assertEqual(vt.token_type, token_type)

    params_test__get_xtext__regex = Params(

        params_test__make_xtext,

        raises_on_no_match = C(
            'foo bar',
            regex=re.compile(r'x'),
            err=Exception('foo'),
            exception=(Exception, 'foo'),
            ),

        returns_match = C(
            'foo bar',
            regex=re.compile(r'[^ ]+'),
            remainder=' bar',
            ),

        ignores_non_printable_after_match = C(
            'foobar\x00',
            regex=re.compile(r'[^b]+'),
            remainder='bar\x00',
            ),

        **for_each_character(RFC_WSP + '()')(
            regex_from_make_non_match_re = C(
                'foo{char}bar',
                regex=parser._make_non_match_re(RFC_WSP + '()'),
                remainder='{char}bar',
                ),
            ),

        )


    # _get_ptext_to_endchars

    # As an internal method these tests are not API requirements; however, the
    # behavior they check must be verified one way or another, so if the
    # implementation changes there need to be equivalent tests.

    @params
    def test__get_ptext_to_endchars(self, s, endchars, has_qp=False, **kw):
        ptext, had_qp = self._test_parse(
            parser._get_ptext_to_endchars,
            C(s, endchars),
            test_start=False,
            **kw,
            )
        self.assertEqual(had_qp, has_qp)

    @params_map
    def for_each_endchar_set(*args, **kw):
        # The function is general, but these are the ones we actually use.
        endchar_sets = dict(
            quoted_string='"',
            comment='()',
            domain_literal='[]',
            )
        for name, endchars in endchar_sets.items():
            yield name, C(*args, endchars=endchars, **kw)

    @params_map
    def for_each_endchar(*args, **kw):
        return for_each_character(kw['endchars'])(C(*args, **kw)).items()

    # This params_map is used on exactly one expression, which has to contain a
    # list of characters with no repeats.
    @params_map
    def stops_at_first_endchar_found(s):
        for i in range(len(s)):
            endchars = ''.join(sample((r := s[i:]), len(r)))
            ec = charname(s[i])
            yield f'stops_at_first_endchar_found__string__{ec}', C(
                s,
                endchars=endchars,
                remainder=r,
                )
            yield f'stops_at_first_endchar_found__set__{ec}', C(
                s,
                endchars=set(endchars),
                remainder=r,
                )

    params_test__get_ptext_to_endchars = Params(

        **for_each_endchar(
            wsp_can_be_legal_endchars = C(
                'foo{char}bar"',
                endchars='()' + RFC_WSP,
                remainder='{char}bar"',
                ),
            ),

        **stops_at_first_endchar_found('(random?{})'),

        **for_each_endchar_set(

            one_word_no_wsp = C(
                'foo',
                ),

            escaped_letter = C(
                r'bar\s',
                stringified='bars',
                has_qp=True,
                ),

            escaped_escape_char = C(
                r'foo\\bar',
                stringified=r'foo\bar',
                has_qp=True,
                ),

            any_printable_may_be_quoted = C(
                ''.join(rf'\{c}' for c in RFC_PRINTABLES),
                stringified=RFC_PRINTABLES,
                has_qp=True,
                ),

            ),

        **for_each_endchar(
            for_each_endchar_set(

                stops_at_endchar = C(
                    'foo{char}bar"',
                    remainder='{char}bar"',
                    ),

                quoted_endchar_no_actual_endchar = C(
                    r'foo\{char}bar',
                    stringified=r'foo{char}bar',
                    has_qp=True,
                    ),

                quoted_endchar_before_actual_endchar = C(
                    r'foo\{char}bar{char}',
                    stringified='foo{char}bar',
                    remainder='{char}',
                    has_qp=True,
                    ),

                multiple_qp = C(
                    r'\{char}\foo\\\{char}\a{char}',
                    stringified=r'{char}foo\{char}a',
                    remainder=r'{char}',
                    has_qp=True,
                    ),

                no_qp_before_endchar_but_some_after = C(
                    r'foo{char}a\b\a\r',
                    remainder=r'{char}a\b\a\r',
                    has_qp=False,
                    ),

                ),
            ),

        )


    # get_fws

    @params
    def test_get_fws(self, s, *args, **kw):
        fws = self._test_parse(parser.get_fws, C(s), *args, value=' ', **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(fws, parser.WhiteSpaceTerminal)
        self.assertEqual(fws.token_type, 'fws')

    # XXX POSTDEP: delete from here...
    @params_map
    def deprecate_oldapi_no_raise_behavior(*args, **kw):
        kw['warnings'] = kw.get('warnings', []) + [
            (DeprecationWarning, r'.*API.*has changed'),
            (DeprecationWarning, r'(?i).*raise'),
            ]
        yield 'oldapi', C(*args, **kw, test_start=False)
    # XXX POSTDEP: ...to here.

    params_test_get_fws = for_each_api(

        wsp_run = C(' \t  '),

        **for_each_character(RFC_WSP)(
            ends_at_non_wsp_after_wsp = C('{char}foo', remainder='foo'),
            ),

        **for_each_character(RFC_PRINTABLES)(
            ends_at_non_wsp_after_wsp_run = C(' \t{char} ', remainder='{char} '),
            ),

    # XXX POSTDEP: delete from here...
        )

    # These ought to error, but get_fws should never be called this way
    # We'll deprecate the lack of raise during the refactor.
    params_test_get_fws.update(
        deprecate_oldapi_no_raise_behavior(
            empty = C(''),
            no_wsp = C('foo', remainder='foo'),
            no_leading_wsp = C('foo bar', remainder='foo bar'),
            ),
        )

    params_test_get_fws.update(
        add_label('newapi')(
    # XXX POStDEP: ... to here.  And fix the indentation below.
            **params_map(
                lambda s, **k: only(
                    C(s, exception=(errors.HeaderParseError, '(?i)expected'))
                    )
                )(
                empty='',
                no_wsp='foo',
                no_leading_wsp='foo bar',
                )
            ),  # XXX POSTDEP: delete this line

        )


    # get_encoded_word

    @params
    def test_get_encoded_word(
            self,
            s,
            *args,
            charset='us-ascii',
            lang='',
            terminal_type=None,
            **kw,
            ):
        callspec = C(s) if terminal_type is None else C(s, terminal_type)
        res = self._test_parse(parser.get_encoded_word, callspec, *args, **kw)
        if 'exception' in kw:
            return
        self.assertEqual(res.charset, charset)
        self.assertEqual(res.lang, lang)
        terminal_type = 'vtext' if terminal_type is None else terminal_type
        self.verify_terminal_types(res, terminal_type, 'fws')

    # This params_map will handle either single strings or C objects.
    @params_map
    def expect_get_encoded_word_raise(v, *args, **kw):
        newspec = C(
            v,
            *args,
            # "expected encoded word but found '...'"
            exception=(errors.HeaderParseError, re.escape(v)),
            test_start=False,
            **kw,
            )
        yield 'oldapi', newspec

    params_test_get_encoded_word__invalid_input = expect_get_encoded_word_raise(
        null_string =                               '',
        no_chrome =                                 'content',
        eq_only =                                   '=content',
        start_chrome_only =                         '=?',
        start_and_charset_only =                    '=?UTF-8',
        start_charset_qm_only =                     '=?UTF-8?',
        start_charset_qm_cte_only =                 '=?UTF-8?q',
        start_charset_qm_cte_qm_only =              '=?UTF-8?q?',
        start_charset_qm_cte_qm_content_only =      '=?UTF-8?q?content',
        start_charset_qm_cte_qm_content_qm_only =   '=?UTF-8?q?content?',
        end_eq_only =                               'content=',
        end_chrome_only =                           '?=',
        end_and_content_only =                      'content?=',
        end_content_eq_only =                       '?content?=',
        end_content_eq_cte_only =                   'q?content?=',
        end_content_eq_cte_eq_only =                '?q?content?=',
        end_content_eq_cte_eq_charset_only =        'UTF-8?q?content?=',
        end_content_eq_cte_eq_charset_eq_only =     '?UTF-8?q?content?=',
        missing_both_middle =                       '=?content?=',
        missing_one_middle =                        '=?q?content?=',
        empty_cte =                                 '=UTF-8??content?=',
        empty_charset_and_cte =                     '=???content?=',
        empty_everything =                          '=????=',
        unknown_cte =                               '=?UTF-8?X?content?=',
        invalid_base64_length =                     '=?utf-8?b?abcde?=',
        multicharacter_cte =                        '=?UTF-8?qq?content?=',
        too_many_qm =                               '=?UTF-8?q?q?content?=',
        empty_lang =                                '=?UTF-8*??q?content?=',
        lang_with_empty_charset =                   '=?*foo??q?content?=',
        **for_each_character(ALL_ASCII)(
            character_before_valid_ew = C('{char}=?us-ascii?q?test?='),
            ),
        )

    params_test_get_encoded_word = old_api_only(

        valid_ew = C(
            '=?us-ascii?q?this_is_a_test?=  bird',
            stringified='this is a test',
            remainder='  bird',
            ),

        # XXX XXX the skip for the RFC_WSP will go away after refactor.  It's
        # here because it would be a pain to handle the lack of the defect,
        # which will go away in the refactor.
        **for_each_character(ALL_ASCII, skip=RFC_WSP)(
            ew_followed_by = C(
                '=?us-ascii?q?foo?={char}',
                stringified='foo',
                remainder='{char}',
                defects=[missing_whitespace_after_ew_defect],
                ),
            ),

        # XXX some of these characters should result in defects depending on
        # the context from which get_encoded_word is called (ex: ()s are
        # illegal in comment encoded words), but but at least at the moment
        # that it isn't worth the effort to implement.
        # XXX XXX the skip for ? is a bug which will be fixed in the refactor
        **for_each_character(RFC_PRINTABLES, skip='_?')(
            q_content_may_contain = C(
                '=?us-ascii?q?foo_{char}_bar_{char}?=',
                stringified='foo {char} bar {char}',
                )
            ),

        internal_spaces = C(
            '=?us-ascii?q?this is a test?=  bird',
            stringified='this is a test',
            # 'whitespace inside encoded word'
            defects=[whitespace_inside_ew_defect],
            remainder='  bird',
            ),

        only_gets_first_ew = C(
            '=?us-ascii?q?first?=  =?utf-8?q?second?=',
            stringified='first',
            remainder='  =?utf-8?q?second?=',
            ),

        # XXX XXX This defect will also go away (gets detected higher up)
        only_gets_first_ew_even_if_no_space = C(
            '=?us-ascii?q?first?==?utf-8?q?second?=',
            stringified='first',
            # 'missing trailing whitespace after encoded-word'
            defects=[missing_whitespace_after_ew_defect],
            remainder='=?utf-8?q?second?=',
            ),

        lang_set = C(
            '=?us-ascii*jive?q?first_second?=',
            stringified='first second',
            lang='jive',
            ),

        utf8_charset = C(
            '=?utf-8?q?first_second?=',
            stringified='first second',
            charset='utf-8',
            ),

        **for_each_character(
                RFC_NONPRINTABLES,
                # XXX XXX skip things split considers whitespace. This is buggy.
                #                         US  RS  GS  FS
                skip=RFC_WSP + '\r\n\v\f\x1f\x1e\x1d\x1c',
                )(
            non_printable_defect = C(
                '=?us-ascii?q?first{char}second?=',
                stringified='first{char}second',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        # Note that other characters may work as well, but these *must* work.
        **for_each_character(RFC_CHARSET_CHARS)(
            char_valid_in_charset_name = C(
                '=?a_bad_{char}set_name?q?foo?=',
                stringified='foo',
                defects=[(charset_defect('a_bad_{echar}set_name'))],
                charset='a_bad_{char}set_name',
                ),
            ),

        leading_internal_encoded_space = C(
            '=?us-ascii?q?=20foo?=',
            stringified=' foo',
            ),

        leading_internal_unencoded_space = C(
            '=?us-ascii?q? foo?=',
            stringified=' foo',
            defects=[whitespace_inside_ew_defect],
            ),

        trailing_internal_encoded_space = C(
            '=?us-ascii?q?foo=20_?=  bird',
            stringified='foo  ',
            value='foo ',
            remainder='  bird',
            ),

        trailing_internal_unencoded_space = C(
            '=?us-ascii?q?foo _ ?=  bird',
            stringified='foo   ',
            value='foo ',
            defects=[whitespace_inside_ew_defect],
            remainder='  bird',
            ),

        # Issue 18044
        quopri_utf_escape_follows_cte = C(
            '=?utf-8?q?=C3=89ric?=',
            stringified='Éric',
            charset='utf-8',
            ),

        unknown_charset_leads_to_undecodable_bytes_with_non_ascii = C(
            '=?invalid?q?=C3=89ric?=',
            stringified='\udcc3\udc89ric',
            charset='invalid',
            defects=[charset_defect('invalid'), undecodable_bytes_defect],
            ),

        empty_charset = C(
            '=??q?content?=',
            stringified='content',
            charset='',
            defects=[charset_defect('')],
            ),

        missing_base64_padding = C(
            '=?us-ascii?b?dmk?=',
            stringified='vi',
            defects=[invalid_base64_padding_defect],
            ),


        invalid_base64_character = C(
            '=?us-ascii?b?dm\x01k===?=',
            stringified='vi',
            defects=[invalid_base64_characters_defect],
            ),

        invalid_base64_character_and_bad_padding = C(
            '=?us-ascii?b?dm\x01k?=',
            stringified='vi',
            defects=[
                invalid_base64_padding_defect,
                invalid_base64_characters_defect,
                ],
            ),

        ws_only_charset_leads_to_undecodable_bytes_with_non_ascii = C(
            '=? * ?q?=C3=89ric?=',
            stringified='\udcc3\udc89ric',
            charset='',
            defects=[
               charset_defect(' '),
               undecodable_bytes_defect,
               whitespace_inside_ew_defect,
               ],
            ),

        eq_is_only_special_with_two_digits_after_it = C(
            '=?UTF-8?q?=C3=89ric_=_?=',
            stringified='Éric = ',
            charset='UTF-8',
            ),

        ws_around_charset_and_lang = C(
            '=?  us-ascii\t* jive\t ?q?test?=  bird',
            stringified='test',
            lang='jive',
            defects=[whitespace_inside_ew_defect],
            remainder='  bird',
            ),

        set_terminal_type_on_single_word_content = C(
            '=?us-ascii?q?text?=',
            stringified='text',
            terminal_type='test',
            ),

        set_terminal_type_on_multiple_word_content = C(
            '=?us-ascii?q?text_and_more_text?=',
            stringified='text and more text',
            terminal_type='test',
            ),

        )


    # get_unstructured

    @params
    def test_get_unstructured(self, s, *args, **kw):
        result = self._test_parse(
            parser.get_unstructured,
            C(s),
            *args,
            test_start=False,
            warnings=...,   # XXX XXX ignore warnings until after refactor.
            **kw,
            )
        self.assertIsInstance(result, parser.UnstructuredTokenList)
        self.verify_terminal_types(result, 'utext', 'fws')

    # get_unstructured should correctly decode anything get_encoded_word does,
    # so it should correctly handle most get_encoded_word parameters.
    @params_map(with_namelist=True)
    def adapt_get_encoded_word_tests_for_get_unstructured(nl, *args, **kw):
        kw.pop('test_start')
        kw.pop('charset', None)
        kw.pop('terminal_type', None)
        kw.pop('lang', None)
        # get_unstructured parses all of its input, so it will also parse and
        # return anything get_encoded_word treats as a remainder.
        remainder = kw.pop('remainder', '')
        if '=?' in remainder or 'ew_followed_by' in nl:
            # The remainder includes something get_unstructured would decode,
            # or might contain something it would treat as a defect.  Either
            # way, parse_unstructured isn't expected to handle those parameters.
            return
        if 'stringified' in kw:
            stringified = kw['stringified']
            kw['stringified'] = stringified + remainder
        rstripped = remainder.lstrip(RFC_WSP)
        if remainder != rstripped:
            kw['value'] = kw.get('value', stringified) + ' ' + rstripped
        # Drop the 'warning=...' added by only_old_api; we're doing it ourselves
        # in the test method.
        kw.pop('warnings')
        yield 'from_test_get_encoded_word', C(*args, **kw)

    @params_map(with_namelist=True)
    def adapt_get_encoded_word_invalid_input_for_get_unstructured(nl, s, **kw):
        # Get unstructured should return the inputs unaltered,
        # except for the ones where the ew itself is valid.
        if 'character_before_valid_ew' in nl:
            return
        yield 'from_test_get_encoded_word_invalid_input', C(s)

    @params_map
    def add_unstructured_prefix_and_suffix(s, *args, **kw):
        # Make sure the reused parameters are correctly interpreted when
        # intermixed with other text by adding some text.
        pad = lambda s: f'pre fix {s} suf fix'
        if not s:
            # null value is a special case, and we already have a test for it.
            return
        s = pad(s)
        kw = {n: (pad(v) if n in ('stringified', 'value') else v)
              for n, v in kw.items()
              }
        yield '', C(s, *args, **kw)

    params_test_get_unstructured = Params(

        add_unstructured_prefix_and_suffix(
            adapt_get_encoded_word_tests_for_get_unstructured(
                params_test_get_encoded_word,
                ),
            adapt_get_encoded_word_invalid_input_for_get_unstructured(
                params_test_get_encoded_word__invalid_input,
                ),
            ),

        null = C(
            '',
            ),

        one_word = C(
            'foo',
            ),

        normal_phrase = C(
            'foo bar bird',
            ),

        normal_phrase_with_whitespace = C(
            'foo \t bar      bird',
            value='foo bar bird',
            ),

        leading_whitespace = C(
            '  foo bar',
            value=' foo bar',
            ),

        trailing_whitespace = C(
            'foo bar  ',
            value='foo bar ',
            ),

        leading_and_trailing_whitespace = C(
            '  foo bar  ',
            value=' foo bar ',
            ),

        one_valid_ew_no_ws = C(
            '=?us-ascii?q?bar?=',
            stringified='bar',
            value='bar',
            ),

        one_ew_trailing_ws = C(
            '=?us-ascii?q?bar?=  ',
            stringified='bar  ',
            value='bar ',
            ),

        one_valid_ew_trailing_text = C(
            '=?us-ascii?q?bar?= bird',
            stringified='bar bird',
            ),

        phrase_with_ew_in_middle_of_text = C(
            'foo =?us-ascii?q?bar?= bird',
            stringified='foo bar bird',
            ),

        phrase_with_two_ew = C(
            'foo =?us-ascii?q?bar?= =?us-ascii?q?bird?=',
            stringified='foo barbird',
            ),

        phrase_with_two_ew_trailing_ws = C(
            'foo =?us-ascii?q?bar?= =?us-ascii?q?bird?=   ',
            stringified='foo barbird   ',
            value='foo barbird ',
            ),

        phrase_with_ew_with_leading_ws = C(
            '  =?us-ascii?q?bar?=',
            stringified='  bar',
            value=' bar',
            ),

        phrase_with_two_ew_extra_ws = C(
            'foo =?us-ascii?q?bar?= \t  =?us-ascii?q?bird?=',
            stringified='foo barbird',
            ),

        two_ew_extra_ws_trailing_text = C(
            '=?us-ascii?q?test?=   =?us-ascii?q?foo?=  val',
            stringified='testfoo  val',
            value='testfoo val',
            ),

        ew_with_internal_ws = C(
            '=?iso-8859-1?q?hello=20world?=',
            stringified='hello world',
            ),

        ew_with_internal_leading_ws = C(
            '   =?us-ascii?q?=20test?=   =?us-ascii?q?=20foo?=  val',
            stringified='    test foo  val',
            value='  test foo val',
            ),

        invalid_ew = C(
            '=?test val',
            ),

        undecodable_bytes = C(
            b'test \xACfoo  val'.decode('ascii', 'surrogateescape'),
            stringified='test \uDCACfoo  val',
            value='test \uDCACfoo val',
            defects=[undecodable_bytes_defect],
            ),

        undecodable_bytes_in_EW = C(
            (b'=?us-ascii?q?=20test?=   =?us-ascii?q?=20\xACfoo?='
                b'  val').decode('ascii', 'surrogateescape'),
            stringified=' test \uDCACfoo  val',
            value=' test \uDCACfoo val',
            defects=[
                undecodable_bytes_defect,
                (undecodable_bytes_in_ew_defect, 'us-ascii'),
                ],
            ),


        no_whitespace_between_ews = C(
            '=?utf-8?q?foo?==?utf-8?q?bar?=',
            stringified='foobar',
            defects=[
                missing_whitespace_after_ew_defect,
                missing_whitespace_before_ew_defect,
                ],
            ),

        ew_without_leading_whitespace = C(
            'nowhitespace=?utf-8?q?somevalue?=',
            stringified='nowhitespacesomevalue',
            defects=[missing_whitespace_before_ew_defect],
            ),

        ew_without_trailing_whitespace = C(
            '=?utf-8?q?somevalue?=nowhitespace',
            stringified='somevaluenowhitespace',
            defects=[missing_whitespace_after_ew_defect],
            ),

        # bpo-37764
        without_trailing_whitespace_hang_case = C(
            '=?utf-8?q?somevalue?=aa',
            stringified='somevalueaa',
            defects=[missing_whitespace_after_ew_defect],
            ),

        invalid_ew2 = C(
            '=?utf-8?q?=somevalue?=',
            ),

        **for_each_character(RFC_PRINTABLES)(
            printable_around_and_between_ews = C(
                '{char} =?utf-8?q?foo?= {char} =?utf-8?q?bar?= {char}',
                stringified='{char} foo {char} bar {char}',
                ),
            ),

        # XXX XXX the '?=' skip is a sort-of bug the refactoring will fix.
        **for_each_character(RFC_PRINTABLES, skip='_?=')(
            printable_inside_ews = C(
                '=?utf-8?q?rock{char}?= =?utf-8?q?{char}hard_place?=',
                stringified='rock{char}{char}hard place',
                ),
            ),

        **for_each_character(
                RFC_NONPRINTABLES,
                # XXX XXX skip things split considers whitespace. This is buggy.
                #                         US  RS  GS  FS
                skip=RFC_WSP + '\r\n\v\f\x1f\x1e\x1d\x1c',
                )(
            non_wsp_non_printable = C(
                'some {char} text',
                stringified='some {char} text',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_wsp_non_printable_inside_ew = C(
                '=?utf-8?q?some{char}?= text',
                stringified='some{char} text',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        unicode = C(
            '📦',
            ),

        non_ascii_bytes = C(
            '📦'.encode().decode('ascii', 'surrogateescape'),
            defects=[undecodable_bytes_defect],
            ),

        invalid_ew_charset = C(
            'a =?invalid?q?=C3=89ric?= b',
            stringified='a \udcc3\udc89ric b',
            defects=[charset_defect('invalid'), undecodable_bytes_defect],
            ),

        ew_start_chrome_before_real_ew = C(
            'z=?xx =?UTF-8?Q?foo?=',
            stringified='z=?xx foo',
            ),

        )


    # get_qp_ctext

    @params
    def test_get_qp_ctext(self, s, *args, value=' ', **kw):
        ptext = self._test_parse(
            parser.get_qp_ctext,
            C(s),
            *args,
            value=value,
            **kw,
            )
        self.assertIsInstance(ptext, parser.Terminal)
        self.assertEqual(ptext.token_type, 'ptext')

    params_test_get_qp_ctext = old_api_only(

        value_ends_at_input_end = C(
            'foobar',
            ),

        all_printables = C(
            RFC_PRINTABLES.
                replace('\\', r'\\').replace('(', r'\(').replace(')', r'\)'),
            stringified=RFC_PRINTABLES,
            ),

        two_words_gets_first = C(
            'foo de',
            remainder=' de',
            ),

        following_wsp_preserved = C(
            'foo \t\tde',
            remainder=' \t\tde',
            ),

        up_to_close_paren_only = C(
            'foo)',
            remainder=')',
            ),

        wsp_before_close_paren_preserved = C(
            'foo  )',
            remainder='  )',
            ),

        close_paren_mid_word = C(
            'foo)bar',
            remainder=')bar',
            ),

        up_to_open_paren_only = C(
            'foo(',
            remainder='(',
            ),

        wsp_before_open_paren_preserved = C(
            'foo  (',
            remainder='  (',
            ),

        open_paren_mid_word = C(
            'foo(bar',
            remainder='(bar',
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printables = C(
                'foo{char}bar)',
                defects=[(nonprintable_defect, '{char}')],
                remainder=')',
                ),
            ),

        close_paren_only = C(
            ')',
            remainder=')',
            ),

        open_paren_only = C(
            '(',
            remainder='(',
            ),

        no_content = C(
            '',
            ),

        parens_are_content_if_quoted = C(
            r'\(bar\)\)bird\(',
            stringified='(bar))bird(',
            ),

        escapes_are_removed_in_str = C(
            r'fairly\&\boring\W\@\!ks',
            stringified='fairly&boringW@!ks',
            ),

        any_printable_may_be_escaped = C(
            ''.join(rf'\{c}' for c in RFC_PRINTABLES),
            RFC_PRINTABLES,
            ),

        unicode_content = C(
            '⛔❌❗',
            ),

        mixed_unicode_and_ascii = C(
            'ministry✌of⛔silly❌walks❗',
            ),

        unicode_can_be_quoted = C(
            r'sillier\❌walks\❗',
            stringified='sillier❌walks❗',
            ),

        )


    # get_qcontent

    @params
    def test_get_qcontent(self, s, *args, **kw):
        ptext = self._test_parse(parser.get_qcontent, C(s), *args, **kw)
        self.assertIsInstance(ptext, parser.Terminal)
        self.assertEqual(ptext.token_type, 'ptext')

    params_test_get_qcontent = old_api_only(

        no_qp_no_end_char = C(
            'foobar',
            ),

        all_printables = C(
            RFC_PRINTABLES.replace('\\', r'\\').replace('"', r'\"'),
            stringified=RFC_PRINTABLES,
            ),

        **for_each_character(RFC_WSP)(
            two_words_gets_first = C(
                 'foo{char}de',
                 remainder='{char}de',
                 ),
            ),

        following_wsp_preserved = C(
            'foo \t\tde',
            remainder=' \t\tde',
            ),

        up_to_dquote_only = C(
            'foo"',
            remainder='"',
            ),

        wsp_before_dquote_preserved = C(
            'foo  "',
            remainder='  "',
            ),

        dquote_mid_word = C(
            'foo"bar',
            remainder='"bar',
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable = C(
                'foo{char}bar"',
                defects=[(nonprintable_defect, '{char}')],
                remainder='"',
                ),
            ),

        no_content_before_dquote = C(
            '"',
            remainder='"',
            ),

        empty_value = C(
            '',
            ),

        )


    # get_atext

    @params
    def test_get_atext(self, s, *args, **kw):
        atext = self._test_parse(parser.get_atext, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(atext, parser.Terminal)
        self.assertEqual(atext.token_type, 'atext')

    params_test_get_atext = old_api_only(

        only = C(
            'foobar',
            ),

        all_atext = C(
             RFC_ATEXT,
             ),

        two_words_gets_first = C(
            'foo bar',
            remainder=' bar',
            ),

        following_wsp_preserved = C(
            'foo \t\tbar',
            remainder=' \t\tbar',
            ),

        **for_each_character(RFC_SPECIALS)(
            up_to_special = C(
                RFC_ATEXT.
                    replace('{', '{{').replace('}', '}}') + '{char}' + 'bar',
                remainder='{char}bar',
                ),
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printables = C(
                'foo{char}bar(',
                defects=[(nonprintable_defect, '{char}')],
                remainder='(',
                ),
            ),

        **for_each_character(RFC_SPECIALS + RFC_WSP)(
            no_atext_before_special_or_wsp = C(
                '{char}foo',
                exception=(errors.HeaderParseError, '{echar}foo'),
                ),
            ),

        undecodable_characters = C(
            'foo🎁bar'.encode().decode('us-ascii', errors='surrogateescape'),
            defects=[undecodable_bytes_defect],
            ),

        empty = C(
            '',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        )


    # get_bare_quoted_string

    @params
    def test_get_bare_quoted_string(self, s, *args, **kw):
        bqs = self._test_parse(
            parser.get_bare_quoted_string,
            C(s),
            *args,
            **kw,
            )
        if 'exception' in kw:
            return
        self.assertIsInstance(bqs, parser.BareQuotedString)
        self.assertEqual(bqs.token_type, 'bare-quoted-string')
        self.verify_terminal_types(bqs, 'ptext', 'fws')

    params_test_get_bare_quoted_string = old_api_only(

        non_ws = C(
            '"foo"',
            value='foo',
            ),

        no_leading_dquote_before_non_ws = C(
            'foo"',
            exception=(errors.HeaderParseError, 'expected.*foo'),
            ),

        no_leading_dquote_before_ws = C(
            '  "foo"',
            exception=(errors.HeaderParseError, 'expected.*"foo"'),
            ),

        only_quotes = C(
            '""',
            value='',
            ),

        missing_endquote = C(
            '"',
            stringified='""',
            value='',
            defects=[end_inside_quoted_string_defect],
            ),

        following_wsp_preserved = C(
            '"foo"\t bar',
            value='foo',
            remainder='\t bar',
            ),

        multiple_words = C(
            '"foo bar moo"',
            value='foo bar moo',
            ),

        multiple_words_wsp_preserved = C(
            '" foo  moo\t"',
            value=' foo  moo\t',
            ),

        end_dquote_mid_word = C(
            '"foo"bar',
            value='foo',
            remainder='bar',
            ),

        quoted_dquote = C(
            r'"foo\"in"@',
            value='foo"in',
            remainder='@',
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printables = C(
                 '"a{char}a"',
                 value='a{char}a',
                 defects=[(nonprintable_defect, '{char}')],
                 ),
            ),

        all_printables_allowed = C(
            f'"{RFC_PRINTABLES.replace('\\', r'\\').replace('"', r'\"')}"',
            value=RFC_PRINTABLES,
            ),

        any_printable_may_be_escaped = C(
            f'"{''.join(rf'\{c}' for c in RFC_PRINTABLES)}"',
            stringified=
                f'"{RFC_PRINTABLES.replace('\\', r'\\').replace('"', r'\"')}"',
            value=RFC_PRINTABLES,
            ),

        no_end_dquote_after_non_ws = C(
            '"foo',
            stringified='"foo"',
            value='foo',
            defects=[end_inside_quoted_string_defect],
            ),

        no_end_dquote_after_ws = C(
            '"foo ',
            stringified='"foo "',
            value='foo ',
            defects=[end_inside_quoted_string_defect],
            ),

        # Issue 16983: apply postel's law to some bad encoding.
        encoded_word_inside_quotes = C(
            '"=?utf-8?Q?not_really_valid?="',
            stringified='"not really valid"',
            value='not really valid',
            defects=[
                ew_inside_quoted_string_defect,
                missing_whitespace_after_ew_defect,
                ],
            ),

        # XXX XXX The decode failure here will be fixed in the refactor.
        mixed_encoded_words_and_regular_text = C(
            '"This has=?utf-8?Q?multiple?= =?utf-8?q?errors?=in it',
            stringified='"This has=?utf-8?Q?multiple?= errorsin it"',
            value='This has=?utf-8?Q?multiple?= errorsin it',
            defects=[
                ew_inside_quoted_string_defect,
                missing_whitespace_after_ew_defect,
                end_inside_quoted_string_defect,
                ],
            ),

        encoded_word_after_dquote_with_no_ws = C(
            '"test"of=?UTF-8?q?bad?=data',
            value='test',
            remainder='of=?UTF-8?q?bad?=data',
            ),

        invalid_charset = C(
            '"=?foo?Q?not_really_valid?= at all"',
            stringified='"not really valid at all"',
            value='not really valid at all',
            defects=[
                ew_inside_quoted_string_defect,
                charset_defect('foo'),
                ],
            ),

        empty = C(
            '',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        )


    # get_comment

    @params
    def test_get_comment(self,
            s,
            *args,
            value=' ',
            comments=None,
            content=None,
            commenttree=None,
            **kw):
        if content is None:
            content = comments[0] if comments else None
        if commenttree is None:
            commenttree = [content]
        cmt = self._test_parse(
            parser.get_comment,
            C(s),
            *args,
            value=value,
            comments=comments,
            commenttree=commenttree,
            **kw,
            )
        if 'exception' in kw:
            return
        self.assertEqual(cmt.content, content)
        self.assertIsInstance(cmt, parser.Comment)
        self.assertEqual(cmt.token_type, 'comment')
        self.verify_terminal_types(cmt, 'ptext', 'fws')

    params_test_get_comment = old_api_only(

        simple_comment_only = C(
            '(comment)',
            comments=['comment'],
            ),

        non_wsp_before_left_paren_is_error = C(
            'foo"',
            exception=(errors.HeaderParseError, r'(?=.*expected)(?=.*foo)'),
            ),

        wsp_before_left_paren_is_error = C(
            '  (foo"',
            exception=(errors.HeaderParseError, r'(?=.*expected)(?=.* \(foo)'),
            ),

        wsp_after_right_paren = C(
            '(comment)  \t',
            remainder='  \t',
            comments=['comment'],
            ),

        multiple_words = C(
            '(foo bar)',
            comments=['foo bar'],
            ),

        wsp_runs_inside_comment = C(
            '( foo  bar\t )',
            comments=[' foo  bar\t '],
            ),

        non_wsp_after_right_paren = C(
            '(foo)bar',
            remainder='bar',
            comments=['foo'],
            ),

        quoted_parens = C(
            r'(foo\) \(\)bar)',
            comments=['foo) ()bar'],
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable = C(
                '(foo{char}bar)',
                defects=[(nonprintable_defect, '{char}')],
                comments=['foo{char}bar'],
                ),
            ),

        no_right_paren_after_non_ws = C(
            '(foo bar',
            stringified='(foo bar)',
            defects=[end_inside_comment_defect],
            comments=['foo bar'],
            ),

        no_right_paren_after_ws = C(
            '(foo bar  ',
            stringified='(foo bar  )',
            defects=[end_inside_comment_defect],
            comments=['foo bar  '],
            ),

        nested_comment = C(
            '(foo(bar))',
            comments=['foo(bar)'],
            commenttree=['foo', ['bar']],
            ),

        nested_comment_wsp = C(
            '(foo ( bar ) )',
            comments=['foo ( bar ) '],
            commenttree=['foo ', [' bar '], ' '],
            ),

        empty_comment = C(
            '()',
            comments=[''],
            commenttree=[''],
            ),

        multiple_nesting = C(
            '(((((foo)))))',
            comments=['((((foo))))'],
            commenttree=[[[[['foo']]]]],
            ),

        multiple_mesting_missing_two_right_parens = C(
            '(((((foo)))',
            stringified='(((((foo)))))',
            defects=[*[end_inside_comment_defect]*2],
            comments=['((((foo))))'],
            commenttree=[[[[['foo']]]]],
            ),

        quoted_paren_in_nested_comment = C(
            r'(foo (b\)))',
            comments=[r'foo (b\))'],
            commenttree=['foo ', ['b)']],
            ),

        any_printable_may_be_escaped = C(
            f"({''.join(fr'\{c}' for c in RFC_PRINTABLES)})",
            stringified=
                f"({RFC_PRINTABLES
                    .replace('\\', r'\\')
                    .replace('(', r'\(')
                    .replace(')', r'\)')
                    })",
            comments=[RFC_PRINTABLES],
            ),

        all_printables = C(
            f"({RFC_PRINTABLES.
                replace('\\', r'\\').replace('(', r'\(').replace(')', r'\)')})",
            comments=[RFC_PRINTABLES],
            ),

        multiple_nested_comments = C(
            '(foo (nest 1) (nest 2 (nest 3)))',
            comments=['foo (nest 1) (nest 2 (nest 3))'],
            commenttree=['foo ', ['nest 1'], ' ', ['nest 2 ', ['nest 3']]],
            ),

        nested_empty_comments = C(
            '( () ( (   ) ) )',
            comments=[' () ( (   ) ) '],
            commenttree=[' ', [''], ' ', [' ', ['   '], ' '], ' '],
            ),

        empty = C(
            '',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        ew_after_comment_no_ws = C(
            '(foo)=?UTF-8?q?foo?=',
            stringified='(foo)',
            comments=['foo'],
            remainder='=?UTF-8?q?foo?=',
            ),

        # XXX XXX comments may contain EWs, but the current code is buggy.
        # These will get decoded after the refactor is done.  We'll add some
        # some more test then, this is a target sample.

        ws_around_ew = C(
            '( =?utf-8?q?test?= )',
            #stringified='( test )',
            comments=[' =?utf-8?q?test?= '],
            #comments=[' test '],
            ),

        ew_in_nested_comment = C(
            '(foo (=?UTF-8?q?bar?=))',
            #stringified='(foo (bar))',
            comments=['foo (=?UTF-8?q?bar?=)'],
            #comments=['foo (bar)'],
            commenttree=['foo ', ['=?UTF-8?q?bar?=']],
            #commenttree=['foo ', ['bar']],
            ),

        ew_missing_whitespace = C(
            '(=?UTF-8?q?foo?==?UTF-8?q?bar?=)',
            #stringified='(foobar)',
            comments=['=?UTF-8?q?foo?==?UTF-8?q?bar?='],
            #comments=['foobar'],
            #defects=[
            #    missing_whitespace_after_ew_defect,
            #    missing_whitespace_before_ew_defect,
            #    ],
            ),

        )


    # get_cfws

    @params
    def test_get_cfws(self, s, *args, **kw):
        kw.setdefault('value', ' ')
        cfws = self._test_parse(parser.get_cfws, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(cfws, parser.CFWSList)
        self.assertEqual(cfws.token_type, 'cfws')
        self.verify_terminal_types(cfws, 'ptext', 'fws')

    # get_cfws should behave exactly the same as get_comment when parsing
    # values containing just a comment.
    @params_map(with_namelist=True)
    def adapt_comment_tests_for_cfws(nl, s, *args, **kw):
        # Our 'ctree' nested comment check returns a list of comments instead
        # of just the single nested comment it does for Comment.
        if 'commenttree' in kw:
            kw['commenttree'] = [kw['commenttree']]
        # XXX: get_cfws has the same bug that get_fws has: it does *not* raise
        # an error if there is no cfws, and it should.
        # XXX XXX Like get_fws, we'll deprecate this in the refactor.
        if nl.has_any('empty', 'non_wsp_before_left_paren_is_error'):
            kw.pop('exception')
            kw['remainder'] = s
        yield 'from_test_get_comment', C(s, *args, **kw)

    params_test_get_cfws = old_api_only(

        # get_cfws should behave exactly the same as get_fws when parsing
        # whitespace only strings, except for the case of ending at a '('
        # because cfws *doesn't* end there.
        include_unless(
            lambda n, *a, **k: 'left_parenthesis' in n,
            label="from_test_get_fws",
            )(params_test_get_fws),

        # get_cfws should behave exactly the same as get_comment when parsing
        # values containing just a comment.  Even the tests with remainders
        # should pass if the remainder doesn't start with whitespace.
        include_unless(
            lambda n, *a, remainder=..., **k:
                remainder is not ...
                    and remainder.startswith(tuple(RFC_WSP))
                or 'wsp_before_left_paren_is_error' in n
            )(adapt_comment_tests_for_cfws(params_test_get_comment)),

        mixed_comments_and_wsp = C(
            ' (foo )  ( bar) ',
            comments=['foo ', ' bar'],
            commenttree=[['foo '], [' bar']],
            ),

        **for_each_character(
                ALL_ASCII,
                # XXX XXX skip things split considers whitespace. This is buggy.
                #                             US  RS  GS  FS
                skip=CFWS_LEADER + '\r\n\v\f\x1f\x1e\x1d\x1c',
                )(
            ends_at_non_comment_non_ws = C(
                '(foo) {char}',
                remainder='{char}',
                comments=['foo'],
                commenttree=[['foo']],
                ),
            ),

        header_ends_in_comment = C(
            '  (foo ',
            stringified='  (foo )',
            defects=[end_inside_comment_defect],
            comments=['foo '],
            commenttree=[['foo ']],
            ),

        multiple_nested_comments = C(
            '(foo (bar)) ((a)(a))',
            comments=['foo (bar)', '(a)(a)'],
            commenttree=[['foo ', ['bar']], [['a'], ['a']]],
            ),

        ew_after_comment_no_ws = C(
            '  (bar)  (foo)=?UTF-8?q?foo?=',
            comments=['bar', 'foo'],
            remainder='=?UTF-8?q?foo?=',
            ),

        # XXX XXX these will get decoded after refactor is done.

        ew_in_nested_comment = C(
            ' (a) (foo (=?UTF-8?q?bar?=))',
            #stringified=' (a) (foo (bar))',
            comments=['a', 'foo (=?UTF-8?q?bar?=)'],
            #comments=['a', 'foo (bar)'],
            #commenttree=[('a', []), ('foo (bar)', [('bar', [])])],
            ),

        ew_missing_whitespace = C(
            '(=?UTF-8?q?foo?==?UTF-8?q?bar?=) (b)',
            #stringified='(foobar) (b)',
            comments=['=?UTF-8?q?foo?==?UTF-8?q?bar?=', 'b'],
            #comments=['foobar', 'b'],
            #defects=[
            #    missing_whitespace_after_ew_defect,
            #    missing_whitespace_before_ew_defect,
            #    ],
            ),

        nested_and_unnested_empty_comments = C(
            '()  (()) ( () ) ( ( ) )',
            comments=['', '()', ' () ', ' ( ) '],
            commenttree=[[''], [['']], [' ', [''], ' '], [' ', [' '], ' ']],
            ),

        )

    # get_quoted_string

    @params
    def test_get_quoted_string(
            self,
            s,
            *args,
            content=None,
            quoted_value=None,
            **kw,
            ):
        qs = self._test_parse(parser.get_quoted_string, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertEqual(qs.content, content)
        self.assertEqual(qs.quoted_value, quoted_value)
        self.assertIsInstance(qs, parser.QuotedString)
        self.assertEqual(qs.token_type, 'quoted-string')
        self.verify_terminal_types(qs, 'ptext', 'fws')


    # get_quoted_string should pass any get_bare_quoted_string test that
    # doesn't involve leading or trailing whitespace.
    @params_map
    def adapt_bare_quoted_string_tests_for_get_quoted_string(s, *args, **kw):
        r = kw.get('remainder', '')
        if s.startswith(tuple(RFC_WSP)) or r.startswith(tuple(RFC_WSP)):
            return
        if not 'exception' in kw:
            kw['quoted_value'] = kw.get('stringified', s[:-len(r)] if r else s)
            kw['content'] = kw['value']
        kw['quoted_value'] = kw.get('stringified', s[:-len(r)] if r else s)
        yield 'from_test_bare_quoted_string', C(s, *args, **kw)

    # If there is no remainder a cfws test string should be valid as a quoted
    # string prefix or suffix, with a few exceptions that test for what happens
    # if closing parens are missing.
    @params_map(with_namelist=True)
    def adapt_get_cfws_tests_for_get_quoted_string(
            nl,
            s,
            *args,
            stringified=None,
            remainder=None,
            **kw,
        ):
        if remainder or nl.has_any(
                'multiple_mesting_missing_two_right_parens',
                'no_right_paren_after_non_ws',
                'no_right_paren_after_ws',
                'header_ends_in_comment',
            ):
            return
        new_s = f'{s} "foo" {s}'
        if stringified:
            kw['stringified'] = f'{stringified} "foo" {stringified}'
        kw['value'] = ' foo '
        kw['quoted_value'] = ' "foo" '
        kw['content'] = 'foo'
        for k in ('comments', 'commenttree', 'defects'):
            if (v := kw.get(k)):
                kw[k] = v * 2
        yield 'adapted_from_get_cfws', C(new_s, **kw)

    params_test_get_quoted_string = old_api_only(

        adapt_bare_quoted_string_tests_for_get_quoted_string(
            params_test_get_bare_quoted_string,
            ),

        adapt_get_cfws_tests_for_get_quoted_string(params_test_get_cfws),

        with_wsp = C(
            '\t "bob"  ',
            value=' bob ',
            quoted_value=' "bob" ',
            content='bob',
            ),

        with_comments_and_wsp = C(
            ' (foo) "bob"(bar)',
            value=' bob ',
            quoted_value=' "bob" ',
            content='bob',
            comments=['foo', 'bar'],
            commenttree=[['foo'], ['bar']],
            ),

        with_multiple_comments = C(
            ' (foo) (bar) "bob"(bird)',
            value=' bob ',
            quoted_value=' "bob" ',
            content='bob',
            comments=['foo', 'bar', 'bird'],
            commenttree=[['foo'], ['bar'], ['bird']],
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable_in_comment = C(
                ' ({char}) "bob"',
                value=' bob',
                quoted_value=' "bob"',
                content='bob',
                defects=[(nonprintable_defect, '{char}')],
                comments=['{char}'],
                ),
            ),

        # all the non printables in qcontent are checked by the included
        # bare_quoted_string tests, this one proves that the defect is
        # correctly copied up even if there is also comment text involved.
        non_printable_in_qcontent = C(
            ' (a) "a\x0B"',
            value=' a\x0B',
            quoted_value=' "a\x0B"',
            content='a\x0B',
            defects=[nonprintable_defect('\x0b')],
            comments=['a'],
            ),

        internal_ws = C(
            ' (a) "foo  bar "',
            value=' foo  bar ',
            quoted_value=' "foo  bar "',
            content='foo  bar ',
            comments=['a'],
            ),

        header_ends_in_comment = C(
            ' (a) "bob" (a',
            stringified=' (a) "bob" (a)',
            value=' bob ',
            quoted_value=' "bob" ',
            content='bob',
            defects=[end_inside_comment_defect],
            comments=['a', 'a'],
            commenttree=[['a'], ['a']],
            ),

        header_ends_in_qcontent = C(
            ' (a) "bob',
            stringified=' (a) "bob"',
            value=' bob',
            quoted_value=' "bob"',
            content='bob',
            defects=[end_inside_quoted_string_defect],
            comments=['a'],
            ),

        cfws_only_raises = C(
            '(foo) ',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        no_quoted_string = C(
            '(ab) xyz',
            exception=(errors.HeaderParseError, '(?=.*expected.*")(?=.*xyz)'),
            ),

        **for_each_character(RFC_PRINTABLES, skip='(')(
            qs_ends_at_noncfws = C(
                '\t "bob" {char}',
                value=' bob ',
                quoted_value=' "bob" ',
                content='bob',
                remainder='{char}',
                ),
            ),

        ew_after_dquote = C(
            '"bob"=?UTF-8?q?foo?=',
            value='bob',
            quoted_value='"bob"',
            content='bob',
            remainder='=?UTF-8?q?foo?=',
            ),

        empty_quotes_between_comments = C(
            ' (a)  ""  (foo)',
            value='  ',
            quoted_value=' "" ',
            content='',
            comments=['a', 'foo'],
            ),

        empty_input = C(
            '',
            exception=(errors.HeaderParseError, r'(?i)expected'),
            ),

        )


    # get_atom

    @params
    def test_get_atom(self, s, *args, **kw):
        atom = self._test_parse(parser.get_atom, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(atom, parser.Atom)
        self.assertEqual(atom.token_type, 'atom')
        self.verify_terminal_types(atom, 'atext', 'vtext', 'ptext', 'fws')

    # If there is no remainder a cfws test string should be valid as a atom
    # prefix or suffix, with a few exceptions that test for what happens
    # if closing parens are missing.
    @params_map(with_namelist=True)
    def adapt_get_cfws_tests_for_get_atom(
            nl,
            s,
            *args,
            stringified=None,
            remainder=None,
            **kw,
        ):
        if remainder or nl.has_any(
                'multiple_mesting_missing_two_right_parens',
                'no_right_paren_after_non_ws',
                'no_right_paren_after_ws',
                'header_ends_in_comment',
            ):
            return
        new_s = f'{s} foo {s}'
        if stringified:
            kw['stringified'] = f'{stringified} foo {stringified}'
        kw['value'] = ' foo '
        for k in ('comments', 'commenttree', 'defects'):
            if (v := kw.get(k)):
                kw[k] = v * 2
        yield 'adapted_from_get_cfws', C(new_s, **kw)

    params_test_get_atom = old_api_only(

        adapt_get_cfws_tests_for_get_atom(params_test_get_cfws),

        # get_atom should pass all the get_atext tests except for those
        # involving leading or trailing whitespace.
        include_unless(
            lambda n, s, *a, remainder='', **k:
                s.startswith(tuple(CFWS_LEADER))
                or remainder.startswith(tuple(CFWS_LEADER)),
            label='from_test_get_atext',
            )(params_test_get_atext),

        with_wsp = C(
            '\t bob  ',
            value=' bob ',
            ),

        with_comments_and_wsp = C(
            ' (foo) bob(bar)',
            value=' bob ',
            comments=['foo', 'bar'],
            ),

        with_multiple_comments = C(
            ' (foo) (bar) bob(bird)',
            value=' bob ',
            comments=['foo', 'bar', 'bird'],
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable_in_comment = C(
                ' ({char}) bob',
                value=' bob',
                defects=[(nonprintable_defect, '{char}')],
                comments=['{char}'],
                ),

            non_printable_in_atext = C(
                ' (a) a{char}',
                value=' a{char}',
                defects=[(nonprintable_defect, '{char}')],
                comments=['a'],
                ),

            ),

        header_ends_in_comment = C(
            ' (a) bob (a',
            stringified=' (a) bob (a)',
            value=' bob ',
            defects=[end_inside_comment_defect],
            comments=['a', 'a'],
            ),

        no_atom = C(
            ' (ab) ',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        **for_each_character(RFC_SPECIALS, skip='(')(

            no_atom_before_special = C(
                ' (ab) {char}',
                exception=(
                    errors.HeaderParseError,
                    '(?i)(?=.*expected)(?=.*{echar})',
                    ),
                ),

            atom_ends_at_special = C(
                ' (foo) bob(bar)  {char}bang',
                value=' bob ',
                remainder='{char}bang',
                comments=['foo', 'bar'],
                ),

            ),

        **for_each_character(RFC_PRINTABLES, skip='(')(
            atom_ends_at_noncfws = C(
                'bob  {char}',
                value='bob ',
                remainder='{char}',
                ),
            ),

        ew_only = C(
            '=?utf-8?q?=20bob?=',
            stringified=' bob',
            ),

        ew_and_comments = C(
            '(a) =?UTF-8?q?bob?= (b)',
            stringified='(a) bob (b)',
            value=' bob ',
            comments=['a', 'b'],
            ),

        # XXX XXX this should actually be two missing whitespace defects.
        ew_and_comments_no_ws = C(
            '(a)=?UTF-8?q?bob?=(b)',
            stringified='(a)bob(b)',
            value=' bob ',
            comments=['a', 'b'],
            defects=[
                #missing_whitespace_before_ew_defect,
                missing_whitespace_after_ew_defect,
                ],
            ),

        # XXX XXX ditto
        ew_and_empty_comments_no_ws = C(
            '()=?UTF-8?q?bob?=()',
            stringified='()bob()',
            value=' bob ',
            comments=['', ''],
            defects=[
                #missing_whitespace_before_ew_defect,
                missing_whitespace_after_ew_defect,
                ],
            ),

        # XXX Ideally this should have a defect for the specials.
        **for_each_character(RFC_SPECIALS)(
            ew_with_unencoded_special = C(
                '=?UTF-8?q?bob{char}?= @foo',
                stringified='bob{char} ',
                remainder='@foo',
                ),
            ),

        ew_after_atom_no_ws = C(
            'foo@=?UTF-8?q?bob?=',
            value='foo',
            remainder='@=?UTF-8?q?bob?=',
            ),

        # XXX XXX Technically these are correct as is but we're going to fix it
        # to always decode the ews anyway, because most email software does.

        multiple_ew_no_ws = C(
            '=?UTF-8?q?foo?==?UTF-8?q?bar?=',
            stringified='foo',
            #stringified='foobar',
            remainder='=?UTF-8?q?bar?=',
            defects=[
                missing_whitespace_after_ew_defect,
                #missing_whitespace_before_ew_defect,
                ],
            ),

        ew_in_middle_of_atom_text = C(
            'foo{=?UTF-8?q?foo?=}{=?UTF-8?q?bar?=}bar',
            #stringified='foo{foo}{bar}bar',
            #defects=[
            #    missing_whitespace_before_ew_defect,
            #    missing_whitespace_after_ew_defect,
            #    missing_whitespace_before_ew_defect,
            #    missing_whitespace_after_ew_defect,
            #    ],
            ),

        empty_comments_no_ws = C(
            ' ()bob() ',
            value=' bob ',
            comments=['', ''],
            ),

        all_non_special_printables_are_allowed = C(
            f'{"".join(set(RFC_PRINTABLES) - set(RFC_SPECIALS))}@',
            remainder='@',
            ),

        )

    # get_dot_atom_text

    @params
    def test_get_dot_atom_text(self, s, *args, **kw):
        atom = self._test_parse(parser.get_dot_atom_text, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(atom, parser.DotAtomText)
        self.assertEqual(atom.token_type, 'dot-atom-text')
        self.verify_terminal_types(atom, 'dot', 'atext')

    params_test_get_dot_atom_text = old_api_only(

        # a bare atext is valid in a dot-atom, so we should pass all the
        # get_atext tests except the ones involving the dot.
        include_unless(
            lambda n, *a, **k:  'full_stop' in n,
            label='from_test_get_atext',
            )(params_test_get_atext),

        only = C(
            'foo.bar.bang',
            ),

        raises_on_leading_dot = C(
            '.foo.bar',
            exception=(errors.HeaderParseError, '.*'),
            ),

        raises_on_trailing_dot = C(
            'foo.bar.',
            exception=(errors.HeaderParseError, '.*'),
            ),

        **for_each_character(RFC_SPECIALS + RFC_WSP)(
            raises_on_leading_special_or_wsp = C(
                '{char}foo.bar',
                exception=(errors.HeaderParseError, r'expected.*{echar}foo\.'),
                ),
            ),

        **for_each_character(RFC_SPECIALS + RFC_WSP, skip='.')(
            ends_at_special_or_wsp = C(
                'foo.bird{char}bar',
                remainder='{char}bar',
                ),
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable_in_atext = C(
                'foo.{char}.bar',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        undecodable_characters = C(
            'foo.🎁.bar'.encode().decode('us-ascii', errors='surrogateescape'),
            defects=[undecodable_bytes_defect],
            ),

        all_atext_characters_allowed = C(
            RFC_ATEXT + '.' + RFC_ATEXT + '@foo',
            remainder = '@foo',
            ),

        raises_on_paired_dots = C(
            'foo..bar',
            exception=(
                errors.HeaderParseError,
                r'(?=.*expected)(?=.*atom)(?=.*\.\.bar)',
                ),
            ),

        )


    # get_dot_atom

    @params
    def test_get_dot_atom(self, s, *args, **kw):
        atom = self._test_parse(parser.get_dot_atom, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(atom, parser.DotAtom)
        self.assertEqual(atom.token_type, 'dot-atom')
        self.verify_terminal_types(atom, 'dot', 'atext', 'ptext', 'fws', 'vtext')

    params_test_get_dot_atom = old_api_only(

        # Atom is a subset of dot atom, so get_dot_atom should pass any
        # get_atom test except those involving the dot (full_stop).
        include_unless(
            lambda n, *a, **k:  'full_stop' in n,
            label='from_test_get_atom',
            )(params_test_get_atom),

        with_wsp = C(
            '\t  foo.bar.bing  ',
            value=' foo.bar.bing ',
            ),

        with_comments_and_wsp = C(
            ' (sing)  foo.bar.bing (here) ',
            value=' foo.bar.bing ',
            comments=['sing', 'here'],
            ),

        space_ends_dot_atom = C(
            ' (sing)  foo.bar .bing (here) ',
            value=' foo.bar ',
            remainder='.bing (here) ',
            comments=['sing'],
            ),

        no_atom_raises = C(
            ' (foo) ',
            exception=(errors.HeaderParseError, r'expected')
            ),

        **for_each_character(RFC_SPECIALS, skip='(')(
            leading_special_raises = C(
                ' (foo) {char}bar',
                exception=(errors.HeaderParseError, r'(?i)expected.*{echar}bar')
                ),
            ),

        two_dots_raises = C(
            'bar..bang',
            exception=(errors.HeaderParseError, r'expected.*\.\.bang')
            ),

        trailing_dot_raises = C(
            ' (foo) bar.bang. foo',
            exception=(errors.HeaderParseError, r'expected.*\. foo')
            ),

        rfc2047_atom = C(
            '=?utf-8?q?=20bob?=',
            stringified=' bob',
            ),

        **for_each_character(RFC_NONPRINTABLES, skip=RFC_WSP)(
            non_printable_in_atext = C(
                'foo.{char}.bar',
                defects=[(nonprintable_defect, '{char}')],
                ),
            ),

        undecodable_characters = C(
            'foo.🎁.bar'.encode().decode('us-ascii', errors='surrogateescape'),
            defects=[undecodable_bytes_defect],
            ),

        **for_each_character(RFC_SPECIALS, skip='.(')(
            ends_at_special = C(
                '(hey)foo.bar{char}.bird',
                value=' foo.bar',
                remainder='{char}.bird',
                comments=['hey'],
                ),
            ),

        **for_each_character(RFC_SPECIALS, skip='(')(
            ends_at_special_after_comment = C(
                '(hey)foo.bar(hey){char} bird',
                value=' foo.bar ',
                remainder='{char} bird',
                comments=['hey', 'hey'],
                ),
            ),

        two_ew_two_atoms = C(
            '(hey) =?UTF-8?q?foo?= =?UTF-8?q?bar?=',
            stringified='(hey) foo ',
            value=' foo ',
            remainder='=?UTF-8?q?bar?=',
            comments=['hey'],
            ),

        # XXX XXX These additional EW cases not already tested by the atom
        # tests will be fully decoded after refactoring.

        mixed_ews_and_atext = C(
            '(hey)foo.bar=?UTF-8?q?_foo?=bar.=?UTF-8?q?foo?=bar (hey)',
            #stringified='(hey)foo.bar foobar.foobar (hey)',
            value=' foo.bar=?UTF-8?q?_foo?=bar.=?UTF-8?q?foo?=bar ',
            #value=' foo.bar foobar.foobar ',
            #defects=[
            #    missing_whitespace_before_ew_defect,
            #    missing_whitespace_after_ew_defect,
            #    missing_whitespace_before_ew_defect,
            #    missing_whitespace_after_ew_defect,
            #    ],
            comments=['hey', 'hey'],
            ),

        two_ew_with_dot = C(
            '=?UTF-8?q?foo?=.=?UTF-8?q?bar?=(hey)',
            stringified='foo',
            #stringified='foo.bar(hey)',
            value='foo',
            #value='foo.bar ',
            remainder='.=?UTF-8?q?bar?=(hey)',
            defects=[
                missing_whitespace_after_ew_defect,
            #    missing_whitespace_before_ew_defect,
            #    missing_whitespace_after_ew_defect,
                ],
            #comments=['hey'],
            ),

        )


    # get_word

    @params
    def test_get_word(
            self,
            s,
            *args,
            quoted_value=None,
            content=None,
            tokenlisttype,
            **kw,
            ):
        word = self._test_parse(parser.get_word, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(word, tokenlisttype)
        if quoted_value is not None:
            self.assertEqual(word.quoted_value, quoted_value)
        if content is not None:
            self.assertEqual(word.content, content)
        self.verify_terminal_types(word, 'dot', 'atext', 'ptext', 'fws', 'vtext')

    @params_map
    def adapt_get_atom_tests_for_get_word(*args, **kw):
        kw['tokenlisttype'] = parser.TokenList
        yield '', C(*args, **kw)

    @params_map
    def adapt_get_quoted_string_tests_for_get_word(*args, **kw):
        kw['tokenlisttype'] = parser.QuotedString
        yield '', C(*args, **kw)

    params_test_get_word = old_api_only(

        # A word can be an atom, so get_word should pass many of the atom tests.
        adapt_get_atom_tests_for_get_word(
            include_unless(
                lambda n, *a, **k:
                    # For get_atom a leading quotation mark means there is no
                    # atom and is therefor an error, but get_word will treat it
                    # as a quoted_string.  Quoted strings are tested below.
                    n.has_any(
                        'no_atom_before_special',
                        'no_atext_before_special_or_wsp',
                        )
                    and 'quotation_mark' in n,
                label='from_test_get_atom',
                )(params_test_get_atom),
            ),

        # Or it can be a quoted string, so should pass most quoted_string tests.
        adapt_get_quoted_string_tests_for_get_word(
            include_unless(
                lambda n, *a, **k:
                    # These tests have an atom first; get_quoted_string raises
                    # for that, but get_word parses it. Atoms are tested above.
                    n.has_any(
                        'no_quoted_string',
                        'no_leading_dquote_before_non_ws',
                        ),
                label='from_test_get_quoted_string',
                )(params_test_get_quoted_string),
            ),

        )


    # get_phrase

    @params
    def test_get_phrase(self, s, *args, obs_dots=0, **kw):
        phrase = self._test_parse(parser.get_phrase, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.assertIsInstance(phrase, parser.Phrase)
        self.assertEqual(
            len([x for x in phrase if x.token_type == 'dot']),
            obs_dots,
            phrase.ppstr(),
            )
        self.verify_terminal_types(phrase, 'dot', 'atext', 'ptext', 'fws', 'vtext')

    @params_map(with_namelist=True)
    def adapt_get_word_tests_for_get_phrase(nl, *args, **kw):
        kw.pop('tokenlisttype')
        kw.pop('quoted_value', None)
        kw.pop('content', None)
        # XXX XXX A phrase has to have at least one word, but the current code
        # does not enforce this.  We'll fix this in the refactor, but for now
        # we skip the parameters that expect a raise on a value with no
        # content.
        if nl.has_any(
                'empty',
                'no_atom_before_special',
                'no_atom',
                'no_atext_before_special_or_wsp',
                'cfws_only_raises',
                'empty_input',
            ):
            return
        yield '', C(*args, **kw)

    params_test_get_phrase = old_api_only(

        # A phrase is a series of words, and single words are valid,
        # so get_phrase should pass many of the get_word tests.
        adapt_get_word_tests_for_get_phrase(
            include_unless(
                lambda n, *a, remainder=False, **k:
                    n.has_any(
                        # A phrase only ends at specials other than " and .
                        'atom_ends_at_noncfws',
                        'qs_ends_at_noncfws',
                        'ew_after_dquote',
                        'encoded_word_after_dquote_with_no_ws',
                        'end_dquote_mid_word',
                        # XXX XXX This test should pass after refactoring.
                        'multiple_ew_no_ws',
                        )
                    # A phrase does *not* end at a period or a quotation mark.
                    or remainder and n.has_any(
                        'full_stop',
                        'quotation_mark',
                        ),
                label='from_test_get_word',
                )(params_test_get_word),
            ),

        simple_phrase = C(
            '"Fred A. Johnson" is his name, oh.',
            value='Fred A. Johnson is his name',
            remainder=', oh.',
            ),

        complex_phrase = C(
            ' (A) bird (in (my|your)) "hand  " is messy\t<>\t',
            value=' bird hand   is messy ',
            remainder='<>\t',
            comments=['A', 'in (my|your)'],
            ),

        obsolete_phrase = C(
            'Fred A.(weird).O Johnson',
            value='Fred A. .O Johnson',
            defects=[
                *[period_in_phrase_obs_defect]*2,
                comment_without_atom_in_phrase_obs_defect,
                ],
            comments=['weird'],
            obs_dots=2,
            ),

        should_start_with_word = C(
            '(even weirder).name',
            value=' .name',
            defects=[
                non_word_phrase_start_defect,
                comment_without_atom_in_phrase_obs_defect,
                period_in_phrase_obs_defect,
                ],
            comments=['even weirder'],
            obs_dots=1,
            ),

        obsolete_ending = C(
            'simple phrase.(with trailing comment):boo',
            value='simple phrase. ',
            defects=[
                period_in_phrase_obs_defect,
                comment_without_atom_in_phrase_obs_defect,
                ],
            remainder=':boo',
            comments=['with trailing comment'],
            obs_dots=1,
            ),

        # "'linear-white-space' that separates a pair of adjacent
        # 'encoded-word's is ignored" (rfc2047 section 6.2)

        adjacent_ew = C(
            '=?ascii?q?Joi?= \t =?ascii?q?ned?=',
            stringified='Joined',
            ),

        adjacent_ew_different_encodings = C(
            '=?utf-8?q?B=C3=A9r?= =?iso-8859-1?q?=E9nice?=',
            stringified='Bérénice',
            ),

        adjacent_ew_encoded_spaces = C(
            '=?ascii?q?Encoded?= =?ascii?q?_spaces_?= =?ascii?q?preserved?=',
            stringified='Encoded spaces preserved',
            ),

        adjacent_ew_comment_is_not_linear_white_space = C(
            '=?ascii?q?Comment?= (is not) =?ascii?q?linear-white-space?=',
            stringified='Comment (is not) linear-white-space',
            value='Comment linear-white-space',
            comments=['is not'],
            ),

        adjacent_ew_no_error_on_defects = C(
            '=?ascii?q?Def?= =?ascii?q?ect still joins?=',
            stringified='Defect still joins',
            defects=[whitespace_inside_ew_defect],
            ),

        adjacent_ew_ignore_non_ew = C(
            '=?ascii?q?No?= =?join?= for non-ew',
            stringified='No =?join?= for non-ew',
            ),

        adjacent_ew_ignore_invalid_ew = C(
            '=?ascii?q?No?= =?ascii?rot13?wbva= for invalid ew',
            stringified='No =?ascii?rot13?wbva= for invalid ew',
            ),

        adjacent_ew_missing_space = C(
            '=?ascii?q?Joi?==?ascii?q?ned?=',
            stringified='Joined',
            defects=[missing_whitespace_after_ew_defect],
            ),

        ew_before_quoted_string_missing_space = C(
            '=?ascii?q?disjoin?="=?ascii?q?ted?="',
            stringified='disjoin"ted"',
            value='disjointed',
            defects=[
                # XXX XXX After refactoring there should be one 'after' defect
                missing_whitespace_after_ew_defect,
                missing_whitespace_after_ew_defect,
                ew_inside_quoted_string_defect,
                ],
            ),

        ew_after_quoted_string_missing_space = C(
            '"=?ascii?q?disjoin?="=?ascii?q?ted?=',
            stringified='"disjoin"ted',
            value='disjointed',
            defects=[
                # XXX XXX After refactoring 'after' should become 'before'
                missing_whitespace_after_ew_defect,
                ew_inside_quoted_string_defect,
                ],
            ),

        **for_each_character(RFC_SPECIALS, skip=CFWS_LEADER + '."')(
            ends_at_special = C(
                'complex (obsolete). "phrase" {char}foo',
                value='complex . phrase ',
                defects=[period_in_phrase_obs_defect],
                remainder='{char}foo',
                comments=['obsolete'],
                obs_dots=1,
                ),
            ),

        # While these violate the RFC in several ways, allowing the '.'
        # as the value of the phrase is the only sensible recovery.

        obsolete_dot_only = C(
            '.',
            defects=[
                non_word_phrase_start_defect,
                period_in_phrase_obs_defect,
                ],
            obs_dots=1,
            ),

        obsolete_dot_with_wsp = C(
            '\t .  ',
            value=' . ',
            defects=[
                non_word_phrase_start_defect,
                *[comment_without_atom_in_phrase_obs_defect]*2,
                period_in_phrase_obs_defect,
                ],
            obs_dots=1,
            ),

        obsolete_dot_and_comments_only = C(
            '(foo).(bar)',
            value=' . ',
            comments=['foo', 'bar'],
            defects=[
                non_word_phrase_start_defect,
                *[comment_without_atom_in_phrase_obs_defect]*2,
                period_in_phrase_obs_defect,
                ],
            obs_dots=1,
            ),

        obsolete_dot_and_comments_with_fws = C(
            ' (foo).  (bar) ',
            value=' . ',
            comments=['foo', 'bar'],
            defects=[
                non_word_phrase_start_defect,
                *[comment_without_atom_in_phrase_obs_defect]*2,
                period_in_phrase_obs_defect,
                ],
            obs_dots=1,
            ),

       )


    # get_obs_local_part

    @params
    def test_get_obs_local_part(self, s, *args, local_part=None, **kw):
        lp = self._test_parse(parser.get_obs_local_part, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.verify_terminal_types(
            lp,
            'dot',
            'atext',
            'ptext',
            'fws',
            'vtext',
            'misplaced-special',
            )

    # This function should only get called when the non-obs expressions have
    # already been checked for, so we are only testing the obs syntax handling,
    # not what it does with non-obs syntax.  Anything else is "don't care".
    # The 'local_part' specs are checked by the get_local_part tests, since the
    # token list returned by get_obs_local_part doesn't have that attribute.
    params_test_get_obs_local_part = old_api_only(

        simple_obsolete = C(
            'Fred. A.Johnson@python.org',
            remainder='@python.org',
            local_part='Fred.A.Johnson',
            ),

        complex_obsolete_1 = C(
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson."and  dogs "@python.org',
            value=' Fred . A. Johnson.and  dogs ',
            remainder='@python.org',
            comments=['foo ', 'bar', 'bird', 'sheep'],
            local_part='Fred.A.Johnson.and  dogs ',
            ),

        complex_obsolete_invalid = C(
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson "and  dogs"@python.org',
            value=' Fred . A. Johnson and  dogs',
            defects=[missing_dot_in_local_part_defect],
            remainder='@python.org',
            comments=['foo ', 'bar', 'bird', 'sheep'],
            local_part='Fred.A.Johnson and  dogs',
            ),

        trailing_dot = C(
            ' borris.@python.org',
            defects=[trailing_dot_in_local_part_defect],
            remainder='@python.org',
            local_part='borris.',
            ),

        trailing_dot_with_ws = C(
            ' borris. @python.org',
            defects=[trailing_dot_in_local_part_defect],
            remainder='@python.org',
            local_part='borris.',
            ),

        leading_dot = C(
            '.borris@python.org',
            defects=[leading_dot_in_local_part_defect],
            remainder='@python.org',
            local_part='.borris',
            ),

        leading_dot_after_ws = C(
            ' .borris@python.org',
            defects=[leading_dot_in_local_part_defect],
            remainder='@python.org',
            local_part='.borris',
            ),

        dots_around_comment = C(
            ' borris.(foo).natasha@python.org',
            value=' borris. .natasha',
            defects=[repeated_dot_in_local_part_defect],
            remainder='@python.org',
            comments=['foo'],
            local_part='borris..natasha',
            ),

        quoted_strings_in_atom_list = C(
            '""example" example"@example.com',
            value='example example',
            defects=[*[missing_dot_in_local_part_defect]*2],
            remainder='@example.com',
            local_part="example example",
            ),

        # This is intentionally a weird one: first there is a quoted string
        # consisting of a single quoted pair resolving to a single backslash.
        # Then there is unquoted atext and an invalid quoted pair that
        # therefore gets interpreted as two backslashes.  Then there is a
        # quoted string containing 'example' with a leading space.
        valid_and_invalid_qp_in_atom_list = C(
            r'"\\"example\\" example"@example.com',
            value=r'\example\\ example',
            defects=[
                *[missing_dot_in_local_part_defect]*2,
                *[misplaced_backslash_defect]*2,
                ],
            remainder='@example.com',
            local_part=r'\example\\ example',
            ),

        # We do want to check that it raises on an empty input, even
        # though it should never be called with one.
        empty = C(
            '',
            exception=(errors.HeaderParseError, '(?i)expected'),
            ),

        quoted_words_but_no_ws = C(
            '"words"."separated".by.dots',
            value='words.separated.by.dots',
            local_part='words.separated.by.dots',
            ),

        backlashes_in_various_places = C(
            r"\invali\d\.\really" + '\\',
            local_part=r'\invali\d\.\really' + '\\',
            defects=[
                *[misplaced_backslash_defect]*5,
                *[missing_dot_in_local_part_defect]*3,
                ],
            ),

        double_dot_no_ws = C(
            ' borris..natasha@python.org',
            value=' borris..natasha',
            defects=[repeated_dot_in_local_part_defect],
            remainder='@python.org',
            local_part='borris..natasha',
            ),

        # The end of this is treated as a quoted string, so the stringified
        # version has a trailing quote added, but the local_part attribute
        # does not include the quotes.
        looks_like_qp_quote_but_quote_is_respected = C(
            r'invalid.\"for.sure',
            stringified=r'invalid.\"for.sure"',
            value=r'invalid.\for.sure',
            local_part=r'invalid.\for.sure',
            defects=[
                end_inside_quoted_string_defect,
                misplaced_backslash_defect,
                missing_dot_in_local_part_defect,
                ],
            ),

        # obs_local_part parses anything that can be in a phrase (cfws
        # atoms and quoted strings), plus \ and dots.
        **for_each_character(RFC_SPECIALS, skip=CFWS_LEADER + r'\."')(
            ends_at_phrase_ends = C(
                'doted.words. and . space{char}',
                local_part='doted.words.and.space',
                remainder='{char}',
                ),
            ),

        # Encoded words are not legitimate in local-part, but we decode
        # them anyway.

        invalid_ew_atoms = C(
            '=?utf-8?q?foo_?="=?utf-8?q?_bar?=".bird',
            # It's not clear this str is the best choice.  It's
            # a consequence of the underlying parsed structures.
            stringified='foo " bar".bird',
            value="foo  bar.bird",
            local_part="foo  bar.bird",
            defects=[
                # XXX XXX There should be exactly one ew whitespace defect
                # here, but the number generated will change during refactor,
                # until it is fixed when get_obs_local_part is refactored.
                *[missing_whitespace_after_ew_defect]*2,
                missing_dot_in_local_part_defect,
                ew_inside_quoted_string_defect,
                ],
            ),

        less_invalid_ew_atoms = C(
            '=?utf-8?q?foo_?= . (=?utf-8?q?test?=) =?utf-8?q?_bar?= .bird',
            # XXX XXX after refactoring the comment ew will also be decoded.
            #stringified='foo  . (test)  bar .bird',
            stringified='foo  . (=?utf-8?q?test?=)  bar .bird',
            value="foo  .  bar .bird",
            local_part="foo . bar.bird",
            # XXX XXX after refactoring the comment ew will also be decoded.
            # comments=['test']
            comments=['=?utf-8?q?test?='],
            ),

        # XXX XXX Since we've decided to decode encoded words, this becomes a
        # "valid" dot-atom, which it will be treated as after the refactoring.
        # But if you clear up the whitespace defects by adding whitespace, it
        # turns into an obs_local_part because of the whitespace.
        sort_of_valid_ew_dot_atom = C(
            '=?utf-8?q?foo_?=.=?utf-8?q?_bar?=.bird',
            stringified='foo . bar.bird',
            value="foo . bar.bird",
            local_part="foo . bar.bird",
            defects=[
                # XXX XXX the whitespace defects will change during refactoring
                missing_whitespace_after_ew_defect,
                missing_whitespace_after_ew_defect,
                ],
            ),

        )


    # get_local_part

    @params
    def test_get_local_part(self, s, *args, local_part=None, **kw):
        lp = self._test_parse(parser.get_local_part, C(s), *args, **kw)
        if 'exception' in kw:
            return
        self.verify_terminal_types(
            lp,
            'dot',
            'atext',
            'ptext',
            'fws',
            'vtext',
            'misplaced-special',
            )
        self.assertEqual(lp.local_part, local_part)

    @params_map(with_namelist=True)
    def adapt_get_dot_atom_tests_for_get_local_part(nl, s, *args, **kw):
        r = kw.get('remainder')
        if 'value' in kw:
            local_part = kw['value']
        else:
            local_part = kw.get('stringified', s[:-len(r)] if r else s)
        if not nl.has_any('ew_only', 'rfc2047_atom'):
            # Except for the above two tests, the leading and trailing
            # whitespace in the 'value' is the 'semantic blank' it produces
            # for leading and trailing cfws, which local_part doesn't include.
            # For those two ew tests the blank comes from inside the ew.
            local_part = local_part.removeprefix(' ').removesuffix(' ')
        kw['local_part'] = local_part
        yield '', C(s, *args, **kw)

    @params_map
    def adapt_get_quoted_string_tests_for_get_local_part(*args, **kw):
        if 'quoted_value' in kw:
            kw['value'] = kw.pop('quoted_value')
        if 'exception' not in kw:
            kw['local_part'] = kw.pop('content')
        yield '', C(*args, **kw)

    @params_map
    def adapt_get_obs_local_part_tests_for_get_local_part(
            *args,
            defects=[],
            **kw,
        ):
        defects = list(defects)
        if any(
            x in (
                repeated_dot_in_local_part_defect,
                misplaced_backslash_defect,
                missing_dot_in_local_part_defect,
                leading_dot_in_local_part_defect,
                trailing_dot_in_local_part_defect,
                ) for x in defects
            ):
            defects.append(not_even_obs_local_part_defect)
        else:
            defects.append(non_dot_atom_local_part_obs_defect)
        yield '', C(*args, defects=defects, **kw)

    params_test_get_local_part = old_api_only(

        # An RFC compliant local part can be a dot atom or a quoted string, so
        # it should pass some of the tests for those.

        adapt_get_dot_atom_tests_for_get_local_part(
            include_unless(
                lambda n, *a, **k:
                    n.has_any(
                        # Get local part handles multiple atoms.
                        'two_ew_two_atoms',
                        'atom_ends_at_noncfws',
                        # There are some things get_dot_atom raises for that
                        # get_local_part treats as obs-local-part.
                        'two_dots_raises',
                        'trailing_dot_raises',
                        'space_ends_dot_atom',
                        # XXX XXX These tests should pass after the refactoring
                        # of get_dot_atom.
                        'two_ew_with_dot',
                        'multiple_ew_no_ws',
                        )
                    or
                        # get_local_part handles quoted strings (tested above),
                        # and leading dots or \ are handled as obs-local-part.
                        n.has_any(
                            'up_to_special',
                            'leading_special_raises',
                            'no_atom_before_special',
                            'no_atext_before_special_or_wsp',
                            'atom_ends_at_special',
                            'ends_at_special_after_comment',
                            'ends_at_special',
                            )
                        and n.has_any(
                            'reverse_solidus',
                            'quotation_mark',
                            'full_stop',
                            ),
                label='from_test_get_dot_atom',
                )(params_test_get_dot_atom),
            ),

        adapt_get_quoted_string_tests_for_get_local_part(
            include_unless(
                lambda n, *a, **k: n.has_any(
                    # These tests have an atom first; get_quoted_string raises,
                    # but get_local_part parses the atom.  Atoms are tested above.
                    'no_quoted_string',
                    'no_leading_dquote_before_non_ws',
                    # A local part only ends at specials other than " and .
                    'qs_ends_at_noncfws',
                    'ew_after_dquote',
                    'encoded_word_after_dquote_with_no_ws',
                    'end_dquote_mid_word',
                    ),
                label='from_test_get_quoted_string',
                )(params_test_get_quoted_string),
            ),

        add_label('from_test_get_obs_local_part')(
            adapt_get_obs_local_part_tests_for_get_local_part(
                params_test_get_obs_local_part,
                ),
            ),

        simple = C(
            'dinsdale@python.org',
            remainder='@python.org',
            local_part='dinsdale',
            ),

        with_dot = C(
            'Fred.A.Johnson@python.org',
            remainder='@python.org',
            local_part='Fred.A.Johnson',
            ),

        with_whitespace = C(
            ' Fred.A.Johnson  @python.org',
            value=' Fred.A.Johnson ',
            remainder='@python.org',
            local_part='Fred.A.Johnson',
            ),

        with_cfws = C(
            ' (foo) Fred.A.Johnson (bar (bird))  @python.org',
            value=' Fred.A.Johnson ',
            remainder='@python.org',
            comments=['foo', 'bar (bird)'],
            local_part='Fred.A.Johnson',
            ),

        simple_quoted = C(
            '"dinsdale"@python.org',
            remainder='@python.org',
            local_part='dinsdale',
            ),

        with_quoted_dot = C(
            '"Fred.A.Johnson"@python.org',
            remainder='@python.org',
            local_part='Fred.A.Johnson',
            ),

        quoted_with_whitespace = C(
            ' "Fred A. Johnson"  @python.org',
            value=' "Fred A. Johnson" ',
            remainder='@python.org',
            local_part='Fred A. Johnson',
            ),

        quoted_with_cfws = C(
            ' (foo) " Fred A. Johnson " (bar (bird))  @python.org',
            value=' " Fred A. Johnson " ',
            remainder='@python.org',
            comments=['foo', 'bar (bird)'],
            local_part=' Fred A. Johnson ',
            ),

        empty_raises = C(
            '',
            exception=(errors.HeaderParseError, '.*'),
            ),

        no_part_raises = C(
            ' (foo) ',
            exception=(errors.HeaderParseError, '.*'),
            ),

        special_instead_raises = C(
            ' (foo) @python.org',
            exception=(errors.HeaderParseError, '.*'),
            ),

        unicode = C(
            'exámple@example.com',
            remainder='@example.com',
            local_part='exámple',
            ),

        ew_non_ascii = C(
            '=?utf-8?q?ex=c3=a1mple?=@example.com',
            stringified='exámple',
            remainder='@example.com',
            defects=[
                # XXX XXX there should be exactly one missing whitespace here,
                # but it will change until we refactor get_local_part.
                missing_whitespace_after_ew_defect,
                # XXX XXX there should be a defect for there being an EW at all.
                ],
            local_part='exámple',
            ),

        )


    # get_dtext

    def test_get_dtext_only(self):
        dtext = self._test_get_x(parser.get_dtext,
                                'foobar', 'foobar', 'foobar', [], '')
        self.assertEqual(dtext.token_type, 'ptext')

    def test_get_dtext_all_dtext(self):
        dtext = self._test_get_x(parser.get_dtext, self.rfc_dtext_chars,
                                 self.rfc_dtext_chars,
                                 self.rfc_dtext_chars, [], '')

    def test_get_dtext_two_words_gets_first(self):
        self._test_get_x(parser.get_dtext,
                        'foo bar', 'foo', 'foo', [], ' bar')

    def test_get_dtext_following_wsp_preserved(self):
        self._test_get_x(parser.get_dtext,
                        'foo \t\tbar', 'foo', 'foo', [], ' \t\tbar')

    def test_get_dtext_non_printables(self):
        dtext = self._test_get_x(parser.get_dtext,
                                'foo\x00bar]', 'foo\x00bar', 'foo\x00bar',
                                [errors.NonPrintableDefect], ']')
        self.assertEqual(dtext.defects[0].non_printables[0], '\x00')

    def test_get_dtext_with_qp(self):
        ptext = self._test_get_x(parser.get_dtext,
                                 r'foo\]\[\\bar\b\e\l\l',
                                 r'foo][\barbell',
                                 r'foo][\barbell',
                                 [errors.ObsoleteHeaderDefect],
                                 '')

    def test_get_dtext_up_to_close_bracket_only(self):
        self._test_get_x(parser.get_dtext,
                        'foo]', 'foo', 'foo', [], ']')

    def test_get_dtext_wsp_before_close_bracket_preserved(self):
        self._test_get_x(parser.get_dtext,
                        'foo  ]', 'foo', 'foo', [], '  ]')

    def test_get_dtext_close_bracket_mid_word(self):
        self._test_get_x(parser.get_dtext,
                        'foo]bar', 'foo', 'foo', [], ']bar')

    def test_get_dtext_up_to_open_bracket_only(self):
        self._test_get_x(parser.get_dtext,
                        'foo[', 'foo', 'foo', [], '[')

    def test_get_dtext_wsp_before_open_bracket_preserved(self):
        self._test_get_x(parser.get_dtext,
                        'foo  [', 'foo', 'foo', [], '  [')

    def test_get_dtext_open_bracket_mid_word(self):
        self._test_get_x(parser.get_dtext,
                        'foo[bar', 'foo', 'foo', [], '[bar')

    def test_get_dtext_open_bracket_only(self):
        self._test_get_x(parser.get_dtext,
                        '[', '', '', [], '[')

    def test_get_dtext_close_bracket_only(self):
        self._test_get_x(parser.get_dtext,
                        ']', '', '', [], ']')

    def test_get_dtext_empty(self):
        self._test_get_x(parser.get_dtext,
                        '', '', '', [], '')

    # get_domain_literal

    def test_get_domain_literal_only(self):
        domain_literal = domain_literal = self._test_get_x(parser.get_domain_literal,
                                '[127.0.0.1]',
                                '[127.0.0.1]',
                                '[127.0.0.1]',
                                [],
                                '')
        self.assertEqual(domain_literal.token_type, 'domain-literal')
        self.assertEqual(domain_literal.domain, '[127.0.0.1]')
        self.assertEqual(domain_literal.ip, '127.0.0.1')

    def test_get_domain_literal_with_internal_ws(self):
        domain_literal = self._test_get_x(parser.get_domain_literal,
                                '[  127.0.0.1\t ]',
                                '[  127.0.0.1\t ]',
                                '[ 127.0.0.1 ]',
                                [],
                                '')
        self.assertEqual(domain_literal.domain, '[127.0.0.1]')
        self.assertEqual(domain_literal.ip, '127.0.0.1')

    def test_get_domain_literal_with_surrounding_cfws(self):
        domain_literal = self._test_get_x(parser.get_domain_literal,
                                '(foo)[  127.0.0.1] (bar)',
                                '(foo)[  127.0.0.1] (bar)',
                                ' [ 127.0.0.1] ',
                                [],
                                '')
        self.assertEqual(domain_literal.domain, '[127.0.0.1]')
        self.assertEqual(domain_literal.ip, '127.0.0.1')

    def test_get_domain_literal_no_start_char_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain_literal('(foo) ')

    def test_get_domain_literal_no_start_char_before_special_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain_literal('(foo) @')

    def test_get_domain_literal_bad_dtext_char_before_special_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain_literal('(foo) [abc[@')

    # get_domain

    def test_get_domain_regular_domain_only(self):
        domain = self._test_get_x(parser.get_domain,
                                  'example.com',
                                  'example.com',
                                  'example.com',
                                  [],
                                  '')
        self.assertEqual(domain.token_type, 'domain')
        self.assertEqual(domain.domain, 'example.com')

    def test_get_domain_domain_literal_only(self):
        domain = self._test_get_x(parser.get_domain,
                                  '[127.0.0.1]',
                                  '[127.0.0.1]',
                                  '[127.0.0.1]',
                                  [],
                                  '')
        self.assertEqual(domain.token_type, 'domain')
        self.assertEqual(domain.domain, '[127.0.0.1]')

    def test_get_domain_with_cfws(self):
        domain = self._test_get_x(parser.get_domain,
                                  '(foo) example.com(bar)\t',
                                  '(foo) example.com(bar)\t',
                                  ' example.com ',
                                  [],
                                  '')
        self.assertEqual(domain.domain, 'example.com')

    def test_get_domain_domain_literal_with_cfws(self):
        domain = self._test_get_x(parser.get_domain,
                                  '(foo)[127.0.0.1]\t(bar)',
                                  '(foo)[127.0.0.1]\t(bar)',
                                  ' [127.0.0.1] ',
                                  [],
                                  '')
        self.assertEqual(domain.domain, '[127.0.0.1]')

    def test_get_domain_domain_with_cfws_ends_at_special(self):
        domain = self._test_get_x(parser.get_domain,
                                  '(foo)example.com\t(bar), next',
                                  '(foo)example.com\t(bar)',
                                  ' example.com ',
                                  [],
                                  ', next')
        self.assertEqual(domain.domain, 'example.com')

    def test_get_domain_domain_literal_with_cfws_ends_at_special(self):
        domain = self._test_get_x(parser.get_domain,
                                  '(foo)[127.0.0.1]\t(bar), next',
                                  '(foo)[127.0.0.1]\t(bar)',
                                  ' [127.0.0.1] ',
                                  [],
                                  ', next')
        self.assertEqual(domain.domain, '[127.0.0.1]')

    def test_get_domain_obsolete(self):
        domain = self._test_get_x(parser.get_domain,
                                  '(foo) example . (bird)com(bar)\t',
                                  '(foo) example . (bird)com(bar)\t',
                                  ' example . com ',
                                  [errors.ObsoleteHeaderDefect],
                                  '')
        self.assertEqual(domain.domain, 'example.com')

    def test_get_domain_empty_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain("")

    def test_get_domain_no_non_cfws_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain("  (foo)\t")

    def test_get_domain_no_atom_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_domain("  (foo)\t, broken")


    # get_addr_spec

    def test_get_addr_spec_normal(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
                                    'dinsdale@example.com',
                                    'dinsdale@example.com',
                                    'dinsdale@example.com',
                                    [],
                                    '')
        self.assertEqual(addr_spec.token_type, 'addr-spec')
        self.assertEqual(addr_spec.local_part, 'dinsdale')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, 'dinsdale@example.com')

    def test_get_addr_spec_with_doamin_literal(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
                                    'dinsdale@[127.0.0.1]',
                                    'dinsdale@[127.0.0.1]',
                                    'dinsdale@[127.0.0.1]',
                                    [],
                                    '')
        self.assertEqual(addr_spec.local_part, 'dinsdale')
        self.assertEqual(addr_spec.domain, '[127.0.0.1]')
        self.assertEqual(addr_spec.addr_spec, 'dinsdale@[127.0.0.1]')

    def test_get_addr_spec_with_cfws(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
                '(foo) dinsdale(bar)@ (bird) example.com (bog)',
                '(foo) dinsdale(bar)@ (bird) example.com (bog)',
                ' dinsdale@example.com ',
                [],
                '')
        self.assertEqual(addr_spec.local_part, 'dinsdale')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, 'dinsdale@example.com')

    def test_get_addr_spec_with_qouoted_string_and_cfws(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
                '(foo) "roy a bug"(bar)@ (bird) example.com (bog)',
                '(foo) "roy a bug"(bar)@ (bird) example.com (bog)',
                ' "roy a bug"@example.com ',
                [],
                '')
        self.assertEqual(addr_spec.local_part, 'roy a bug')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, '"roy a bug"@example.com')

    def test_get_addr_spec_ends_at_special(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
                '(foo) "roy a bug"(bar)@ (bird) example.com (bog) , next',
                '(foo) "roy a bug"(bar)@ (bird) example.com (bog) ',
                ' "roy a bug"@example.com ',
                [],
                ', next')
        self.assertEqual(addr_spec.local_part, 'roy a bug')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, '"roy a bug"@example.com')

    def test_get_addr_spec_quoted_strings_in_atom_list(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
            '""example" example"@example.com',
            '""example" example"@example.com',
            'example example@example.com',
            [errors.InvalidHeaderDefect]*3,
            '')
        self.assertEqual(addr_spec.local_part, 'example example')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, '"example example"@example.com')

    def test_get_addr_spec_dot_atom(self):
        addr_spec = self._test_get_x(parser.get_addr_spec,
            'star.a.star@example.com',
            'star.a.star@example.com',
            'star.a.star@example.com',
            [],
            '')
        self.assertEqual(addr_spec.local_part, 'star.a.star')
        self.assertEqual(addr_spec.domain, 'example.com')
        self.assertEqual(addr_spec.addr_spec, 'star.a.star@example.com')

    def test_get_addr_spec_multiple_domains(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_addr_spec('star@a.star@example.com')

        with self.assertRaises(errors.HeaderParseError):
            parser.get_addr_spec('star@a@example.com')

        with self.assertRaises(errors.HeaderParseError):
            parser.get_addr_spec('star@172.17.0.1@example.com')

    # get_obs_route

    def test_get_obs_route_simple(self):
        obs_route = self._test_get_x(parser.get_obs_route,
            '@example.com, @two.example.com:',
            '@example.com, @two.example.com:',
            '@example.com, @two.example.com:',
            [],
            '')
        self.assertEqual(obs_route.token_type, 'obs-route')
        self.assertEqual(obs_route.domains, ['example.com', 'two.example.com'])

    def test_get_obs_route_complex(self):
        obs_route = self._test_get_x(parser.get_obs_route,
            '(foo),, (blue)@example.com (bar),@two.(foo) example.com (bird):',
            '(foo),, (blue)@example.com (bar),@two.(foo) example.com (bird):',
            ' ,, @example.com ,@two. example.com :',
            [errors.ObsoleteHeaderDefect],  # This is the obs-domain
            '')
        self.assertEqual(obs_route.token_type, 'obs-route')
        self.assertEqual(obs_route.domains, ['example.com', 'two.example.com'])

    def test_get_obs_route_no_route_before_end_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('(foo) @example.com,')

    def test_get_obs_route_no_route_before_end_raises2(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('(foo) @example.com, (foo) ')

    def test_get_obs_route_no_route_before_special_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('(foo) [abc],')

    def test_get_obs_route_no_route_before_special_raises2(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('(foo) @example.com [abc],')

    def test_get_obs_route_no_domain_after_at_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('@')

    def test_get_obs_route_no_domain_after_at_raises2(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_obs_route('@example.com, @')

    # get_angle_addr

    def test_get_angle_addr_simple(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            [],
            '')
        self.assertEqual(angle_addr.token_type, 'angle-addr')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_empty(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<>',
            '<>',
            '<>',
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(angle_addr.token_type, 'angle-addr')
        self.assertIsNone(angle_addr.local_part)
        self.assertIsNone(angle_addr.domain)
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, '<>')

    def test_get_angle_addr_qs_only_quotes(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<""@example.com>',
            '<""@example.com>',
            '<""@example.com>',
            [],
            '')
        self.assertEqual(angle_addr.token_type, 'angle-addr')
        self.assertEqual(angle_addr.local_part, '')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, '""@example.com')

    def test_get_angle_addr_with_cfws(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            ' (foo) <dinsdale@example.com>(bar)',
            ' (foo) <dinsdale@example.com>(bar)',
            ' <dinsdale@example.com> ',
            [],
            '')
        self.assertEqual(angle_addr.token_type, 'angle-addr')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_qs_and_domain_literal(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<"Fred Perfect"@[127.0.0.1]>',
            '<"Fred Perfect"@[127.0.0.1]>',
            '<"Fred Perfect"@[127.0.0.1]>',
            [],
            '')
        self.assertEqual(angle_addr.local_part, 'Fred Perfect')
        self.assertEqual(angle_addr.domain, '[127.0.0.1]')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, '"Fred Perfect"@[127.0.0.1]')

    def test_get_angle_addr_internal_cfws(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<(foo) dinsdale@example.com(bar)>',
            '<(foo) dinsdale@example.com(bar)>',
            '< dinsdale@example.com >',
            [],
            '')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_obs_route(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '(foo)<@example.com, (bird) @two.example.com: dinsdale@example.com> (bar) ',
            '(foo)<@example.com, (bird) @two.example.com: dinsdale@example.com> (bar) ',
            ' <@example.com, @two.example.com: dinsdale@example.com> ',
            [errors.ObsoleteHeaderDefect],
            '')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertEqual(angle_addr.route, ['example.com', 'two.example.com'])
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_missing_closing_angle(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<dinsdale@example.com',
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_missing_closing_angle_with_cfws(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<dinsdale@example.com (foo)',
            '<dinsdale@example.com (foo)>',
            '<dinsdale@example.com >',
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_ends_at_special(self):
        angle_addr = self._test_get_x(parser.get_angle_addr,
            '<dinsdale@example.com> (foo), next',
            '<dinsdale@example.com> (foo)',
            '<dinsdale@example.com> ',
            [],
            ', next')
        self.assertEqual(angle_addr.local_part, 'dinsdale')
        self.assertEqual(angle_addr.domain, 'example.com')
        self.assertIsNone(angle_addr.route)
        self.assertEqual(angle_addr.addr_spec, 'dinsdale@example.com')

    def test_get_angle_addr_empty_raise(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('')

    def test_get_angle_addr_left_angle_only_raise(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('<')

    def test_get_angle_addr_no_angle_raise(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('(foo) ')

    def test_get_angle_addr_no_angle_before_special_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('(foo) , next')

    def test_get_angle_addr_no_angle_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('bar')

    def test_get_angle_addr_special_after_angle_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_angle_addr('(foo) <, bar')

    # get_display_name  This is phrase but with a different value.

    def test_get_display_name_simple(self):
        display_name = self._test_get_x(parser.get_display_name,
            'Fred A Johnson',
            'Fred A Johnson',
            'Fred A Johnson',
            [],
            '')
        self.assertEqual(display_name.token_type, 'display-name')
        self.assertEqual(display_name.display_name, 'Fred A Johnson')

    def test_get_display_name_complex1(self):
        display_name = self._test_get_x(parser.get_display_name,
            '"Fred A. Johnson" is his name, oh.',
            '"Fred A. Johnson" is his name',
            '"Fred A. Johnson is his name"',
            [],
            ', oh.')
        self.assertEqual(display_name.token_type, 'display-name')
        self.assertEqual(display_name.display_name, 'Fred A. Johnson is his name')

    def test_get_display_name_complex2(self):
        display_name = self._test_get_x(parser.get_display_name,
            ' (A) bird (in (my|your)) "hand  " is messy\t<>\t',
            ' (A) bird (in (my|your)) "hand  " is messy\t',
            ' "bird hand   is messy" ',
            [],
            '<>\t')
        self.assertEqual(display_name[0][0].comments, ['A'])
        self.assertEqual(display_name[0][2].comments, ['in (my|your)'])
        self.assertEqual(display_name.display_name, 'bird hand   is messy')

    def test_get_display_name_obsolete(self):
        display_name = self._test_get_x(parser.get_display_name,
            'Fred A.(weird).O Johnson',
            'Fred A.(weird).O Johnson',
            '"Fred A. .O Johnson"',
            [errors.ObsoleteHeaderDefect]*3,
            '')
        self.assertEqual(len(display_name), 7)
        self.assertEqual(display_name[3].comments, ['weird'])
        self.assertEqual(display_name.display_name, 'Fred A. .O Johnson')

    def test_get_display_name_pharse_must_start_with_word(self):
        display_name = self._test_get_x(parser.get_display_name,
            '(even weirder).name',
            '(even weirder).name',
            ' ".name"',
            [errors.InvalidHeaderDefect] + [errors.ObsoleteHeaderDefect]*2,
            '')
        self.assertEqual(len(display_name), 3)
        self.assertEqual(display_name[0].comments, ['even weirder'])
        self.assertEqual(display_name.display_name, '.name')

    def test_get_display_name_ending_with_obsolete(self):
        display_name = self._test_get_x(parser.get_display_name,
            'simple phrase.(with trailing comment):boo',
            'simple phrase.(with trailing comment)',
            '"simple phrase." ',
            [errors.ObsoleteHeaderDefect]*2,
            ':boo')
        self.assertEqual(len(display_name), 4)
        self.assertEqual(display_name[3].comments, ['with trailing comment'])
        self.assertEqual(display_name.display_name, 'simple phrase.')

    def test_get_display_name_for_invalid_address_field(self):
        # bpo-32178: Test that address fields starting with `:` don't cause
        # IndexError when parsing the display name.
        display_name = self._test_get_x(
            parser.get_display_name,
            ':Foo ', '', '', [errors.InvalidHeaderDefect], ':Foo ')
        self.assertEqual(display_name.value, '')

    # get_name_addr

    def test_get_name_addr_angle_addr_only(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            [],
            '')
        self.assertEqual(name_addr.token_type, 'name-addr')
        self.assertIsNone(name_addr.display_name)
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_atom_name(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            'Dinsdale <dinsdale@example.com>',
            'Dinsdale <dinsdale@example.com>',
            'Dinsdale <dinsdale@example.com>',
            [],
            '')
        self.assertEqual(name_addr.token_type, 'name-addr')
        self.assertEqual(name_addr.display_name, 'Dinsdale')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_atom_name_with_cfws(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '(foo) Dinsdale (bar) <dinsdale@example.com> (bird)',
            '(foo) Dinsdale (bar) <dinsdale@example.com> (bird)',
            ' Dinsdale <dinsdale@example.com> ',
            [],
            '')
        self.assertEqual(name_addr.display_name, 'Dinsdale')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_name_with_cfws_and_dots(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '(foo) Roy.A.Bear (bar) <dinsdale@example.com> (bird)',
            '(foo) Roy.A.Bear (bar) <dinsdale@example.com> (bird)',
            ' "Roy.A.Bear" <dinsdale@example.com> ',
            [errors.ObsoleteHeaderDefect]*2,
            '')
        self.assertEqual(name_addr.display_name, 'Roy.A.Bear')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_qs_name(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '"Roy.A.Bear" <dinsdale@example.com>',
            '"Roy.A.Bear" <dinsdale@example.com>',
            '"Roy.A.Bear" <dinsdale@example.com>',
            [],
            '')
        self.assertEqual(name_addr.display_name, 'Roy.A.Bear')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_ending_with_dot_without_space(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            'John X.<jxd@example.com>',
            'John X.<jxd@example.com>',
            '"John X."<jxd@example.com>',
            [errors.ObsoleteHeaderDefect],
            '')
        self.assertEqual(name_addr.display_name, 'John X.')
        self.assertEqual(name_addr.local_part, 'jxd')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'jxd@example.com')

    def test_get_name_addr_starting_with_dot(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '. Doe <jxd@example.com>',
            '. Doe <jxd@example.com>',
            '". Doe" <jxd@example.com>',
            [errors.InvalidHeaderDefect, errors.ObsoleteHeaderDefect],
            '')
        self.assertEqual(name_addr.display_name, '. Doe')
        self.assertEqual(name_addr.local_part, 'jxd')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'jxd@example.com')

    def test_get_name_addr_with_route(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '"Roy.A.Bear" <@two.example.com: dinsdale@example.com>',
            '"Roy.A.Bear" <@two.example.com: dinsdale@example.com>',
            '"Roy.A.Bear" <@two.example.com: dinsdale@example.com>',
            [errors.ObsoleteHeaderDefect],
            '')
        self.assertEqual(name_addr.display_name, 'Roy.A.Bear')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertEqual(name_addr.route, ['two.example.com'])
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_ends_at_special(self):
        name_addr = self._test_get_x(parser.get_name_addr,
            '"Roy.A.Bear" <dinsdale@example.com>, next',
            '"Roy.A.Bear" <dinsdale@example.com>',
            '"Roy.A.Bear" <dinsdale@example.com>',
            [],
            ', next')
        self.assertEqual(name_addr.display_name, 'Roy.A.Bear')
        self.assertEqual(name_addr.local_part, 'dinsdale')
        self.assertEqual(name_addr.domain, 'example.com')
        self.assertIsNone(name_addr.route)
        self.assertEqual(name_addr.addr_spec, 'dinsdale@example.com')

    def test_get_name_addr_empty_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_name_addr('')

    def test_get_name_addr_no_content_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_name_addr(' (foo) ')

    def test_get_name_addr_no_content_before_special_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_name_addr(' (foo) ,')

    def test_get_name_addr_no_angle_after_display_name_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_name_addr('foo bar')

    # get_mailbox

    def test_get_mailbox_addr_spec_only(self):
        mailbox = self._test_get_x(parser.get_mailbox,
            'dinsdale@example.com',
            'dinsdale@example.com',
            'dinsdale@example.com',
            [],
            '')
        self.assertEqual(mailbox.token_type, 'mailbox')
        self.assertIsNone(mailbox.display_name)
        self.assertEqual(mailbox.local_part, 'dinsdale')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertIsNone(mailbox.route)
        self.assertEqual(mailbox.addr_spec, 'dinsdale@example.com')

    def test_get_mailbox_angle_addr_only(self):
        mailbox = self._test_get_x(parser.get_mailbox,
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            '<dinsdale@example.com>',
            [],
            '')
        self.assertEqual(mailbox.token_type, 'mailbox')
        self.assertIsNone(mailbox.display_name)
        self.assertEqual(mailbox.local_part, 'dinsdale')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertIsNone(mailbox.route)
        self.assertEqual(mailbox.addr_spec, 'dinsdale@example.com')

    def test_get_mailbox_name_addr(self):
        mailbox = self._test_get_x(parser.get_mailbox,
            '"Roy A. Bear" <dinsdale@example.com>',
            '"Roy A. Bear" <dinsdale@example.com>',
            '"Roy A. Bear" <dinsdale@example.com>',
            [],
            '')
        self.assertEqual(mailbox.token_type, 'mailbox')
        self.assertEqual(mailbox.display_name, 'Roy A. Bear')
        self.assertEqual(mailbox.local_part, 'dinsdale')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertIsNone(mailbox.route)
        self.assertEqual(mailbox.addr_spec, 'dinsdale@example.com')

    def test_get_mailbox_ends_at_special(self):
        mailbox = self._test_get_x(parser.get_mailbox,
            '"Roy A. Bear" <dinsdale@example.com>, rest',
            '"Roy A. Bear" <dinsdale@example.com>',
            '"Roy A. Bear" <dinsdale@example.com>',
            [],
            ', rest')
        self.assertEqual(mailbox.token_type, 'mailbox')
        self.assertEqual(mailbox.display_name, 'Roy A. Bear')
        self.assertEqual(mailbox.local_part, 'dinsdale')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertIsNone(mailbox.route)
        self.assertEqual(mailbox.addr_spec, 'dinsdale@example.com')

    def test_get_mailbox_quoted_strings_in_atom_list(self):
        mailbox = self._test_get_x(parser.get_mailbox,
            '""example" example"@example.com',
            '""example" example"@example.com',
            'example example@example.com',
            [errors.InvalidHeaderDefect]*3,
            '')
        self.assertEqual(mailbox.local_part, 'example example')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertEqual(mailbox.addr_spec, '"example example"@example.com')

    # get_mailbox_list

    def test_get_mailbox_list_single_addr(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            'dinsdale@example.com',
            'dinsdale@example.com',
            'dinsdale@example.com',
            [],
            '')
        self.assertEqual(mailbox_list.token_type, 'mailbox-list')
        self.assertEqual(len(mailbox_list.mailboxes), 1)
        mailbox = mailbox_list.mailboxes[0]
        self.assertIsNone(mailbox.display_name)
        self.assertEqual(mailbox.local_part, 'dinsdale')
        self.assertEqual(mailbox.domain, 'example.com')
        self.assertIsNone(mailbox.route)
        self.assertEqual(mailbox.addr_spec, 'dinsdale@example.com')
        self.assertEqual(mailbox_list.mailboxes,
                         mailbox_list.all_mailboxes)

    def test_get_mailbox_list_two_simple_addr(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            'dinsdale@example.com, dinsdale@test.example.com',
            'dinsdale@example.com, dinsdale@test.example.com',
            'dinsdale@example.com, dinsdale@test.example.com',
            [],
            '')
        self.assertEqual(mailbox_list.token_type, 'mailbox-list')
        self.assertEqual(len(mailbox_list.mailboxes), 2)
        self.assertEqual(mailbox_list.mailboxes[0].addr_spec,
                        'dinsdale@example.com')
        self.assertEqual(mailbox_list.mailboxes[1].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes,
                         mailbox_list.all_mailboxes)

    def test_get_mailbox_list_two_name_addr(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            ('"Roy A. Bear" <dinsdale@example.com>,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            [],
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 2)
        self.assertEqual(mailbox_list.mailboxes[0].addr_spec,
                        'dinsdale@example.com')
        self.assertEqual(mailbox_list.mailboxes[0].display_name,
                        'Roy A. Bear')
        self.assertEqual(mailbox_list.mailboxes[1].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes[1].display_name,
                        'Fred Flintstone')
        self.assertEqual(mailbox_list.mailboxes,
                         mailbox_list.all_mailboxes)

    def test_get_mailbox_list_two_complex(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            ('(foo) "Roy A. Bear" <dinsdale@example.com>(bar),'
                ' "Fred Flintstone" <dinsdale@test.(bird)example.com>'),
            ('(foo) "Roy A. Bear" <dinsdale@example.com>(bar),'
                ' "Fred Flintstone" <dinsdale@test.(bird)example.com>'),
            (' "Roy A. Bear" <dinsdale@example.com> ,'
                ' "Fred Flintstone" <dinsdale@test. example.com>'),
            [errors.ObsoleteHeaderDefect],
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 2)
        self.assertEqual(mailbox_list.mailboxes[0].addr_spec,
                        'dinsdale@example.com')
        self.assertEqual(mailbox_list.mailboxes[0].display_name,
                        'Roy A. Bear')
        self.assertEqual(mailbox_list.mailboxes[1].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes[1].display_name,
                        'Fred Flintstone')
        self.assertEqual(mailbox_list.mailboxes,
                         mailbox_list.all_mailboxes)

    def test_get_mailbox_list_unparseable_mailbox_null(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            ('"Roy A. Bear"[] dinsdale@example.com,'
                ' "Fred Flintstone" <dinsdale@test.(bird)example.com>'),
            ('"Roy A. Bear"[] dinsdale@example.com,'
                ' "Fred Flintstone" <dinsdale@test.(bird)example.com>'),
            ('"Roy A. Bear"[] dinsdale@example.com,'
                ' "Fred Flintstone" <dinsdale@test. example.com>'),
            [errors.InvalidHeaderDefect,   # the 'extra' text after the local part
             errors.InvalidHeaderDefect,   # the local part with no angle-addr
             errors.ObsoleteHeaderDefect,  # period in extra text (example.com)
             errors.ObsoleteHeaderDefect], # (bird) in valid address.
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 1)
        self.assertEqual(len(mailbox_list.all_mailboxes), 2)
        self.assertEqual(mailbox_list.all_mailboxes[0].token_type,
                        'invalid-mailbox')
        self.assertIsNone(mailbox_list.all_mailboxes[0].display_name)
        self.assertEqual(mailbox_list.all_mailboxes[0].local_part,
                        'Roy A. Bear')
        self.assertIsNone(mailbox_list.all_mailboxes[0].domain)
        self.assertEqual(mailbox_list.all_mailboxes[0].addr_spec,
                        '"Roy A. Bear"')
        self.assertIs(mailbox_list.all_mailboxes[1],
                        mailbox_list.mailboxes[0])
        self.assertEqual(mailbox_list.mailboxes[0].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes[0].display_name,
                        'Fred Flintstone')

    def test_get_mailbox_list_junk_after_valid_address(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            ('"Roy A. Bear" <dinsdale@example.com>@@,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>@@,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>@@,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 1)
        self.assertEqual(len(mailbox_list.all_mailboxes), 2)
        self.assertEqual(mailbox_list.all_mailboxes[0].addr_spec,
                        'dinsdale@example.com')
        self.assertEqual(mailbox_list.all_mailboxes[0].display_name,
                        'Roy A. Bear')
        self.assertEqual(mailbox_list.all_mailboxes[0].token_type,
                        'invalid-mailbox')
        self.assertIs(mailbox_list.all_mailboxes[1],
                        mailbox_list.mailboxes[0])
        self.assertEqual(mailbox_list.mailboxes[0].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes[0].display_name,
                        'Fred Flintstone')

    def test_get_mailbox_list_empty_list_element(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            ('"Roy A. Bear" <dinsdale@example.com>, (bird),,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, (bird),,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, ,,'
                ' "Fred Flintstone" <dinsdale@test.example.com>'),
            [errors.ObsoleteHeaderDefect]*2,
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 2)
        self.assertEqual(mailbox_list.all_mailboxes,
                         mailbox_list.mailboxes)
        self.assertEqual(mailbox_list.all_mailboxes[0].addr_spec,
                        'dinsdale@example.com')
        self.assertEqual(mailbox_list.all_mailboxes[0].display_name,
                        'Roy A. Bear')
        self.assertEqual(mailbox_list.mailboxes[1].addr_spec,
                        'dinsdale@test.example.com')
        self.assertEqual(mailbox_list.mailboxes[1].display_name,
                        'Fred Flintstone')

    def test_get_mailbox_list_only_empty_elements(self):
        mailbox_list = self._test_get_x(parser.get_mailbox_list,
            '(foo),, (bar)',
            '(foo),, (bar)',
            ' ,, ',
            [errors.ObsoleteHeaderDefect]*3,
            '')
        self.assertEqual(len(mailbox_list.mailboxes), 0)
        self.assertEqual(mailbox_list.all_mailboxes,
                         mailbox_list.mailboxes)

    # get_group_list

    def test_get_group_list_cfws_only(self):
        group_list = self._test_get_x(parser.get_group_list,
            '(hidden);',
            '(hidden)',
            ' ',
            [],
            ';')
        self.assertEqual(group_list.token_type, 'group-list')
        self.assertEqual(len(group_list.mailboxes), 0)
        self.assertEqual(group_list.mailboxes,
                         group_list.all_mailboxes)

    def test_get_group_list_mailbox_list(self):
        group_list = self._test_get_x(parser.get_group_list,
            'dinsdale@example.org, "Fred A. Bear" <dinsdale@example.org>',
            'dinsdale@example.org, "Fred A. Bear" <dinsdale@example.org>',
            'dinsdale@example.org, "Fred A. Bear" <dinsdale@example.org>',
            [],
            '')
        self.assertEqual(group_list.token_type, 'group-list')
        self.assertEqual(len(group_list.mailboxes), 2)
        self.assertEqual(group_list.mailboxes,
                         group_list.all_mailboxes)
        self.assertEqual(group_list.mailboxes[1].display_name,
                         'Fred A. Bear')

    def test_get_group_list_obs_group_list(self):
        group_list = self._test_get_x(parser.get_group_list,
            ', (foo),,(bar)',
            ', (foo),,(bar)',
            ', ,, ',
            [errors.ObsoleteHeaderDefect] * 5,
            '')
        self.assertEqual(group_list.token_type, 'group-list')
        self.assertEqual(len(group_list.mailboxes), 0)
        self.assertEqual(group_list.mailboxes,
                         group_list.all_mailboxes)

    def test_get_group_list_comment_only_invalid(self):
        group_list = self._test_get_x(parser.get_group_list,
            '(bar)',
            '(bar)',
            ' ',
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(group_list.token_type, 'group-list')
        self.assertEqual(len(group_list.mailboxes), 0)
        self.assertEqual(group_list.mailboxes,
                         group_list.all_mailboxes)

    # get_group

    def test_get_group_empty(self):
        group = self._test_get_x(parser.get_group,
            'Monty Python:;',
            'Monty Python:;',
            'Monty Python:;',
            [],
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 0)
        self.assertEqual(group.mailboxes,
                         group.all_mailboxes)

    def test_get_group_null_addr_spec(self):
        group = self._test_get_x(parser.get_group,
            'foo: <>;',
            'foo: <>;',
            'foo: <>;',
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(group.display_name, 'foo')
        self.assertEqual(len(group.mailboxes), 0)
        self.assertEqual(len(group.all_mailboxes), 1)
        self.assertEqual(group.all_mailboxes[0].value, '<>')

    def test_get_group_cfws_only(self):
        group = self._test_get_x(parser.get_group,
            'Monty Python: (hidden);',
            'Monty Python: (hidden);',
            'Monty Python: ;',
            [],
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 0)
        self.assertEqual(group.mailboxes,
                         group.all_mailboxes)

    def test_get_group_single_mailbox(self):
        group = self._test_get_x(parser.get_group,
            'Monty Python: "Fred A. Bear" <dinsdale@example.com>;',
            'Monty Python: "Fred A. Bear" <dinsdale@example.com>;',
            'Monty Python: "Fred A. Bear" <dinsdale@example.com>;',
            [],
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 1)
        self.assertEqual(group.mailboxes,
                         group.all_mailboxes)
        self.assertEqual(group.mailboxes[0].addr_spec,
                         'dinsdale@example.com')

    def test_get_group_mixed_list(self):
        group = self._test_get_x(parser.get_group,
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                '(foo) Roger <ping@exampele.com>, x@test.example.com;'),
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                '(foo) Roger <ping@exampele.com>, x@test.example.com;'),
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                ' Roger <ping@exampele.com>, x@test.example.com;'),
            [],
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 3)
        self.assertEqual(group.mailboxes,
                         group.all_mailboxes)
        self.assertEqual(group.mailboxes[0].display_name,
                         'Fred A. Bear')
        self.assertEqual(group.mailboxes[1].display_name,
                         'Roger')
        self.assertEqual(group.mailboxes[2].local_part, 'x')

    def test_get_group_one_invalid(self):
        group = self._test_get_x(parser.get_group,
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                '(foo) Roger ping@exampele.com, x@test.example.com;'),
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                '(foo) Roger ping@exampele.com, x@test.example.com;'),
            ('Monty Python: "Fred A. Bear" <dinsdale@example.com>,'
                ' Roger ping@exampele.com, x@test.example.com;'),
            [errors.InvalidHeaderDefect,   # non-angle addr makes local part invalid
             errors.InvalidHeaderDefect],   # and its not obs-local either: no dots.
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 2)
        self.assertEqual(len(group.all_mailboxes), 3)
        self.assertEqual(group.mailboxes[0].display_name,
                         'Fred A. Bear')
        self.assertEqual(group.mailboxes[1].local_part, 'x')
        self.assertIsNone(group.all_mailboxes[1].display_name)

    def test_get_group_missing_final_semicol(self):
        group = self._test_get_x(parser.get_group,
            ('Monty Python:"Fred A. Bear" <dinsdale@example.com>,'
             'eric@where.test,John <jdoe@test>'),
            ('Monty Python:"Fred A. Bear" <dinsdale@example.com>,'
             'eric@where.test,John <jdoe@test>;'),
            ('Monty Python:"Fred A. Bear" <dinsdale@example.com>,'
             'eric@where.test,John <jdoe@test>;'),
            [errors.InvalidHeaderDefect],
            '')
        self.assertEqual(group.token_type, 'group')
        self.assertEqual(group.display_name, 'Monty Python')
        self.assertEqual(len(group.mailboxes), 3)
        self.assertEqual(group.mailboxes,
                         group.all_mailboxes)
        self.assertEqual(group.mailboxes[0].addr_spec,
                         'dinsdale@example.com')
        self.assertEqual(group.mailboxes[0].display_name,
                         'Fred A. Bear')
        self.assertEqual(group.mailboxes[1].addr_spec,
                         'eric@where.test')
        self.assertEqual(group.mailboxes[2].display_name,
                         'John')
        self.assertEqual(group.mailboxes[2].addr_spec,
                         'jdoe@test')
    # get_address

    def test_get_address_simple(self):
        address = self._test_get_x(parser.get_address,
            'dinsdale@example.com',
            'dinsdale@example.com',
            'dinsdale@example.com',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].domain,
                         'example.com')
        self.assertEqual(address[0].token_type,
                         'mailbox')

    def test_get_address_complex(self):
        address = self._test_get_x(parser.get_address,
            '(foo) "Fred A. Bear" <(bird)dinsdale@example.com>',
            '(foo) "Fred A. Bear" <(bird)dinsdale@example.com>',
            ' "Fred A. Bear" < dinsdale@example.com>',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].display_name,
                         'Fred A. Bear')
        self.assertEqual(address[0].token_type,
                         'mailbox')

    def test_get_address_rfc2047_display_name(self):
        address = self._test_get_x(parser.get_address,
            '=?utf-8?q?=C3=89ric?= <foo@example.com>',
            'Éric <foo@example.com>',
            'Éric <foo@example.com>',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].display_name,
                         'Éric')
        self.assertEqual(address[0].token_type,
                         'mailbox')

    def test_get_address_rfc2047_display_name_adjacent_ews(self):
        address = self._test_get_x(parser.get_address,
            '=?utf-8?q?B=C3=A9r?= =?utf-8?q?=C3=A9nice?= <foo@example.com>',
            'Bérénice <foo@example.com>',
            'Bérénice <foo@example.com>',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].display_name,
                         'Bérénice')
        self.assertEqual(address[0].token_type,
                         'mailbox')

    def test_get_address_empty_group(self):
        address = self._test_get_x(parser.get_address,
            'Monty Python:;',
            'Monty Python:;',
            'Monty Python:;',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 0)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address[0].token_type,
                         'group')
        self.assertEqual(address[0].display_name,
                         'Monty Python')

    def test_get_address_group(self):
        address = self._test_get_x(parser.get_address,
            'Monty Python: x@example.com, y@example.com;',
            'Monty Python: x@example.com, y@example.com;',
            'Monty Python: x@example.com, y@example.com;',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 2)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address[0].token_type,
                         'group')
        self.assertEqual(address[0].display_name,
                         'Monty Python')
        self.assertEqual(address.mailboxes[0].local_part, 'x')

    def test_get_address_quoted_local_part(self):
        address = self._test_get_x(parser.get_address,
            '"foo bar"@example.com',
            '"foo bar"@example.com',
            '"foo bar"@example.com',
            [],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].domain,
                         'example.com')
        self.assertEqual(address.mailboxes[0].local_part,
                         'foo bar')
        self.assertEqual(address[0].token_type, 'mailbox')

    def test_get_address_ends_at_special(self):
        address = self._test_get_x(parser.get_address,
            'dinsdale@example.com, next',
            'dinsdale@example.com',
            'dinsdale@example.com',
            [],
            ', next')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 1)
        self.assertEqual(address.mailboxes,
                         address.all_mailboxes)
        self.assertEqual(address.mailboxes[0].domain,
                         'example.com')
        self.assertEqual(address[0].token_type, 'mailbox')

    def test_get_address_invalid_mailbox_invalid(self):
        address = self._test_get_x(parser.get_address,
            'ping example.com, next',
            'ping example.com',
            'ping example.com',
            [errors.InvalidHeaderDefect,    # addr-spec with no domain
             errors.InvalidHeaderDefect,    # invalid local-part
             errors.InvalidHeaderDefect,    # missing .s in local-part
            ],
            ', next')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 0)
        self.assertEqual(len(address.all_mailboxes), 1)
        self.assertIsNone(address.all_mailboxes[0].domain)
        self.assertEqual(address.all_mailboxes[0].local_part, 'ping example.com')
        self.assertEqual(address[0].token_type, 'invalid-mailbox')

    def test_get_address_quoted_strings_in_atom_list(self):
        address = self._test_get_x(parser.get_address,
            '""example" example"@example.com',
            '""example" example"@example.com',
            'example example@example.com',
            [errors.InvalidHeaderDefect]*3,
            '')
        self.assertEqual(address.all_mailboxes[0].local_part, 'example example')
        self.assertEqual(address.all_mailboxes[0].domain, 'example.com')
        self.assertEqual(address.all_mailboxes[0].addr_spec, '"example example"@example.com')

    def test_get_address_with_invalid_domain(self):
        address = self._test_get_x(parser.get_address,
            '<T@[',
            '<T@[]>',
            '<T@[]>',
            [errors.InvalidHeaderDefect,    # missing trailing '>' on angle-addr
             errors.InvalidHeaderDefect,    # end of input inside domain-literal
            ],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 0)
        self.assertEqual(len(address.all_mailboxes), 1)
        self.assertEqual(address.all_mailboxes[0].domain, '[]')
        self.assertEqual(address.all_mailboxes[0].local_part, 'T')
        self.assertEqual(address.all_mailboxes[0].token_type, 'invalid-mailbox')
        self.assertEqual(address[0].token_type, 'invalid-mailbox')

        address = self._test_get_x(parser.get_address,
            '!an??:=m==fr2@[C',
            '!an??:=m==fr2@[C];',
            '!an??:=m==fr2@[C];',
            [errors.InvalidHeaderDefect,    # end of header in group
             errors.InvalidHeaderDefect,    # end of input inside domain-literal
            ],
            '')
        self.assertEqual(address.token_type, 'address')
        self.assertEqual(len(address.mailboxes), 0)
        self.assertEqual(len(address.all_mailboxes), 1)
        self.assertEqual(address.all_mailboxes[0].domain, '[C]')
        self.assertEqual(address.all_mailboxes[0].local_part, '=m==fr2')
        self.assertEqual(address.all_mailboxes[0].token_type, 'invalid-mailbox')
        self.assertEqual(address[0].token_type, 'group')

    # get_address_list

    def test_get_address_list_CFWS(self):
        address_list = self._test_get_x(parser.get_address_list,
                                        '(Recipient list suppressed)',
                                        '(Recipient list suppressed)',
                                        ' ',
                                        [errors.ObsoleteHeaderDefect],  # no content in address list
                                        '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 0)
        self.assertEqual(address_list.mailboxes, address_list.all_mailboxes)

    def test_get_address_list_mailboxes_simple(self):
        address_list = self._test_get_x(parser.get_address_list,
            'dinsdale@example.com',
            'dinsdale@example.com',
            'dinsdale@example.com',
            [],
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 1)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual([str(x) for x in address_list.mailboxes],
                         [str(x) for x in address_list.addresses])
        self.assertEqual(address_list.mailboxes[0].domain, 'example.com')
        self.assertEqual(address_list[0].token_type, 'address')
        self.assertIsNone(address_list[0].display_name)

    def test_get_address_list_mailboxes_two_simple(self):
        address_list = self._test_get_x(parser.get_address_list,
            'foo@example.com, "Fred A. Bar" <bar@example.com>',
            'foo@example.com, "Fred A. Bar" <bar@example.com>',
            'foo@example.com, "Fred A. Bar" <bar@example.com>',
            [],
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 2)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual([str(x) for x in address_list.mailboxes],
                         [str(x) for x in address_list.addresses])
        self.assertEqual(address_list.mailboxes[0].local_part, 'foo')
        self.assertEqual(address_list.mailboxes[1].display_name, "Fred A. Bar")

    def test_get_address_list_mailboxes_complex(self):
        address_list = self._test_get_x(parser.get_address_list,
            ('"Roy A. Bear" <dinsdale@example.com>, '
                '(ping) Foo <x@example.com>,'
                'Nobody Is. Special <y@(bird)example.(bad)com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, '
                '(ping) Foo <x@example.com>,'
                'Nobody Is. Special <y@(bird)example.(bad)com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, '
                'Foo <x@example.com>,'
                '"Nobody Is. Special" <y@example. com>'),
            [errors.ObsoleteHeaderDefect, # period in Is.
            errors.ObsoleteHeaderDefect], # cfws in domain
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 3)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual([str(x) for x in address_list.mailboxes],
                         [str(x) for x in address_list.addresses])
        self.assertEqual(address_list.mailboxes[0].domain, 'example.com')
        self.assertEqual(address_list.mailboxes[0].token_type, 'mailbox')
        self.assertEqual(address_list.addresses[0].token_type, 'address')
        self.assertEqual(address_list.mailboxes[1].local_part, 'x')
        self.assertEqual(address_list.mailboxes[2].display_name,
                         'Nobody Is. Special')

    def test_get_address_list_mailboxes_invalid_addresses(self):
        address_list = self._test_get_x(parser.get_address_list,
            ('"Roy A. Bear" <dinsdale@example.com>, '
                '(ping) Foo x@example.com[],'
                'Nobody Is. Special <(bird)example.(bad)com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, '
                '(ping) Foo x@example.com[],'
                'Nobody Is. Special <(bird)example.(bad)com>'),
            ('"Roy A. Bear" <dinsdale@example.com>, '
                'Foo x@example.com[],'
                '"Nobody Is. Special" < example. com>'),
             [errors.InvalidHeaderDefect,   # invalid address in list
              errors.InvalidHeaderDefect,   # 'Foo x' local part invalid.
              errors.InvalidHeaderDefect,   # Missing . in 'Foo x' local part
              errors.ObsoleteHeaderDefect,  # period in 'Is.' disp-name phrase
              errors.InvalidHeaderDefect,   # no domain part in addr-spec
              errors.ObsoleteHeaderDefect], # addr-spec has comment in it
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 1)
        self.assertEqual(len(address_list.all_mailboxes), 4)
        self.assertEqual([str(x) for x in address_list.all_mailboxes],
                         [str(x) for x in address_list.addresses])
        self.assertEqual(address_list.mailboxes[0].domain, 'example.com')
        self.assertEqual(address_list.mailboxes[0].token_type, 'mailbox')
        self.assertEqual(address_list.addresses[0].token_type, 'address')
        self.assertEqual(address_list.addresses[1].token_type, 'address')
        self.assertEqual(len(address_list.addresses[0].mailboxes), 1)
        self.assertEqual(len(address_list.addresses[1].mailboxes), 0)
        self.assertEqual(len(address_list.addresses[2].mailboxes), 0)
        self.assertEqual(len(address_list.addresses[3].mailboxes), 0)
        self.assertEqual(
            address_list.addresses[1].all_mailboxes[0].local_part, 'Foo x')
        self.assertEqual(address_list.addresses[2].all_mailboxes[0].value, '[]')
        self.assertEqual(
            address_list.addresses[3].all_mailboxes[0].display_name,
                "Nobody Is. Special")

    def test_get_address_list_group_empty(self):
        address_list = self._test_get_x(parser.get_address_list,
            'Monty Python: ;',
            'Monty Python: ;',
            'Monty Python: ;',
            [],
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 0)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual(len(address_list.addresses), 1)
        self.assertEqual(address_list.addresses[0].token_type, 'address')
        self.assertEqual(address_list.addresses[0].display_name, 'Monty Python')
        self.assertEqual(len(address_list.addresses[0].mailboxes), 0)

    def test_get_address_list_group_simple(self):
        address_list = self._test_get_x(parser.get_address_list,
            'Monty Python: dinsdale@example.com;',
            'Monty Python: dinsdale@example.com;',
            'Monty Python: dinsdale@example.com;',
            [],
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 1)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual(address_list.mailboxes[0].domain, 'example.com')
        self.assertEqual(address_list.addresses[0].display_name,
                         'Monty Python')
        self.assertEqual(address_list.addresses[0].mailboxes[0].domain,
                         'example.com')

    def test_get_address_list_group_and_mailboxes(self):
        address_list = self._test_get_x(parser.get_address_list,
            ('Monty Python: dinsdale@example.com, "Fred" <flint@example.com>;, '
                'Abe <x@example.com>, Bee <y@example.com>'),
            ('Monty Python: dinsdale@example.com, "Fred" <flint@example.com>;, '
                'Abe <x@example.com>, Bee <y@example.com>'),
            ('Monty Python: dinsdale@example.com, "Fred" <flint@example.com>;, '
                'Abe <x@example.com>, Bee <y@example.com>'),
            [],
            '')
        self.assertEqual(address_list.token_type, 'address-list')
        self.assertEqual(len(address_list.mailboxes), 4)
        self.assertEqual(address_list.mailboxes,
                         address_list.all_mailboxes)
        self.assertEqual(len(address_list.addresses), 3)
        self.assertEqual(address_list.mailboxes[0].local_part, 'dinsdale')
        self.assertEqual(address_list.addresses[0].display_name,
                         'Monty Python')
        self.assertEqual(address_list.addresses[0].mailboxes[0].domain,
                         'example.com')
        self.assertEqual(address_list.addresses[0].mailboxes[1].local_part,
                         'flint')
        self.assertEqual(address_list.addresses[1].mailboxes[0].local_part,
                         'x')
        self.assertEqual(address_list.addresses[2].mailboxes[0].local_part,
                         'y')
        self.assertEqual(str(address_list.addresses[1]),
                         str(address_list.mailboxes[2]))

    def test_get_address_list_trailing_garbage(self):
        address_list = self._test_get_x(parser.get_address_list,
            'unlisted-recipients:; (no To-header on input)',
            'unlisted-recipients:; (no To-header on input)',
            'unlisted-recipients:; ',
            [errors.InvalidHeaderDefect]*2 + [errors.ObsoleteHeaderDefect],
            '')

    def test_invalid_content_disposition(self):
        content_disp = self._test_parse_x(
            parser.parse_content_disposition_header,
            ";attachment", "; attachment", ";attachment",
            [errors.InvalidHeaderDefect]*2
        )

    def test_invalid_content_transfer_encoding(self):
        cte = self._test_parse_x(
            parser.parse_content_transfer_encoding_header,
            ";foo", ";foo", ";foo", [errors.InvalidHeaderDefect]*3
        )

    # get_msg_id

    def test_get_msg_id_empty(self):
        # bpo-38708: Test that HeaderParseError is raised and not IndexError.
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id('')

    def test_get_msg_id_valid(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "<simeple.local@example.something.com>",
            "<simeple.local@example.something.com>",
            "<simeple.local@example.something.com>",
            [],
            '',
            )
        self.assertEqual(msg_id.token_type, 'msg-id')

    def test_get_msg_id_obsolete_local(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            '<"simeple.local"@example.com>',
            '<"simeple.local"@example.com>',
            '<simeple.local@example.com>',
            [errors.ObsoleteHeaderDefect],
            '',
            )
        self.assertEqual(msg_id.token_type, 'msg-id')

    def test_get_msg_id_non_folding_literal_domain(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "<simple.local@[someexamplecom.domain]>",
            "<simple.local@[someexamplecom.domain]>",
            "<simple.local@[someexamplecom.domain]>",
            [],
            "",
            )
        self.assertEqual(msg_id.token_type, 'msg-id')


    def test_get_msg_id_obsolete_domain_part(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "<simplelocal@(old)example.com>",
            "<simplelocal@(old)example.com>",
            "<simplelocal@ example.com>",
            [errors.ObsoleteHeaderDefect],
            ""
        )

    def test_get_msg_id_no_id_right_part(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "<simplelocal>",
            "<simplelocal>",
            "<simplelocal>",
            [errors.InvalidHeaderDefect],
            ""
        )
        self.assertEqual(msg_id.token_type, 'msg-id')

    def test_get_msg_id_invalid_expected_msg_id_not_found(self):
        text = "935-XPB-567:0:45327:9:90305:17843586-40@example.com"
        msg_id = parser.parse_message_id(text)
        self.assertDefectsEqual(
            msg_id.all_defects,
            [errors.InvalidHeaderDefect])

    def test_parse_invalid_message_id(self):
        message_id = self._test_parse_x(
            parser.parse_message_id,
            "935-XPB-567:0:45327:9:90305:17843586-40@example.com",
            "935-XPB-567:0:45327:9:90305:17843586-40@example.com",
            "935-XPB-567:0:45327:9:90305:17843586-40@example.com",
            [errors.InvalidHeaderDefect],
            )
        self.assertEqual(message_id.token_type, 'invalid-message-id')

    def test_parse_valid_message_id(self):
        message_id = self._test_parse_x(
            parser.parse_message_id,
            "<aperson@somedomain>",
            "<aperson@somedomain>",
            "<aperson@somedomain>",
            [],
            )
        self.assertEqual(message_id.token_type, 'message-id')

    def test_parse_message_id_with_invalid_domain(self):
        message_id = self._test_parse_x(
            parser.parse_message_id,
            "<T@[",
            "<T@[]>",
            "<T@[]>",
            [errors.ObsoleteHeaderDefect] + [errors.InvalidHeaderDefect] * 2,
            [],
            )
        self.assertEqual(message_id.token_type, 'message-id')
        self.assertEqual(str(message_id.all_defects[-1]),
                         "end of input inside domain-literal")

    def test_parse_message_id_with_remaining(self):
        message_id = self._test_parse_x(
            parser.parse_message_id,
            "<validmessageid@example>thensomething",
            "<validmessageid@example>",
            "<validmessageid@example>",
            [errors.InvalidHeaderDefect],
            [],
            )
        self.assertEqual(message_id.token_type, 'message-id')
        self.assertEqual(str(message_id.all_defects[0]),
                         "Unexpected 'thensomething'")

    def test_get_msg_id_no_angle_start(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id("msgwithnoankle")

    def test_get_msg_id_no_angle_end(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "<simplelocal@domain",
            "<simplelocal@domain>",
            "<simplelocal@domain>",
            [errors.InvalidHeaderDefect],
            ""
        )
        self.assertEqual(msg_id.token_type, 'msg-id')

    def test_get_msg_id_empty_id_left(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id("<@domain>")

    def test_get_msg_id_empty_id_right(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id("<simplelocal@>")

    def test_get_msg_id_no_id_right(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id("<simplelocal@")

    def test_get_msg_id_with_brackets(self):
        # Microsoft Outlook generates non-standard one-off addresses:
        # https://learn.microsoft.com/en-us/office/client-developer/outlook/mapi/one-off-addresses
        with self.assertRaises(errors.HeaderParseError):
            parser.get_msg_id("<[abrakadabra@microsoft.com]>")

    def test_get_msg_id_ws_only_local(self):
        msg_id = self._test_get_x(
            parser.get_msg_id,
            "< @domain>",
            "< @domain>",
            "< @domain>",
            [errors.ObsoleteHeaderDefect],
            ""
        )
        self.assertEqual(msg_id.token_type, 'msg-id')

    def test_parse_message_ids_valid(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<foo@bar> <bar@foo>",
            "<foo@bar> <bar@foo>",
            "<foo@bar> <bar@foo>",
            [],
            )
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_empty(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            " ",
            " ",
            " ",
            [errors.InvalidHeaderDefect],
            )
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_comment(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<foo@bar> (foo's message from \"bar\")",
            "<foo@bar> (foo's message from \"bar\")",
            "<foo@bar> ",
            [],
            )
        self.assertEqual(message_ids.message_ids[0].value, '<foo@bar> ')
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_no_sep(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<foo@bar><bar@foo>",
            "<foo@bar><bar@foo>",
            "<foo@bar><bar@foo>",
            [],
            )
        self.assertEqual(message_ids.message_ids[0].value, '<foo@bar>')
        self.assertEqual(message_ids.message_ids[1].value, '<bar@foo>')
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_comma_sep(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<foo@bar>,<bar@foo>",
            "<foo@bar> <bar@foo>",
            "<foo@bar> <bar@foo>",
            [errors.InvalidHeaderDefect],
            )
        self.assertEqual(message_ids.message_ids[0].value, '<foo@bar>')
        self.assertEqual(message_ids.message_ids[1].value, '<bar@foo>')
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_invalid_id(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<Date: Wed, 08 Jun 2002 09:78:58 +0600>",
            "<Date: Wed, 08 Jun 2002 09:78:58 +0600>",
            "<Date: Wed, 08 Jun 2002 09:78:58 +0600>",
            [errors.InvalidHeaderDefect]*2,
            )
        self.assertEqual(message_ids.token_type, 'message-id-list')

    def test_parse_message_ids_broken_ang(self):
        message_ids = self._test_parse_x(
            parser.parse_message_ids,
            "<foo@bar> >bar@foo",
            "<foo@bar> >bar@foo",
            "<foo@bar> >bar@foo",
            [errors.InvalidHeaderDefect]*1,
            )
        self.assertEqual(message_ids.token_type, 'message-id-list')



@parameterize
class Test_parse_mime_parameters(TestParserMixin, TestEmailBase):

    def mime_parameters_as_value(self,
                                 value,
                                 tl_str,
                                 tl_value,
                                 params,
                                 defects):
        mime_parameters = self._test_parse_x(parser.parse_mime_parameters,
            value, tl_str, tl_value, defects)
        self.assertEqual(mime_parameters.token_type, 'mime-parameters')
        self.assertEqual(list(mime_parameters.params), params)


    mime_parameters_params = {

        'simple': (
            'filename="abc.py"',
            ' filename="abc.py"',
            'filename=abc.py',
            [('filename', 'abc.py')],
            []),

        'multiple_keys': (
            'filename="abc.py"; xyz=abc',
            ' filename="abc.py"; xyz="abc"',
            'filename=abc.py; xyz=abc',
            [('filename', 'abc.py'), ('xyz', 'abc')],
            []),

        'split_value': (
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66",
            ' filename="201.tif"',
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66",
            [('filename', '201.tif')],
            []),

        # Note that it is undefined what we should do for error recovery when
        # there are duplicate parameter names or duplicate parts in a split
        # part.  We choose to ignore all duplicate parameters after the first
        # and to take duplicate or missing rfc 2231 parts in appearance order.
        # This is backward compatible with get_param's behavior, but the
        # decisions are arbitrary.

        'duplicate_key': (
            'filename=abc.gif; filename=def.tiff',
            ' filename="abc.gif"',
            "filename=abc.gif; filename=def.tiff",
            [('filename', 'abc.gif')],
            [errors.InvalidHeaderDefect]),

        'duplicate_key_with_split_value': (
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66;"
                " filename=abc.gif",
            ' filename="201.tif"',
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66;"
                " filename=abc.gif",
            [('filename', '201.tif')],
            [errors.InvalidHeaderDefect]),

        'duplicate_key_with_split_value_other_order': (
            "filename=abc.gif; "
                " filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66",
            ' filename="abc.gif"',
            "filename=abc.gif;"
                " filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66",
            [('filename', 'abc.gif')],
            [errors.InvalidHeaderDefect]),

        'duplicate_in_split_value': (
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66;"
                " filename*1*=abc.gif",
            ' filename="201.tifabc.gif"',
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*1*=%74%69%66;"
                " filename*1*=abc.gif",
            [('filename', '201.tifabc.gif')],
            [errors.InvalidHeaderDefect]),

        'missing_split_value': (
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66;",
            ' filename="201.tif"',
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66;",
            [('filename', '201.tif')],
            [errors.InvalidHeaderDefect]),

        'duplicate_and_missing_split_value': (
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66;"
                " filename*3*=abc.gif",
            ' filename="201.tifabc.gif"',
            "filename*0*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66;"
                " filename*3*=abc.gif",
            [('filename', '201.tifabc.gif')],
            [errors.InvalidHeaderDefect]*2),

        # Here we depart from get_param and assume the *0* was missing.
        'duplicate_with_broken_split_value': (
            "filename=abc.gif; "
                " filename*2*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66",
            ' filename="abc.gif201.tif"',
            "filename=abc.gif;"
                " filename*2*=iso-8859-1''%32%30%31%2E; filename*3*=%74%69%66",
            [('filename', 'abc.gif201.tif')],
            # Defects are apparent missing *0*, and two 'out of sequence'.
            [errors.InvalidHeaderDefect]*3),

        # bpo-37461: Check that we don't go into an infinite loop.
        'extra_dquote': (
            'r*="\'a\'\\"',
            ' r="\\""',
            'r*=\'a\'"',
            [('r', '"')],
            [errors.InvalidHeaderDefect]*2),
    }

@parameterize
class Test_parse_mime_version(TestParserMixin, TestEmailBase):

    def mime_version_as_value(self,
                              value,
                              tl_str,
                              tl_value,
                              major,
                              minor,
                              defects):
        mime_version = self._test_parse_x(parser.parse_mime_version,
            value, tl_str, tl_value, defects)
        self.assertEqual(mime_version.major, major)
        self.assertEqual(mime_version.minor, minor)

    mime_version_params = {

        'rfc_2045_1': (
            '1.0',
            '1.0',
            '1.0',
            1,
            0,
            []),

        'RFC_2045_2': (
            '1.0 (produced by MetaSend Vx.x)',
            '1.0 (produced by MetaSend Vx.x)',
            '1.0 ',
            1,
            0,
            []),

        'RFC_2045_3': (
            '(produced by MetaSend Vx.x) 1.0',
            '(produced by MetaSend Vx.x) 1.0',
            ' 1.0',
            1,
            0,
            []),

        'RFC_2045_4': (
            '1.(produced by MetaSend Vx.x)0',
            '1.(produced by MetaSend Vx.x)0',
            '1. 0',
            1,
            0,
            []),

        'empty': (
            '',
            '',
            '',
            None,
            None,
            [errors.HeaderMissingRequiredValue]),

        }



class TestFolding(TestEmailBase):

    policy = policy.default

    def _test(self, tl, folded, policy=policy):
        self.assertEqual(tl.fold(policy=policy), folded, tl.ppstr())

    def test_simple_unstructured_no_folds(self):
        self._test(parser.get_unstructured("This is a test"),
                   "This is a test\n")

    def test_simple_unstructured_folded(self):
        self._test(parser.get_unstructured("This is also a test, but this "
                        "time there are enough words (and even some "
                        "symbols) to make it wrap; at least in theory."),
                   "This is also a test, but this time there are enough "
                        "words (and even some\n"
                   " symbols) to make it wrap; at least in theory.\n")

    def test_unstructured_with_unicode_no_folds(self):
        self._test(parser.get_unstructured("hübsch kleiner beißt"),
                   "=?utf-8?q?h=C3=BCbsch_kleiner_bei=C3=9Ft?=\n")

    def test_one_ew_on_each_of_two_wrapped_lines(self):
        self._test(parser.get_unstructured("Mein kleiner Kaktus ist sehr "
                                           "hübsch.  Es hat viele Stacheln "
                                           "und oft beißt mich."),
                   "Mein kleiner Kaktus ist sehr =?utf-8?q?h=C3=BCbsch=2E?=  "
                        "Es hat viele Stacheln\n"
                   " und oft =?utf-8?q?bei=C3=9Ft?= mich.\n")

    def test_ews_combined_before_wrap(self):
        self._test(parser.get_unstructured("Mein Kaktus ist hübsch.  "
                                           "Es beißt mich.  "
                                           "And that's all I'm sayin."),
                   "Mein Kaktus ist =?utf-8?q?h=C3=BCbsch=2E__Es_bei=C3=9Ft?= "
                        "mich.  And that's\n"
                   " all I'm sayin.\n")

    def test_unicode_after_unknown_not_combined(self):
        self._test(parser.get_unstructured("=?unknown-8bit?q?=A4?=\xa4"),
                   "=?unknown-8bit?q?=A4?==?utf-8?q?=C2=A4?=\n")
        prefix = "0123456789 "*5
        self._test(parser.get_unstructured(prefix + "=?unknown-8bit?q?=A4?=\xa4"),
                   prefix + "=?unknown-8bit?q?=A4?=\n =?utf-8?q?=C2=A4?=\n")

    def test_ascii_after_unknown_not_combined(self):
        self._test(parser.get_unstructured("=?unknown-8bit?q?=A4?=abc"),
                   "=?unknown-8bit?q?=A4?=abc\n")
        prefix = "0123456789 "*5
        self._test(parser.get_unstructured(prefix + "=?unknown-8bit?q?=A4?=abc"),
                   prefix + "=?unknown-8bit?q?=A4?=\n =?utf-8?q?abc?=\n")

    def test_unknown_after_unicode_not_combined(self):
        self._test(parser.get_unstructured("\xa4"
                                           "=?unknown-8bit?q?=A4?="),
                   "=?utf-8?q?=C2=A4?==?unknown-8bit?q?=A4?=\n")
        prefix = "0123456789 "*5
        self._test(parser.get_unstructured(prefix + "\xa4=?unknown-8bit?q?=A4?="),
                   prefix + "=?utf-8?q?=C2=A4?=\n =?unknown-8bit?q?=A4?=\n")

    def test_unknown_after_ascii_not_combined(self):
        self._test(parser.get_unstructured("abc"
                                           "=?unknown-8bit?q?=A4?="),
                   "abc=?unknown-8bit?q?=A4?=\n")
        prefix = "0123456789 "*5
        self._test(parser.get_unstructured(prefix + "abcd=?unknown-8bit?q?=A4?="),
                   prefix + "abcd\n =?unknown-8bit?q?=A4?=\n")

    def test_unknown_after_unknown(self):
        self._test(parser.get_unstructured("=?unknown-8bit?q?=C2?="
                                           "=?unknown-8bit?q?=A4?="),
                   "=?unknown-8bit?q?=C2=A4?=\n")
        prefix = "0123456789 "*5
        self._test(parser.get_unstructured(prefix + "=?unknown-8bit?q?=C2?="
                                           "=?unknown-8bit?q?=A4?="),
                   prefix + "=?unknown-8bit?q?=C2?=\n =?unknown-8bit?q?=A4?=\n")

    # XXX Need test of an encoded word so long that it needs to be wrapped

    def test_simple_address(self):
        self._test(parser.get_address_list("abc <xyz@example.com>")[0],
                   "abc <xyz@example.com>\n")

    def test_address_list_folding_at_commas(self):
        self._test(parser.get_address_list('abc <xyz@example.com>, '
                                            '"Fred Blunt" <sharp@example.com>, '
                                            '"J.P.Cool" <hot@example.com>, '
                                            '"K<>y" <key@example.com>, '
                                            'Firesale <cheap@example.com>, '
                                            '<end@example.com>')[0],
                    'abc <xyz@example.com>, "Fred Blunt" <sharp@example.com>,\n'
                    ' "J.P.Cool" <hot@example.com>, "K<>y" <key@example.com>,\n'
                    ' Firesale <cheap@example.com>, <end@example.com>\n')

    def test_address_list_with_unicode_names(self):
        self._test(parser.get_address_list(
            'Hübsch Kaktus <beautiful@example.com>, '
                'beißt beißt <biter@example.com>')[0],
            '=?utf-8?q?H=C3=BCbsch?= Kaktus <beautiful@example.com>,\n'
                ' =?utf-8?q?bei=C3=9Ft_bei=C3=9Ft?= <biter@example.com>\n')

    def test_address_list_with_unicode_names_in_quotes(self):
        self._test(parser.get_address_list(
            '"Hübsch Kaktus" <beautiful@example.com>, '
                '"beißt" beißt <biter@example.com>')[0],
            '=?utf-8?q?H=C3=BCbsch?= Kaktus <beautiful@example.com>,\n'
                ' =?utf-8?q?bei=C3=9Ft_bei=C3=9Ft?= <biter@example.com>\n')

    def test_address_list_with_specials_in_encoded_word(self):
        # An encoded-word parsed from a structured header must remain
        # encoded when it contains specials. Regression for gh-121284.
        policy = self.policy.clone(max_line_length=40)
        cases = [
            # (to, folded)
            ('=?utf-8?q?A_v=C3=A9ry_long_name_with=2C_comma?= <to@example.com>',
             'A =?utf-8?q?v=C3=A9ry_long_name_with?=\n'
             ' =?utf-8?q?=2C?= comma <to@example.com>\n'),
            ('=?utf-8?q?This_long_name_does_not_need_encoded=2Dword?= <to@example.com>',
             'This long name does not need\n'
             ' encoded-word <to@example.com>\n'),
            ('"A véry long name with, comma" <to@example.com>',
             # (This isn't the best fold point, but it's not invalid.)
             'A =?utf-8?q?v=C3=A9ry_long_name_with?=\n'
             ' =?utf-8?q?=2C?= comma <to@example.com>\n'),
            ('"A véry long name containing a, comma" <to@example.com>',
             'A =?utf-8?q?v=C3=A9ry?= long name\n'
             ' containing =?utf-8?q?a=2C?= comma\n'
             ' <to@example.com>\n'),
        ]
        for (to, folded) in cases:
            with self.subTest(to=to):
                self._test(parser.get_address_list(to)[0], folded, policy=policy)

    def test_address_list_with_list_separator_after_fold(self):
        a = 'x' * 66 + '@example.com'
        to = f'{a}, "Hübsch Kaktus" <beautiful@example.com>'
        self._test(parser.get_address_list(to)[0],
            f'{a},\n =?utf-8?q?H=C3=BCbsch?= Kaktus <beautiful@example.com>\n')

        a = '.' * 79  # ('.' is a special, so must be in quoted-string.)
        to = f'"{a}" <xyz@example.com>, "Hübsch Kaktus" <beautiful@example.com>'
        self._test(parser.get_address_list(to)[0],
            f'"{a}"\n'
            ' <xyz@example.com>, =?utf-8?q?H=C3=BCbsch?= Kaktus '
            '<beautiful@example.com>\n')

    def test_address_list_with_specials_in_long_quoted_string(self):
        # Regression for gh-80222.
        policy = self.policy.clone(max_line_length=40)
        cases = [
            # (to, folded)
            ('"Exfiltrator <spy@example.org> (unclosed comment?" <to@example.com>',
             '"Exfiltrator <spy@example.org> (unclosed\n'
             ' comment?" <to@example.com>\n'),
            ('"Escaped \\" chars \\\\ in quoted-string stay escaped" <to@example.com>',
             '"Escaped \\" chars \\\\ in quoted-string\n'
             ' stay escaped" <to@example.com>\n'),
            ('This long display name does not need quotes <to@example.com>',
             'This long display name does not need\n'
             ' quotes <to@example.com>\n'),
            ('"Quotes are not required but are retained here" <to@example.com>',
             '"Quotes are not required but are\n'
             ' retained here" <to@example.com>\n'),
            ('"A quoted-string, it can be a valid local-part"@example.com',
             '"A quoted-string, it can be a valid\n'
             ' local-part"@example.com\n'),
            ('"local-part-with-specials@but-no-fws.cannot-fold"@example.com',
             '"local-part-with-specials@but-no-fws.cannot-fold"@example.com\n'),
        ]
        for (to, folded) in cases:
            with self.subTest(to=to):
                self._test(parser.get_address_list(to)[0], folded, policy=policy)

    def test_address_list_with_long_unwrapable_comment(self):
        policy = self.policy.clone(max_line_length=40)
        cases = [
            # (to, folded)
            ('(loremipsumdolorsitametconsecteturadipi)<spy@example.org>',
             '(loremipsumdolorsitametconsecteturadipi)<spy@example.org>\n'),
            ('<spy@example.org>(loremipsumdolorsitametconsecteturadipi)',
             '<spy@example.org>(loremipsumdolorsitametconsecteturadipi)\n'),
            ('(loremipsum dolorsitametconsecteturadipi)<spy@example.org>',
             '(loremipsum dolorsitametconsecteturadipi)<spy@example.org>\n'),
             ('<spy@example.org>(loremipsum dolorsitametconsecteturadipi)',
             '<spy@example.org>(loremipsum\n dolorsitametconsecteturadipi)\n'),
            ('(Escaped \\( \\) chars \\\\ in comments stay escaped)<spy@example.org>',
             '(Escaped \\( \\) chars \\\\ in comments stay\n escaped)<spy@example.org>\n'),
            ('((loremipsum)(loremipsum)(loremipsum)(loremipsum))<spy@example.org>',
             '((loremipsum)(loremipsum)(loremipsum)(loremipsum))<spy@example.org>\n'),
            ('((loremipsum)(loremipsum)(loremipsum) (loremipsum))<spy@example.org>',
             '((loremipsum)(loremipsum)(loremipsum)\n (loremipsum))<spy@example.org>\n'),
        ]
        for (to, folded) in cases:
            with self.subTest(to=to):
                self._test(parser.get_address_list(to)[0], folded, policy=policy)

    # XXX Need tests with comments on various sides of a unicode token,
    # and with unicode tokens in the comments.  Spaces inside the quotes
    # currently don't do the right thing.

    def test_split_at_whitespace_after_header_before_long_token(self):
        body = parser.get_unstructured('   ' + 'x'*77)
        header = parser.Header([
            parser.HeaderLabel([parser.ValueTerminal('test:', 'atext')]),
            parser.CFWSList([parser.WhiteSpaceTerminal(' ', 'fws')]), body])
        self._test(header, 'test:   \n ' + 'x'*77 + '\n')

    def test_split_at_whitespace_before_long_token(self):
        self._test(parser.get_unstructured('xxx   ' + 'y'*77),
                   'xxx  \n ' + 'y'*77 + '\n')

    def test_overlong_encodeable_is_wrapped(self):
        first_token_with_whitespace = 'xxx   '
        chrome_leader = '=?utf-8?q?'
        len_chrome = len(chrome_leader) + 2
        len_non_y = len_chrome + len(first_token_with_whitespace)
        self._test(parser.get_unstructured(first_token_with_whitespace +
                                           'y'*80),
                   first_token_with_whitespace + chrome_leader +
                       'y'*(78-len_non_y) + '?=\n' +
                       ' ' + chrome_leader + 'y'*(80-(78-len_non_y)) + '?=\n')

    def test_long_filename_attachment(self):
        self._test(parser.parse_content_disposition_header(
            'attachment; filename="TEST_TEST_TEST_TEST'
                '_TEST_TEST_TEST_TEST_TEST_TEST_TEST_TEST_TES.txt"'),
            "attachment;\n"
            " filename*0*=us-ascii''TEST_TEST_TEST_TEST_TEST_TEST"
                "_TEST_TEST_TEST_TEST_TEST;\n"
            " filename*1*=_TEST_TES.txt\n",
            )

    def test_fold_unfoldable_element_stealing_whitespace(self):
        # gh-142006: When an element is too long to fit on the current line
        # the previous line's trailing whitespace should not trigger a double newline.
        policy = self.policy.clone(max_line_length=10)
        # The non-whitespace text needs to exactly fill the max_line_length (10).
        text = ("a" * 9) + ", " + ("b" * 20)
        expected = ("a" * 9) + ",\n " + ("b" * 20) + "\n"
        token = parser.get_address_list(text)[0]
        self._test(token, expected, policy=policy)

    def test_encoded_word_with_undecodable_bytes(self):
        self._test(
            parser.get_address_list(
                ' =?utf-8?Q?=E5=AE=A2=E6=88=B6=E6=AD=A3=E8=A6=8F=E4=BA=A4=E7?='
                ' <xyz@abc.com>'
                )[0],
            ' =?unknown-8bit?b?5a6i5oi25q2j6KaP5Lqk5w==?= <xyz@abc.com>\n',
            )


if __name__ == '__main__':
    unittest.main()
