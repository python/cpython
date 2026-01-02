import unittest
from email import _encoded_words as _ew
from email import errors
from test.test_email import TestEmailBase
from test.test_email.params import C, params, Params


class TestDecodeQ(TestEmailBase):

    @params
    def test(self, source, ex_result, ex_defects=[]):
        result, defects = _ew.decode_q(source)
        self.assertEqual(result, ex_result)
        self.assertDefectsEqual(defects, ex_defects)

    params_test = Params(

        test_no_encoded = C(
                   b'foobar', b'foobar'),

        test_encoded_spaces = C(
                   b'foo=20bar=20', b'foo bar '),

        test_underline_space = C(
                   b'foo_bar_', b'foo bar '),

        test_run_of_encoded = C(
                   b'foo=20=20=21=2Cbar', b'foo  !,bar'),

        )


class TestDecodeB(TestEmailBase):

    @params
    def test(self, source, ex_result, ex_defects=[]):
        result, defects = _ew.decode_b(source)
        self.assertEqual(result, ex_result)
        self.assertDefectsEqual(defects, ex_defects)

    params_test = Params(

        test_simple = C(
                   b'Zm9v', b'foo'),

        test_missing_1_padding_char = C(
                   b'dmk', b'vi', [errors.InvalidBase64PaddingDefect]),

        test_missing_2_padding_char = C(
                   b'dg', b'v', [errors.InvalidBase64PaddingDefect]),

        test_invalid_character = C(
                   b'dm\x01k===', b'vi', [errors.InvalidBase64CharactersDefect]),

        test_invalid_character_and_bad_padding = C(
                   b'dm\x01k', b'vi', [errors.InvalidBase64CharactersDefect,
                                       errors.InvalidBase64PaddingDefect]),

        test_invalid_length = C(
                   b'abcde', b'abcde', [errors.InvalidBase64LengthDefect]),

        )


class TestDecode(TestEmailBase):

    @params
    def test_raises_if(self, value, exception=ValueError):
        with self.assertRaises(exception):
            _ew.decode(value)

    params_test_raises_if = Params(

        missing_middle = C(
                       '=?badone?='),

        beginning_only = C(
                       '=?'),

        empty_string = C(
                       ''),

        invalid_encoding = C(
                       '=?utf-8?X?somevalue?=', exception=KeyError),

        )


    @params
    def test(self, source, result, charset='us-ascii', lang='', defects=[]):
        res, char, l, d = _ew.decode(source)
        self.assertEqual(res, result)
        self.assertEqual(char, charset)
        self.assertEqual(l, lang)
        self.assertDefectsEqual(d, defects)

    params_test = Params(

        test_simple_q = C(
                   '=?us-ascii?q?foo?=', 'foo'),

        test_simple_b = C(
                   '=?us-ascii?b?dmk=?=', 'vi'),

        test_q_case_ignored = C(
                   '=?us-ascii?Q?foo?=', 'foo'),

        test_b_case_ignored = C(
                   '=?us-ascii?B?dmk=?=', 'vi'),

        test_non_trivial_q = C(
                   '=?latin-1?q?=20F=fcr=20Elise=20?=', ' Für Elise ', 'latin-1'),

        test_q_escaped_bytes_preserved = C(
                   b'=?us-ascii?q?=20\xACfoo?='.decode('us-ascii',
                                                       'surrogateescape'),
                   ' \uDCACfoo',
                   defects = [errors.UndecodableBytesDefect])
                   ,

        test_b_undecodable_bytes_ignored_with_defect = C(
                   b'=?us-ascii?b?dm\xACk?='.decode('us-ascii',
                                                   'surrogateescape'),
                   'vi',
                   defects = [
                    errors.InvalidBase64CharactersDefect,
                    errors.InvalidBase64PaddingDefect])
                    ,

        test_b_invalid_bytes_ignored_with_defect = C(
                   '=?us-ascii?b?dm\x01k===?=',
                   'vi',
                   defects = [errors.InvalidBase64CharactersDefect])
                   ,

        test_b_invalid_bytes_incorrect_padding = C(
                   '=?us-ascii?b?dm\x01k?=',
                   'vi',
                   defects = [
                    errors.InvalidBase64CharactersDefect,
                    errors.InvalidBase64PaddingDefect])
                    ,

        test_b_padding_defect = C(
                   '=?us-ascii?b?dmk?=',
                   'vi',
                    defects = [errors.InvalidBase64PaddingDefect])
                    ,

        test_nonnull_lang = C(
                   '=?us-ascii*jive?q?test?=', 'test', lang='jive'),

        test_unknown_8bit_charset = C(
                   '=?unknown-8bit?q?foo=ACbar?=',
                   b'foo\xacbar'.decode('ascii', 'surrogateescape'),
                   charset = 'unknown-8bit',
                   defects = [])
                   ,

        test_unknown_charset = C(
                   '=?foobar?q?foo=ACbar?=',
                   b'foo\xacbar'.decode('ascii', 'surrogateescape'),
                   charset = 'foobar',
                   # XXX Should this be a new Defect instead?
                   defects = [errors.CharsetError])
                   ,

        test_invalid_character_in_charset = C(
                   '=?utf-8\udce2\udc80\udc9d?q?foo=ACbar?=',
                   b'foo\xacbar'.decode('ascii', 'surrogateescape'),
                   charset = 'utf-8\udce2\udc80\udc9d',
                   # XXX Should this be a new Defect instead?
                   defects = [errors.CharsetError])
                   ,

        test_q_nonascii = C(
                   '=?utf-8?q?=C3=89ric?=',
                   'Éric',
                   charset='utf-8')
                   ,

        )


class TestEncodeQ(TestEmailBase):

    @params
    def test(self, src, expected):
        self.assertEqual(_ew.encode_q(src), expected)

    params_test = Params(

        test_all_safe = C(
                   b'foobar', 'foobar'),

        test_spaces = C(
                   b'foo bar ', 'foo_bar_'),

        test_run_of_encodables = C(
                   b'foo  ,,bar', 'foo__=2C=2Cbar'),

        )


class TestEncodeB(TestEmailBase):

    @params
    def test(self, src, expected):
        self.assertEqual(_ew.encode_b(src), expected)

    params_test = Params(

        test_simple = C(
                                      b'foo',  'Zm9v'),

        test_padding = C(
                                      b'vi',  'dmk='),

        )


class TestEncode(TestEmailBase):

    @params
    def test(self, callspec, expected):
        self.assertEqual(callspec(_ew.encode), expected)

    params_test = Params(

        test_q = C(
                                  C('foo', 'utf-8', 'q'), '=?utf-8?q?foo?='),

        test_b = C(
                                  C('foo', 'utf-8', 'b'), '=?utf-8?b?Zm9v?='),

        test_auto_q = C(
                                  C('foo', 'utf-8'), '=?utf-8?q?foo?='),

        test_auto_q_if_short_mostly_safe = C(
                                  C('vi.', 'utf-8'), '=?utf-8?q?vi=2E?='),

        test_auto_b_if_enough_unsafe = C(
                                  C('.....', 'utf-8'), '=?utf-8?b?Li4uLi4=?='),

        test_auto_b_if_long_unsafe = C(
                                  C('vi.vi.vi.vi.vi.', 'utf-8'),
                         '=?utf-8?b?dmkudmkudmkudmkudmku?=')
                         ,

        test_auto_q_if_long_mostly_safe = C(
                                  C('vi vi vi.vi ', 'utf-8'),
                         '=?utf-8?q?vi_vi_vi=2Evi_?=')
                         ,

        test_utf8_default = C(
                                  C('foo'), '=?utf-8?q?foo?='),

        test_lang = C(
                                  C('foo', lang='jive'), '=?utf-8*jive?q?foo?='),

        test_unknown_8bit = C(
                                  C('foo\uDCACbar', charset='unknown-8bit'),
                         '=?unknown-8bit?q?foo=ACbar?=')
                         ,

        )


if __name__ == '__main__':
    unittest.main()
