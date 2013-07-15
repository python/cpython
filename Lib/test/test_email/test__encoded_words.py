import unittest
from email import _encoded_words as _ew
from email import errors
from test.test_email import TestEmailBase


class TestDecodeQ(TestEmailBase):

    def _test(self, source, ex_result, ex_defects=[]):
        result, defects = _ew.decode_q(source)
        self.assertEqual(result, ex_result)
        self.assertDefectsEqual(defects, ex_defects)

    def test_no_encoded(self):
        self._test(b'foobar', b'foobar')

    def test_spaces(self):
        self._test(b'foo=20bar=20', b'foo bar ')
        self._test(b'foo_bar_', b'foo bar ')

    def test_run_of_encoded(self):
        self._test(b'foo=20=20=21=2Cbar', b'foo  !,bar')


class TestDecodeB(TestEmailBase):

    def _test(self, source, ex_result, ex_defects=[]):
        result, defects = _ew.decode_b(source)
        self.assertEqual(result, ex_result)
        self.assertDefectsEqual(defects, ex_defects)

    def test_simple(self):
        self._test(b'Zm9v', b'foo')

    def test_missing_padding(self):
        self._test(b'dmk', b'vi', [errors.InvalidBase64PaddingDefect])

    def test_invalid_character(self):
        self._test(b'dm\x01k===', b'vi', [errors.InvalidBase64CharactersDefect])

    def test_invalid_character_and_bad_padding(self):
        self._test(b'dm\x01k', b'vi', [errors.InvalidBase64CharactersDefect,
                                       errors.InvalidBase64PaddingDefect])


class TestDecode(TestEmailBase):

    def test_wrong_format_input_raises(self):
        with self.assertRaises(ValueError):
            _ew.decode('=?badone?=')
        with self.assertRaises(ValueError):
            _ew.decode('=?')
        with self.assertRaises(ValueError):
            _ew.decode('')

    def _test(self, source, result, charset='us-ascii', lang='', defects=[]):
        res, char, l, d = _ew.decode(source)
        self.assertEqual(res, result)
        self.assertEqual(char, charset)
        self.assertEqual(l, lang)
        self.assertDefectsEqual(d, defects)

    def test_simple_q(self):
        self._test('=?us-ascii?q?foo?=', 'foo')

    def test_simple_b(self):
        self._test('=?us-ascii?b?dmk=?=', 'vi')

    def test_q_case_ignored(self):
        self._test('=?us-ascii?Q?foo?=', 'foo')

    def test_b_case_ignored(self):
        self._test('=?us-ascii?B?dmk=?=', 'vi')

    def test_non_trivial_q(self):
        self._test('=?latin-1?q?=20F=fcr=20Elise=20?=', ' Für Elise ', 'latin-1')

    def test_q_escaped_bytes_preserved(self):
        self._test(b'=?us-ascii?q?=20\xACfoo?='.decode('us-ascii',
                                                       'surrogateescape'),
                   ' \uDCACfoo',
                   defects = [errors.UndecodableBytesDefect])

    def test_b_undecodable_bytes_ignored_with_defect(self):
        self._test(b'=?us-ascii?b?dm\xACk?='.decode('us-ascii',
                                                   'surrogateescape'),
                   'vi',
                   defects = [
                    errors.InvalidBase64CharactersDefect,
                    errors.InvalidBase64PaddingDefect])

    def test_b_invalid_bytes_ignored_with_defect(self):
        self._test('=?us-ascii?b?dm\x01k===?=',
                   'vi',
                   defects = [errors.InvalidBase64CharactersDefect])

    def test_b_invalid_bytes_incorrect_padding(self):
        self._test('=?us-ascii?b?dm\x01k?=',
                   'vi',
                   defects = [
                    errors.InvalidBase64CharactersDefect,
                    errors.InvalidBase64PaddingDefect])

    def test_b_padding_defect(self):
        self._test('=?us-ascii?b?dmk?=',
                   'vi',
                    defects = [errors.InvalidBase64PaddingDefect])

    def test_nonnull_lang(self):
        self._test('=?us-ascii*jive?q?test?=', 'test', lang='jive')

    def test_unknown_8bit_charset(self):
        self._test('=?unknown-8bit?q?foo=ACbar?=',
                   b'foo\xacbar'.decode('ascii', 'surrogateescape'),
                   charset = 'unknown-8bit',
                   defects = [])

    def test_unknown_charset(self):
        self._test('=?foobar?q?foo=ACbar?=',
                   b'foo\xacbar'.decode('ascii', 'surrogateescape'),
                   charset = 'foobar',
                   # XXX Should this be a new Defect instead?
                   defects = [errors.CharsetError])

    def test_q_nonascii(self):
        self._test('=?utf-8?q?=C3=89ric?=',
                   'Éric',
                   charset='utf-8')


class TestEncodeQ(TestEmailBase):

    def _test(self, src, expected):
        self.assertEqual(_ew.encode_q(src), expected)

    def test_all_safe(self):
        self._test(b'foobar', 'foobar')

    def test_spaces(self):
        self._test(b'foo bar ', 'foo_bar_')

    def test_run_of_encodables(self):
        self._test(b'foo  ,,bar', 'foo__=2C=2Cbar')


class TestEncodeB(TestEmailBase):

    def test_simple(self):
        self.assertEqual(_ew.encode_b(b'foo'), 'Zm9v')

    def test_padding(self):
        self.assertEqual(_ew.encode_b(b'vi'), 'dmk=')


class TestEncode(TestEmailBase):

    def test_q(self):
        self.assertEqual(_ew.encode('foo', 'utf-8', 'q'), '=?utf-8?q?foo?=')

    def test_b(self):
        self.assertEqual(_ew.encode('foo', 'utf-8', 'b'), '=?utf-8?b?Zm9v?=')

    def test_auto_q(self):
        self.assertEqual(_ew.encode('foo', 'utf-8'), '=?utf-8?q?foo?=')

    def test_auto_q_if_short_mostly_safe(self):
        self.assertEqual(_ew.encode('vi.', 'utf-8'), '=?utf-8?q?vi=2E?=')

    def test_auto_b_if_enough_unsafe(self):
        self.assertEqual(_ew.encode('.....', 'utf-8'), '=?utf-8?b?Li4uLi4=?=')

    def test_auto_b_if_long_unsafe(self):
        self.assertEqual(_ew.encode('vi.vi.vi.vi.vi.', 'utf-8'),
                         '=?utf-8?b?dmkudmkudmkudmkudmku?=')

    def test_auto_q_if_long_mostly_safe(self):
        self.assertEqual(_ew.encode('vi vi vi.vi ', 'utf-8'),
                         '=?utf-8?q?vi_vi_vi=2Evi_?=')

    def test_utf8_default(self):
        self.assertEqual(_ew.encode('foo'), '=?utf-8?q?foo?=')

    def test_lang(self):
        self.assertEqual(_ew.encode('foo', lang='jive'), '=?utf-8*jive?q?foo?=')

    def test_unknown_8bit(self):
        self.assertEqual(_ew.encode('foo\uDCACbar', charset='unknown-8bit'),
                         '=?unknown-8bit?q?foo=ACbar?=')


if __name__ == '__main__':
    unittest.main()
