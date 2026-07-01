import unittest
from email import _encoded_words as _ew
from email import errors
from test.test_email import TestEmailBase
from test.test_email.params import C, params, Params


class TestDecoders(TestEmailBase):

    def _test(self, function, source, result, defects=[]):
        actual_result, actual_defects = function(source)
        self.assertEqual(actual_result, result)
        self.assertDefectsEqual(actual_defects, defects)


    @params
    def test_decode_q(self, *args, **kw):
        return self._test(_ew.decode_q, *args, **kw)

    params_test_decode_q = Params(
        no_encoded = C(b'foobar', b'foobar'),
        encoded_spaces = C(b'foo=20bar=20', b'foo bar '),
        underline_space = C(b'foo_bar_', b'foo bar '),
        run_of_encoded = C(b'foo=20=20=21=2Cbar', b'foo  !,bar'),
        )


    @params
    def test_decode_b(self, *args, **kw):
        return self._test(_ew.decode_b, *args, **kw)

    params_test_decode_b = Params(

        simple = C(
            b'Zm9v',
            b'foo',
            ),

        missing_1_padding_char = C(
            b'dmk',
            b'vi',
            defects=[errors.InvalidBase64PaddingDefect],
            ),

        missing_2_padding_char = C(
            b'dg',
            b'v',
            defects=[errors.InvalidBase64PaddingDefect],
            ),

        invalid_character = C(
            b'dm\x01k===',
            b'vi',
            defects=[errors.InvalidBase64CharactersDefect],
            ),

        invalid_character_and_bad_padding = C(
            b'dm\x01k',
            b'vi',
            defects=[
                errors.InvalidBase64CharactersDefect,
                errors.InvalidBase64PaddingDefect,
                ],
            ),

        invalid_length = C(
            b'abcde',
            b'abcde',
            defects=[errors.InvalidBase64LengthDefect],
            ),

        )


    @params
    def test_decode_raises_if_value(self, value, exception=ValueError):
        with self.assertRaises(exception):
            _ew.decode(value)

    params_test_decode_raises_if_value = Params(
        missing_middle = C('=?badone?='),
        beginning_only = C('=?'),
        empty_string = C(''),
        invalid_encoding = C('=?utf-8?X?somevalue?=', exception=KeyError),
        )


    @params
    def test__decode(
            self,
            cte_encoded,
            *,
            cte,
            charset='us-ascii',
            result,
            defects=[],
        ):
        actual, actual_defects = _ew._decode(charset, cte, cte_encoded)
        self.assertEqual(actual, result)
        self.assertDefectsEqual(actual_defects, defects)

    @params
    def test_decode(
            self,
            cte_encoded,
            *,
            cte,
            charset='us-ascii',
            lang='',
            result,
            defects=[],
        ):
        ew = f'=?{charset}{lang and '*'}{lang}?{cte}?{cte_encoded}?='
        actual, actual_charset, actual_lang, actual_defects = _ew.decode(ew)
        self.assertEqual(actual, result)
        self.assertEqual(actual_charset, charset)
        self.assertEqual(actual_lang, lang)
        self.assertDefectsEqual(actual_defects, defects)

    params_test__decode = params_test_decode = Params(

        simple_q = C(
            'foo',
            cte='q',
            result='foo',
            ),

        simple_b = C(
            'dmk=',
            cte='b',
            result='vi',
            ),

        q_case_ignored = C(
            'foo',
            cte='Q',
            result='foo',
            ),

        b_case_ignored = C(
            'dmk=',
            cte='B',
            result='vi',
            ),

        non_trivial_q = C(
            '=20F=fcr=20Elise=20',
            cte='q',
            result=' Für Elise ',
            charset='latin-1',
            ),

        q_escaped_bytes_preserved = C(
            b'=20\xACfoo'.decode('us-ascii', 'surrogateescape'),
            cte='q',
            result=' \uDCACfoo',
            defects=[errors.UndecodableBytesDefect],
            ),

        b_undecodable_bytes_ignored_with_defect = C(
            b'dm\xACk'.decode('us-ascii', 'surrogateescape'),
            cte='b',
            result='vi',
            defects=[
                errors.InvalidBase64CharactersDefect,
                errors.InvalidBase64PaddingDefect,
                ],
            ),

        b_invalid_bytes_ignored_with_defect = C(
            'dm\x01k===',
            cte='b',
            result='vi',
            defects=[errors.InvalidBase64CharactersDefect],
            ),

        b_invalid_bytes_incorrect_padding = C(
            'dm\x01k',
            cte='b',
            result='vi',
            defects=[
                errors.InvalidBase64CharactersDefect,
                errors.InvalidBase64PaddingDefect,
                ],
            ),

        b_padding_defect = C(
            'dmk',
            cte='b',
            result='vi',
            defects=[errors.InvalidBase64PaddingDefect],
            ),

        unknown_8bit_charset = C(
            'foo=ACbar',
            cte='q',
            result=b'foo\xacbar'.decode('ascii', 'surrogateescape'),
            charset='unknown-8bit',
            defects=[],
            ),

        unknown_charset = C(
            'foo=ACbar',
            cte='q',
            result=b'foo\xacbar'.decode('ascii', 'surrogateescape'),
            charset='foobar',
            # XXX Should this be a new Defect instead?
            defects=[errors.CharsetError],
            ),

        invalid_character_in_charset = C(
            'foo=ACbar',
            cte='q',
            result=b'foo\xacbar'.decode('ascii', 'surrogateescape'),
            charset='utf-8\udce2\udc80\udc9d',
            # XXX Should this be a new Defect instead?
            defects=[errors.CharsetError],
            ),

        q_nonascii = C(
            '=C3=89ric',
            cte='q',
            result='Éric',
            charset='utf-8',
            ),

        )

    params_test_decode__lang = Params(

        nonnull_lang = C(
            'test',
            cte='q',
            result='test',
            lang='jive',
            ),

        )


class TestEncoders(TestEmailBase):

    def _test(self, function, source, expected):
        self.assertEqual(function(source), expected)

    @params(
        all_safe = C(b'foobar', 'foobar'),
        spaces = C(b'foo bar ', 'foo_bar_'),
        run_of_encodables = C(b'foo  ,,bar', 'foo__=2C=2Cbar'),
        )
    def test_encode_q(self, *args, **kw):
        return self._test(_ew.encode_q, *args, **kw)


    @params(
        simple = C(b'foo',  'Zm9v'),
        padding = C(b'vi',  'dmk='),
        )
    def test_encode_b(self, *args, **kw):
        return self._test(_ew.encode_b, *args, **kw)


    @params
    def test_encode(self, callspec, expected):
        self.assertEqual(callspec(_ew.encode), expected)

    params_test_encode = Params(

        q = C(
            C('foo', 'utf-8', 'q'),
            '=?utf-8?q?foo?=',
            ),

        b = C(
            C('foo', 'utf-8', 'b'),
            '=?utf-8?b?Zm9v?=',
            ),

        auto_q = C(
            C('foo', 'utf-8'),
            '=?utf-8?q?foo?=',
            ),

        auto_q_if_short_mostly_safe = C(
            C('vi.', 'utf-8'),
            '=?utf-8?q?vi=2E?=',
            ),

        auto_b_if_enough_unsafe = C(
            C('.....', 'utf-8'),
            '=?utf-8?b?Li4uLi4=?=',
            ),

        auto_b_if_long_unsafe = C(
            C('vi.vi.vi.vi.vi.', 'utf-8'),
            '=?utf-8?b?dmkudmkudmkudmkudmku?=',
            ),

        auto_q_if_long_mostly_safe = C(
            C('vi vi vi.vi ', 'utf-8'),
            '=?utf-8?q?vi_vi_vi=2Evi_?=',
            ),

        utf8_default = C(
            C('foo'),
            '=?utf-8?q?foo?=',
            ),

        lang = C(
            C('foo', lang='jive'),
            '=?utf-8*jive?q?foo?=',
            ),

        unknown_8bit = C(
            C('foo\uDCACbar', charset='unknown-8bit'),
            '=?unknown-8bit?q?foo=ACbar?=',
            ),

        )


if __name__ == '__main__':
    unittest.main()
