import string
import unittest
from email import _header_value_parser as parser
from email import errors
from email import policy
from test.test_email import TestEmailBase, parameterize

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


class TestParser(TestParserMixin, TestEmailBase):

    # _wsp_splitter

    rfc_printable_ascii = bytes(range(33, 127)).decode('ascii')
    rfc_atext_chars = (string.ascii_letters + string.digits +
                        "!#$%&\'*+-/=?^_`{}|~")
    rfc_dtext_chars = rfc_printable_ascii.translate(str.maketrans('','',r'\[]'))

    def test__wsp_splitter_one_word(self):
        self.assertEqual(parser._wsp_splitter('foo', 1), ['foo'])

    def test__wsp_splitter_two_words(self):
        self.assertEqual(parser._wsp_splitter('foo def', 1),
                                               ['foo', ' ', 'def'])

    def test__wsp_splitter_ws_runs(self):
        self.assertEqual(parser._wsp_splitter('foo \t def jik', 1),
                                              ['foo', ' \t ', 'def jik'])


    # get_fws

    def test_get_fws_only(self):
        fws = self._test_get_x(parser.get_fws, ' \t  ', ' \t  ', ' ', [], '')
        self.assertEqual(fws.token_type, 'fws')

    def test_get_fws_space(self):
        self._test_get_x(parser.get_fws, ' foo', ' ', ' ', [], 'foo')

    def test_get_fws_ws_run(self):
        self._test_get_x(parser.get_fws, ' \t foo ', ' \t ', ' ', [], 'foo ')

    # get_encoded_word

    def test_get_encoded_word_missing_start_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_encoded_word('abc')

    def test_get_encoded_word_missing_end_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_encoded_word('=?abc')

    def test_get_encoded_word_missing_middle_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_encoded_word('=?abc?=')

    def test_get_encoded_word_invalid_cte(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_encoded_word('=?utf-8?X?somevalue?=')

    def test_get_encoded_word_valid_ew(self):
        self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?this_is_a_test?=  bird',
                         'this is a test',
                         'this is a test',
                         [],
                         '  bird')

    def test_get_encoded_word_internal_spaces(self):
        self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?this is a test?=  bird',
                         'this is a test',
                         'this is a test',
                         [errors.InvalidHeaderDefect],
                         '  bird')

    def test_get_encoded_word_gets_first(self):
        self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?first?=  =?utf-8?q?second?=',
                         'first',
                         'first',
                         [],
                         '  =?utf-8?q?second?=')

    def test_get_encoded_word_gets_first_even_if_no_space(self):
        self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?first?==?utf-8?q?second?=',
                         'first',
                         'first',
                         [errors.InvalidHeaderDefect],
                         '=?utf-8?q?second?=')

    def test_get_encoded_word_sets_extra_attributes(self):
        ew = self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii*jive?q?first_second?=',
                         'first second',
                         'first second',
                         [],
                         '')
        self.assertEqual(ew.charset, 'us-ascii')
        self.assertEqual(ew.lang, 'jive')

    def test_get_encoded_word_lang_default_is_blank(self):
        ew = self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?first_second?=',
                         'first second',
                         'first second',
                         [],
                         '')
        self.assertEqual(ew.charset, 'us-ascii')
        self.assertEqual(ew.lang, '')

    def test_get_encoded_word_non_printable_defect(self):
        self._test_get_x(parser.get_encoded_word,
                         '=?us-ascii?q?first\x02second?=',
                         'first\x02second',
                         'first\x02second',
                         [errors.NonPrintableDefect],
                         '')

    def test_get_encoded_word_leading_internal_space(self):
        self._test_get_x(parser.get_encoded_word,
                        '=?us-ascii?q?=20foo?=',
                        ' foo',
                        ' foo',
                        [],
                        '')

    def test_get_encoded_word_quopri_utf_escape_follows_cte(self):
        # Issue 18044
        self._test_get_x(parser.get_encoded_word,
                        '=?utf-8?q?=C3=89ric?=',
                        'Éric',
                        'Éric',
                        [],
                        '')

    # get_unstructured

    def _get_unst(self, value):
        token = parser.get_unstructured(value)
        return token, ''

    def test_get_unstructured_null(self):
        self._test_get_x(self._get_unst, '', '', '', [], '')

    def test_get_unstructured_one_word(self):
        self._test_get_x(self._get_unst, 'foo', 'foo', 'foo', [], '')

    def test_get_unstructured_normal_phrase(self):
        self._test_get_x(self._get_unst, 'foo bar bird',
                                         'foo bar bird',
                                         'foo bar bird',
                                         [],
                                         '')

    def test_get_unstructured_normal_phrase_with_whitespace(self):
        self._test_get_x(self._get_unst, 'foo \t bar      bird',
                                         'foo \t bar      bird',
                                         'foo bar bird',
                                         [],
                                         '')

    def test_get_unstructured_leading_whitespace(self):
        self._test_get_x(self._get_unst, '  foo bar',
                                         '  foo bar',
                                         ' foo bar',
                                         [],
                                         '')

    def test_get_unstructured_trailing_whitespace(self):
        self._test_get_x(self._get_unst, 'foo bar  ',
                                         'foo bar  ',
                                         'foo bar ',
                                         [],
                                         '')

    def test_get_unstructured_leading_and_trailing_whitespace(self):
        self._test_get_x(self._get_unst, '  foo bar  ',
                                         '  foo bar  ',
                                         ' foo bar ',
                                         [],
                                         '')

    def test_get_unstructured_one_valid_ew_no_ws(self):
        self._test_get_x(self._get_unst, '=?us-ascii?q?bar?=',
                                         'bar',
                                         'bar',
                                         [],
                                         '')

    def test_get_unstructured_one_ew_trailing_ws(self):
        self._test_get_x(self._get_unst, '=?us-ascii?q?bar?=  ',
                                         'bar  ',
                                         'bar ',
                                         [],
                                         '')

    def test_get_unstructured_one_valid_ew_trailing_text(self):
        self._test_get_x(self._get_unst, '=?us-ascii?q?bar?= bird',
                                         'bar bird',
                                         'bar bird',
                                         [],
                                         '')

    def test_get_unstructured_phrase_with_ew_in_middle_of_text(self):
        self._test_get_x(self._get_unst, 'foo =?us-ascii?q?bar?= bird',
                                         'foo bar bird',
                                         'foo bar bird',
                                         [],
                                         '')

    def test_get_unstructured_phrase_with_two_ew(self):
        self._test_get_x(self._get_unst,
            'foo =?us-ascii?q?bar?= =?us-ascii?q?bird?=',
            'foo barbird',
            'foo barbird',
            [],
            '')

    def test_get_unstructured_phrase_with_two_ew_trailing_ws(self):
        self._test_get_x(self._get_unst,
            'foo =?us-ascii?q?bar?= =?us-ascii?q?bird?=   ',
            'foo barbird   ',
            'foo barbird ',
            [],
            '')

    def test_get_unstructured_phrase_with_ew_with_leading_ws(self):
        self._test_get_x(self._get_unst,
            '  =?us-ascii?q?bar?=',
            '  bar',
            ' bar',
            [],
            '')

    def test_get_unstructured_phrase_with_two_ew_extra_ws(self):
        self._test_get_x(self._get_unst,
            'foo =?us-ascii?q?bar?= \t  =?us-ascii?q?bird?=',
            'foo barbird',
            'foo barbird',
            [],
            '')

    def test_get_unstructured_two_ew_extra_ws_trailing_text(self):
        self._test_get_x(self._get_unst,
            '=?us-ascii?q?test?=   =?us-ascii?q?foo?=  val',
            'testfoo  val',
            'testfoo val',
            [],
            '')

    def test_get_unstructured_ew_with_internal_ws(self):
        self._test_get_x(self._get_unst,
            '=?iso-8859-1?q?hello=20world?=',
            'hello world',
            'hello world',
            [],
            '')

    def test_get_unstructured_ew_with_internal_leading_ws(self):
        self._test_get_x(self._get_unst,
            '   =?us-ascii?q?=20test?=   =?us-ascii?q?=20foo?=  val',
            '    test foo  val',
            '  test foo val',
            [],
            '')

    def test_get_unstructured_invalid_ew(self):
        self._test_get_x(self._get_unst,
            '=?test val',
            '=?test val',
            '=?test val',
            [],
            '')

    def test_get_unstructured_undecodable_bytes(self):
        self._test_get_x(self._get_unst,
            b'test \xACfoo  val'.decode('ascii', 'surrogateescape'),
            'test \uDCACfoo  val',
            'test \uDCACfoo val',
            [errors.UndecodableBytesDefect],
            '')

    def test_get_unstructured_undecodable_bytes_in_EW(self):
        self._test_get_x(self._get_unst,
            (b'=?us-ascii?q?=20test?=   =?us-ascii?q?=20\xACfoo?='
                b'  val').decode('ascii', 'surrogateescape'),
            ' test \uDCACfoo  val',
            ' test \uDCACfoo val',
            [errors.UndecodableBytesDefect]*2,
            '')

    def test_get_unstructured_missing_base64_padding(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?b?dmk?=',
            'vi',
            'vi',
            [errors.InvalidBase64PaddingDefect],
            '')

    def test_get_unstructured_invalid_base64_character(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?b?dm\x01k===?=',
            'vi',
            'vi',
            [errors.InvalidBase64CharactersDefect],
            '')

    def test_get_unstructured_invalid_base64_character_and_bad_padding(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?b?dm\x01k?=',
            'vi',
            'vi',
            [errors.InvalidBase64CharactersDefect,
             errors.InvalidBase64PaddingDefect],
            '')

    def test_get_unstructured_invalid_base64_length(self):
        # bpo-27397: Return the encoded string since there's no way to decode.
        self._test_get_x(self._get_unst,
            '=?utf-8?b?abcde?=',
            'abcde',
            'abcde',
            [errors.InvalidBase64LengthDefect],
            '')

    def test_get_unstructured_no_whitespace_between_ews(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?q?foo?==?utf-8?q?bar?=',
            'foobar',
            'foobar',
            [errors.InvalidHeaderDefect,
            errors.InvalidHeaderDefect],
            '')

    def test_get_unstructured_ew_without_leading_whitespace(self):
        self._test_get_x(
            self._get_unst,
            'nowhitespace=?utf-8?q?somevalue?=',
            'nowhitespacesomevalue',
            'nowhitespacesomevalue',
            [errors.InvalidHeaderDefect],
            '')

    def test_get_unstructured_ew_without_trailing_whitespace(self):
        self._test_get_x(
            self._get_unst,
            '=?utf-8?q?somevalue?=nowhitespace',
            'somevaluenowhitespace',
            'somevaluenowhitespace',
            [errors.InvalidHeaderDefect],
            '')

    def test_get_unstructured_without_trailing_whitespace_hang_case(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?q?somevalue?=aa',
            'somevalueaa',
            'somevalueaa',
            [errors.InvalidHeaderDefect],
            '')

    def test_get_unstructured_invalid_ew2(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?q?=somevalue?=',
            '=?utf-8?q?=somevalue?=',
            '=?utf-8?q?=somevalue?=',
            [],
            '')

    def test_get_unstructured_invalid_ew_cte(self):
        self._test_get_x(self._get_unst,
            '=?utf-8?X?=somevalue?=',
            '=?utf-8?X?=somevalue?=',
            '=?utf-8?X?=somevalue?=',
            [],
            '')

    # get_qp_ctext

    def test_get_qp_ctext_only(self):
        ptext = self._test_get_x(parser.get_qp_ctext,
                                'foobar', 'foobar', ' ', [], '')
        self.assertEqual(ptext.token_type, 'ptext')

    def test_get_qp_ctext_all_printables(self):
        with_qp = self.rfc_printable_ascii.replace('\\', '\\\\')
        with_qp = with_qp.  replace('(', r'\(')
        with_qp = with_qp.replace(')', r'\)')
        ptext = self._test_get_x(parser.get_qp_ctext,
                                 with_qp, self.rfc_printable_ascii, ' ', [], '')

    def test_get_qp_ctext_two_words_gets_first(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo de', 'foo', ' ', [], ' de')

    def test_get_qp_ctext_following_wsp_preserved(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo \t\tde', 'foo', ' ', [], ' \t\tde')

    def test_get_qp_ctext_up_to_close_paren_only(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo)', 'foo', ' ', [], ')')

    def test_get_qp_ctext_wsp_before_close_paren_preserved(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo  )', 'foo', ' ', [], '  )')

    def test_get_qp_ctext_close_paren_mid_word(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo)bar', 'foo', ' ', [], ')bar')

    def test_get_qp_ctext_up_to_open_paren_only(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo(', 'foo', ' ', [], '(')

    def test_get_qp_ctext_wsp_before_open_paren_preserved(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo  (', 'foo', ' ', [], '  (')

    def test_get_qp_ctext_open_paren_mid_word(self):
        self._test_get_x(parser.get_qp_ctext,
                        'foo(bar', 'foo', ' ', [], '(bar')

    def test_get_qp_ctext_non_printables(self):
        ptext = self._test_get_x(parser.get_qp_ctext,
                                'foo\x00bar)', 'foo\x00bar', ' ',
                                [errors.NonPrintableDefect], ')')
        self.assertEqual(ptext.defects[0].non_printables[0], '\x00')

    # get_qcontent

    def test_get_qcontent_only(self):
        ptext = self._test_get_x(parser.get_qcontent,
                                'foobar', 'foobar', 'foobar', [], '')
        self.assertEqual(ptext.token_type, 'ptext')

    def test_get_qcontent_all_printables(self):
        with_qp = self.rfc_printable_ascii.replace('\\', '\\\\')
        with_qp = with_qp.  replace('"', r'\"')
        ptext = self._test_get_x(parser.get_qcontent, with_qp,
                                 self.rfc_printable_ascii,
                                 self.rfc_printable_ascii, [], '')

    def test_get_qcontent_two_words_gets_first(self):
        self._test_get_x(parser.get_qcontent,
                        'foo de', 'foo', 'foo', [], ' de')

    def test_get_qcontent_following_wsp_preserved(self):
        self._test_get_x(parser.get_qcontent,
                        'foo \t\tde', 'foo', 'foo', [], ' \t\tde')

    def test_get_qcontent_up_to_dquote_only(self):
        self._test_get_x(parser.get_qcontent,
                        'foo"', 'foo', 'foo', [], '"')

    def test_get_qcontent_wsp_before_close_paren_preserved(self):
        self._test_get_x(parser.get_qcontent,
                        'foo  "', 'foo', 'foo', [], '  "')

    def test_get_qcontent_close_paren_mid_word(self):
        self._test_get_x(parser.get_qcontent,
                        'foo"bar', 'foo', 'foo', [], '"bar')

    def test_get_qcontent_non_printables(self):
        ptext = self._test_get_x(parser.get_qcontent,
                                'foo\x00fg"', 'foo\x00fg', 'foo\x00fg',
                                [errors.NonPrintableDefect], '"')
        self.assertEqual(ptext.defects[0].non_printables[0], '\x00')

    # get_atext

    def test_get_atext_only(self):
        atext = self._test_get_x(parser.get_atext,
                                'foobar', 'foobar', 'foobar', [], '')
        self.assertEqual(atext.token_type, 'atext')

    def test_get_atext_all_atext(self):
        atext = self._test_get_x(parser.get_atext, self.rfc_atext_chars,
                                 self.rfc_atext_chars,
                                 self.rfc_atext_chars, [], '')

    def test_get_atext_two_words_gets_first(self):
        self._test_get_x(parser.get_atext,
                        'foo bar', 'foo', 'foo', [], ' bar')

    def test_get_atext_following_wsp_preserved(self):
        self._test_get_x(parser.get_atext,
                        'foo \t\tbar', 'foo', 'foo', [], ' \t\tbar')

    def test_get_atext_up_to_special(self):
        self._test_get_x(parser.get_atext,
                        'foo@bar', 'foo', 'foo', [], '@bar')

    def test_get_atext_non_printables(self):
        atext = self._test_get_x(parser.get_atext,
                                'foo\x00bar(', 'foo\x00bar', 'foo\x00bar',
                                [errors.NonPrintableDefect], '(')
        self.assertEqual(atext.defects[0].non_printables[0], '\x00')

    # get_bare_quoted_string

    def test_get_bare_quoted_string_only(self):
        bqs = self._test_get_x(parser.get_bare_quoted_string,
                               '"foo"', '"foo"', 'foo', [], '')
        self.assertEqual(bqs.token_type, 'bare-quoted-string')

    def test_get_bare_quoted_string_must_start_with_dquote(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_bare_quoted_string('foo"')
        with self.assertRaises(errors.HeaderParseError):
            parser.get_bare_quoted_string('  "foo"')

    def test_get_bare_quoted_string_only_quotes(self):
        self._test_get_x(parser.get_bare_quoted_string,
                         '""', '""', '', [], '')

    def test_get_bare_quoted_string_missing_endquotes(self):
        self._test_get_x(parser.get_bare_quoted_string,
                         '"', '""', '', [errors.InvalidHeaderDefect], '')

    def test_get_bare_quoted_string_following_wsp_preserved(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '"foo"\t bar', '"foo"', 'foo', [], '\t bar')

    def test_get_bare_quoted_string_multiple_words(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '"foo bar moo"', '"foo bar moo"', 'foo bar moo', [], '')

    def test_get_bare_quoted_string_multiple_words_wsp_preserved(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '" foo  moo\t"', '" foo  moo\t"', ' foo  moo\t', [], '')

    def test_get_bare_quoted_string_end_dquote_mid_word(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '"foo"bar', '"foo"', 'foo', [], 'bar')

    def test_get_bare_quoted_string_quoted_dquote(self):
        self._test_get_x(parser.get_bare_quoted_string,
             r'"foo\"in"a', r'"foo\"in"', 'foo"in', [], 'a')

    def test_get_bare_quoted_string_non_printables(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '"a\x01a"', '"a\x01a"', 'a\x01a',
             [errors.NonPrintableDefect], '')

    def test_get_bare_quoted_string_no_end_dquote(self):
        self._test_get_x(parser.get_bare_quoted_string,
             '"foo', '"foo"', 'foo',
             [errors.InvalidHeaderDefect], '')
        self._test_get_x(parser.get_bare_quoted_string,
             '"foo ', '"foo "', 'foo ',
             [errors.InvalidHeaderDefect], '')

    def test_get_bare_quoted_string_empty_quotes(self):
        self._test_get_x(parser.get_bare_quoted_string,
            '""', '""', '', [], '')

    # Issue 16983: apply postel's law to some bad encoding.
    def test_encoded_word_inside_quotes(self):
        self._test_get_x(parser.get_bare_quoted_string,
            '"=?utf-8?Q?not_really_valid?="',
            '"not really valid"',
            'not really valid',
            [errors.InvalidHeaderDefect,
             errors.InvalidHeaderDefect],
            '')

    # get_comment

    def test_get_comment_only(self):
        comment = self._test_get_x(parser.get_comment,
            '(comment)', '(comment)', ' ', [], '', ['comment'])
        self.assertEqual(comment.token_type, 'comment')

    def test_get_comment_must_start_with_paren(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_comment('foo"')
        with self.assertRaises(errors.HeaderParseError):
            parser.get_comment('  (foo"')

    def test_get_comment_following_wsp_preserved(self):
        self._test_get_x(parser.get_comment,
            '(comment)  \t', '(comment)', ' ', [], '  \t', ['comment'])

    def test_get_comment_multiple_words(self):
        self._test_get_x(parser.get_comment,
            '(foo bar)  \t', '(foo bar)', ' ', [], '  \t', ['foo bar'])

    def test_get_comment_multiple_words_wsp_preserved(self):
        self._test_get_x(parser.get_comment,
            '( foo  bar\t )  \t', '( foo  bar\t )', ' ', [], '  \t',
                [' foo  bar\t '])

    def test_get_comment_end_paren_mid_word(self):
        self._test_get_x(parser.get_comment,
            '(foo)bar', '(foo)', ' ', [], 'bar', ['foo'])

    def test_get_comment_quoted_parens(self):
        self._test_get_x(parser.get_comment,
            r'(foo\) \(\)bar)', r'(foo\) \(\)bar)', ' ', [], '', ['foo) ()bar'])

    def test_get_comment_non_printable(self):
        self._test_get_x(parser.get_comment,
            '(foo\x7Fbar)', '(foo\x7Fbar)', ' ',
            [errors.NonPrintableDefect], '', ['foo\x7Fbar'])

    def test_get_comment_no_end_paren(self):
        self._test_get_x(parser.get_comment,
            '(foo bar', '(foo bar)', ' ',
            [errors.InvalidHeaderDefect], '', ['foo bar'])
        self._test_get_x(parser.get_comment,
            '(foo bar  ', '(foo bar  )', ' ',
            [errors.InvalidHeaderDefect], '', ['foo bar  '])

    def test_get_comment_nested_comment(self):
        comment = self._test_get_x(parser.get_comment,
            '(foo(bar))', '(foo(bar))', ' ', [], '', ['foo(bar)'])
        self.assertEqual(comment[1].content, 'bar')

    def test_get_comment_nested_comment_wsp(self):
        comment = self._test_get_x(parser.get_comment,
            '(foo ( bar ) )', '(foo ( bar ) )', ' ', [], '', ['foo ( bar ) '])
        self.assertEqual(comment[2].content, ' bar ')

    def test_get_comment_empty_comment(self):
        self._test_get_x(parser.get_comment,
            '()', '()', ' ', [], '', [''])

    def test_get_comment_multiple_nesting(self):
        comment = self._test_get_x(parser.get_comment,
            '(((((foo)))))', '(((((foo)))))', ' ', [], '', ['((((foo))))'])
        for i in range(4, 0, -1):
            self.assertEqual(comment[0].content, '('*(i-1)+'foo'+')'*(i-1))
            comment = comment[0]
        self.assertEqual(comment.content, 'foo')

    def test_get_comment_missing_end_of_nesting(self):
        self._test_get_x(parser.get_comment,
            '(((((foo)))', '(((((foo)))))', ' ',
            [errors.InvalidHeaderDefect]*2, '', ['((((foo))))'])

    def test_get_comment_qs_in_nested_comment(self):
        comment = self._test_get_x(parser.get_comment,
            r'(foo (b\)))', r'(foo (b\)))', ' ', [], '', [r'foo (b\))'])
        self.assertEqual(comment[2].content, 'b)')

    # get_cfws

    def test_get_cfws_only_ws(self):
        cfws = self._test_get_x(parser.get_cfws,
            '  \t \t', '  \t \t', ' ', [], '', [])
        self.assertEqual(cfws.token_type, 'cfws')

    def test_get_cfws_only_comment(self):
        cfws = self._test_get_x(parser.get_cfws,
            '(foo)', '(foo)', ' ', [], '', ['foo'])
        self.assertEqual(cfws[0].content, 'foo')

    def test_get_cfws_only_mixed(self):
        cfws = self._test_get_x(parser.get_cfws,
            ' (foo )  ( bar) ', ' (foo )  ( bar) ', ' ', [], '',
                ['foo ', ' bar'])
        self.assertEqual(cfws[1].content, 'foo ')
        self.assertEqual(cfws[3].content, ' bar')

    def test_get_cfws_ends_at_non_leader(self):
        cfws = self._test_get_x(parser.get_cfws,
            '(foo) bar', '(foo) ', ' ', [], 'bar', ['foo'])
        self.assertEqual(cfws[0].content, 'foo')

    def test_get_cfws_ends_at_non_printable(self):
        cfws = self._test_get_x(parser.get_cfws,
            '(foo) \x07', '(foo) ', ' ', [], '\x07', ['foo'])
        self.assertEqual(cfws[0].content, 'foo')

    def test_get_cfws_non_printable_in_comment(self):
        cfws = self._test_get_x(parser.get_cfws,
            '(foo \x07) "test"', '(foo \x07) ', ' ',
            [errors.NonPrintableDefect], '"test"', ['foo \x07'])
        self.assertEqual(cfws[0].content, 'foo \x07')

    def test_get_cfws_header_ends_in_comment(self):
        cfws = self._test_get_x(parser.get_cfws,
            '  (foo ', '  (foo )', ' ',
            [errors.InvalidHeaderDefect], '', ['foo '])
        self.assertEqual(cfws[1].content, 'foo ')

    def test_get_cfws_multiple_nested_comments(self):
        cfws = self._test_get_x(parser.get_cfws,
            '(foo (bar)) ((a)(a))', '(foo (bar)) ((a)(a))', ' ', [],
                '', ['foo (bar)', '(a)(a)'])
        self.assertEqual(cfws[0].comments, ['foo (bar)'])
        self.assertEqual(cfws[2].comments, ['(a)(a)'])

    # get_quoted_string

    def test_get_quoted_string_only(self):
        qs = self._test_get_x(parser.get_quoted_string,
            '"bob"', '"bob"', 'bob', [], '')
        self.assertEqual(qs.token_type, 'quoted-string')
        self.assertEqual(qs.quoted_value, '"bob"')
        self.assertEqual(qs.content, 'bob')

    def test_get_quoted_string_with_wsp(self):
        qs = self._test_get_x(parser.get_quoted_string,
            '\t "bob"  ', '\t "bob"  ', ' bob ', [], '')
        self.assertEqual(qs.quoted_value, ' "bob" ')
        self.assertEqual(qs.content, 'bob')

    def test_get_quoted_string_with_comments_and_wsp(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (foo) "bob"(bar)', ' (foo) "bob"(bar)', ' bob ', [], '')
        self.assertEqual(qs[0][1].content, 'foo')
        self.assertEqual(qs[2][0].content, 'bar')
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob" ')

    def test_get_quoted_string_with_multiple_comments(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (foo) (bar) "bob"(bird)', ' (foo) (bar) "bob"(bird)', ' bob ',
                [], '')
        self.assertEqual(qs[0].comments, ['foo', 'bar'])
        self.assertEqual(qs[2].comments, ['bird'])
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob" ')

    def test_get_quoted_string_non_printable_in_comment(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (\x0A) "bob"', ' (\x0A) "bob"', ' bob',
                [errors.NonPrintableDefect], '')
        self.assertEqual(qs[0].comments, ['\x0A'])
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob"')

    def test_get_quoted_string_non_printable_in_qcontent(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (a) "a\x0B"', ' (a) "a\x0B"', ' a\x0B',
                [errors.NonPrintableDefect], '')
        self.assertEqual(qs[0].comments, ['a'])
        self.assertEqual(qs.content, 'a\x0B')
        self.assertEqual(qs.quoted_value, ' "a\x0B"')

    def test_get_quoted_string_internal_ws(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (a) "foo  bar "', ' (a) "foo  bar "', ' foo  bar ',
                [], '')
        self.assertEqual(qs[0].comments, ['a'])
        self.assertEqual(qs.content, 'foo  bar ')
        self.assertEqual(qs.quoted_value, ' "foo  bar "')

    def test_get_quoted_string_header_ends_in_comment(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (a) "bob" (a', ' (a) "bob" (a)', ' bob ',
                [errors.InvalidHeaderDefect], '')
        self.assertEqual(qs[0].comments, ['a'])
        self.assertEqual(qs[2].comments, ['a'])
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob" ')

    def test_get_quoted_string_header_ends_in_qcontent(self):
        qs = self._test_get_x(parser.get_quoted_string,
            ' (a) "bob', ' (a) "bob"', ' bob',
                [errors.InvalidHeaderDefect], '')
        self.assertEqual(qs[0].comments, ['a'])
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob"')

    def test_get_quoted_string_cfws_only_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_quoted_string(' (foo) ')

    def test_get_quoted_string_no_quoted_string(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_quoted_string(' (ab) xyz')

    def test_get_quoted_string_qs_ends_at_noncfws(self):
        qs = self._test_get_x(parser.get_quoted_string,
            '\t "bob" fee', '\t "bob" ', ' bob ', [], 'fee')
        self.assertEqual(qs.content, 'bob')
        self.assertEqual(qs.quoted_value, ' "bob" ')

    # get_atom

    def test_get_atom_only(self):
        atom = self._test_get_x(parser.get_atom,
            'bob', 'bob', 'bob', [], '')
        self.assertEqual(atom.token_type, 'atom')

    def test_get_atom_with_wsp(self):
        self._test_get_x(parser.get_atom,
            '\t bob  ', '\t bob  ', ' bob ', [], '')

    def test_get_atom_with_comments_and_wsp(self):
        atom = self._test_get_x(parser.get_atom,
            ' (foo) bob(bar)', ' (foo) bob(bar)', ' bob ', [], '')
        self.assertEqual(atom[0][1].content, 'foo')
        self.assertEqual(atom[2][0].content, 'bar')

    def test_get_atom_with_multiple_comments(self):
        atom = self._test_get_x(parser.get_atom,
            ' (foo) (bar) bob(bird)', ' (foo) (bar) bob(bird)', ' bob ',
                [], '')
        self.assertEqual(atom[0].comments, ['foo', 'bar'])
        self.assertEqual(atom[2].comments, ['bird'])

    def test_get_atom_non_printable_in_comment(self):
        atom = self._test_get_x(parser.get_atom,
            ' (\x0A) bob', ' (\x0A) bob', ' bob',
                [errors.NonPrintableDefect], '')
        self.assertEqual(atom[0].comments, ['\x0A'])

    def test_get_atom_non_printable_in_atext(self):
        atom = self._test_get_x(parser.get_atom,
            ' (a) a\x0B', ' (a) a\x0B', ' a\x0B',
                [errors.NonPrintableDefect], '')
        self.assertEqual(atom[0].comments, ['a'])

    def test_get_atom_header_ends_in_comment(self):
        atom = self._test_get_x(parser.get_atom,
            ' (a) bob (a', ' (a) bob (a)', ' bob ',
                [errors.InvalidHeaderDefect], '')
        self.assertEqual(atom[0].comments, ['a'])
        self.assertEqual(atom[2].comments, ['a'])

    def test_get_atom_no_atom(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_atom(' (ab) ')

    def test_get_atom_no_atom_before_special(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_atom(' (ab) @')

    def test_get_atom_atom_ends_at_special(self):
        atom = self._test_get_x(parser.get_atom,
            ' (foo) bob(bar)  @bang', ' (foo) bob(bar)  ', ' bob ', [], '@bang')
        self.assertEqual(atom[0].comments, ['foo'])
        self.assertEqual(atom[2].comments, ['bar'])

    def test_get_atom_atom_ends_at_noncfws(self):
        self._test_get_x(parser.get_atom,
            'bob  fred', 'bob  ', 'bob ', [], 'fred')

    def test_get_atom_rfc2047_atom(self):
        self._test_get_x(parser.get_atom,
            '=?utf-8?q?=20bob?=', ' bob', ' bob', [], '')

    # get_dot_atom_text

    def test_get_dot_atom_text(self):
        dot_atom_text = self._test_get_x(parser.get_dot_atom_text,
            'foo.bar.bang', 'foo.bar.bang', 'foo.bar.bang', [], '')
        self.assertEqual(dot_atom_text.token_type, 'dot-atom-text')
        self.assertEqual(len(dot_atom_text), 5)

    def test_get_dot_atom_text_lone_atom_is_valid(self):
        dot_atom_text = self._test_get_x(parser.get_dot_atom_text,
            'foo', 'foo', 'foo', [], '')

    def test_get_dot_atom_text_raises_on_leading_dot(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom_text('.foo.bar')

    def test_get_dot_atom_text_raises_on_trailing_dot(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom_text('foo.bar.')

    def test_get_dot_atom_text_raises_on_leading_non_atext(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom_text(' foo.bar')
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom_text('@foo.bar')
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom_text('"foo.bar"')

    def test_get_dot_atom_text_trailing_text_preserved(self):
        dot_atom_text = self._test_get_x(parser.get_dot_atom_text,
            'foo@bar', 'foo', 'foo', [], '@bar')

    def test_get_dot_atom_text_trailing_ws_preserved(self):
        dot_atom_text = self._test_get_x(parser.get_dot_atom_text,
            'foo .bar', 'foo', 'foo', [], ' .bar')

    # get_dot_atom

    def test_get_dot_atom_only(self):
        dot_atom = self._test_get_x(parser.get_dot_atom,
            'foo.bar.bing', 'foo.bar.bing', 'foo.bar.bing', [], '')
        self.assertEqual(dot_atom.token_type, 'dot-atom')
        self.assertEqual(len(dot_atom), 1)

    def test_get_dot_atom_with_wsp(self):
        self._test_get_x(parser.get_dot_atom,
            '\t  foo.bar.bing  ', '\t  foo.bar.bing  ', ' foo.bar.bing ', [], '')

    def test_get_dot_atom_with_comments_and_wsp(self):
        self._test_get_x(parser.get_dot_atom,
            ' (sing)  foo.bar.bing (here) ', ' (sing)  foo.bar.bing (here) ',
                ' foo.bar.bing ', [], '')

    def test_get_dot_atom_space_ends_dot_atom(self):
        self._test_get_x(parser.get_dot_atom,
            ' (sing)  foo.bar .bing (here) ', ' (sing)  foo.bar ',
                ' foo.bar ', [], '.bing (here) ')

    def test_get_dot_atom_no_atom_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom(' (foo) ')

    def test_get_dot_atom_leading_dot_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom(' (foo) .bar')

    def test_get_dot_atom_two_dots_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom('bar..bang')

    def test_get_dot_atom_trailing_dot_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_dot_atom(' (foo) bar.bang. foo')

    def test_get_dot_atom_rfc2047_atom(self):
        self._test_get_x(parser.get_dot_atom,
            '=?utf-8?q?=20bob?=', ' bob', ' bob', [], '')

    # get_word (if this were black box we'd repeat all the qs/atom tests)

    def test_get_word_atom_yields_atom(self):
        word = self._test_get_x(parser.get_word,
            ' (foo) bar (bang) :ah', ' (foo) bar (bang) ', ' bar ', [], ':ah')
        self.assertEqual(word.token_type, 'atom')
        self.assertEqual(word[0].token_type, 'cfws')

    def test_get_word_all_CFWS(self):
        # bpo-29412: Test that we don't raise IndexError when parsing CFWS only
        # token.
        with self.assertRaises(errors.HeaderParseError):
            parser.get_word('(Recipients list suppressed')

    def test_get_word_qs_yields_qs(self):
        word = self._test_get_x(parser.get_word,
            '"bar " (bang) ah', '"bar " (bang) ', 'bar  ', [], 'ah')
        self.assertEqual(word.token_type, 'quoted-string')
        self.assertEqual(word[0].token_type, 'bare-quoted-string')
        self.assertEqual(word[0].value, 'bar ')
        self.assertEqual(word.content, 'bar ')

    def test_get_word_ends_at_dot(self):
        self._test_get_x(parser.get_word,
            'foo.', 'foo', 'foo', [], '.')

    # get_phrase

    def test_get_phrase_simple(self):
        phrase = self._test_get_x(parser.get_phrase,
            '"Fred A. Johnson" is his name, oh.',
            '"Fred A. Johnson" is his name',
            'Fred A. Johnson is his name',
            [],
            ', oh.')
        self.assertEqual(phrase.token_type, 'phrase')

    def test_get_phrase_complex(self):
        phrase = self._test_get_x(parser.get_phrase,
            ' (A) bird (in (my|your)) "hand  " is messy\t<>\t',
            ' (A) bird (in (my|your)) "hand  " is messy\t',
            ' bird hand   is messy ',
            [],
            '<>\t')
        self.assertEqual(phrase[0][0].comments, ['A'])
        self.assertEqual(phrase[0][2].comments, ['in (my|your)'])

    def test_get_phrase_obsolete(self):
        phrase = self._test_get_x(parser.get_phrase,
            'Fred A.(weird).O Johnson',
            'Fred A.(weird).O Johnson',
            'Fred A. .O Johnson',
            [errors.ObsoleteHeaderDefect]*3,
            '')
        self.assertEqual(len(phrase), 7)
        self.assertEqual(phrase[3].comments, ['weird'])

    def test_get_phrase_pharse_must_start_with_word(self):
        phrase = self._test_get_x(parser.get_phrase,
            '(even weirder).name',
            '(even weirder).name',
            ' .name',
            [errors.InvalidHeaderDefect] + [errors.ObsoleteHeaderDefect]*2,
            '')
        self.assertEqual(len(phrase), 3)
        self.assertEqual(phrase[0].comments, ['even weirder'])

    def test_get_phrase_ending_with_obsolete(self):
        phrase = self._test_get_x(parser.get_phrase,
            'simple phrase.(with trailing comment):boo',
            'simple phrase.(with trailing comment)',
            'simple phrase. ',
            [errors.ObsoleteHeaderDefect]*2,
            ':boo')
        self.assertEqual(len(phrase), 4)
        self.assertEqual(phrase[3].comments, ['with trailing comment'])

    def get_phrase_cfws_only_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_phrase(' (foo) ')

    # get_local_part

    def test_get_local_part_simple(self):
        local_part = self._test_get_x(parser.get_local_part,
            'dinsdale@python.org', 'dinsdale', 'dinsdale', [], '@python.org')
        self.assertEqual(local_part.token_type, 'local-part')
        self.assertEqual(local_part.local_part, 'dinsdale')

    def test_get_local_part_with_dot(self):
        local_part = self._test_get_x(parser.get_local_part,
            'Fred.A.Johnson@python.org',
            'Fred.A.Johnson',
            'Fred.A.Johnson',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson')

    def test_get_local_part_with_whitespace(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' Fred.A.Johnson  @python.org',
            ' Fred.A.Johnson  ',
            ' Fred.A.Johnson ',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson')

    def test_get_local_part_with_cfws(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' (foo) Fred.A.Johnson (bar (bird))  @python.org',
            ' (foo) Fred.A.Johnson (bar (bird))  ',
            ' Fred.A.Johnson ',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson')
        self.assertEqual(local_part[0][0].comments, ['foo'])
        self.assertEqual(local_part[0][2].comments, ['bar (bird)'])

    def test_get_local_part_simple_quoted(self):
        local_part = self._test_get_x(parser.get_local_part,
            '"dinsdale"@python.org', '"dinsdale"', '"dinsdale"', [], '@python.org')
        self.assertEqual(local_part.token_type, 'local-part')
        self.assertEqual(local_part.local_part, 'dinsdale')

    def test_get_local_part_with_quoted_dot(self):
        local_part = self._test_get_x(parser.get_local_part,
            '"Fred.A.Johnson"@python.org',
            '"Fred.A.Johnson"',
            '"Fred.A.Johnson"',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson')

    def test_get_local_part_quoted_with_whitespace(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' "Fred A. Johnson"  @python.org',
            ' "Fred A. Johnson"  ',
            ' "Fred A. Johnson" ',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred A. Johnson')

    def test_get_local_part_quoted_with_cfws(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' (foo) " Fred A. Johnson " (bar (bird))  @python.org',
            ' (foo) " Fred A. Johnson " (bar (bird))  ',
            ' " Fred A. Johnson " ',
            [],
            '@python.org')
        self.assertEqual(local_part.local_part, ' Fred A. Johnson ')
        self.assertEqual(local_part[0][0].comments, ['foo'])
        self.assertEqual(local_part[0][2].comments, ['bar (bird)'])


    def test_get_local_part_simple_obsolete(self):
        local_part = self._test_get_x(parser.get_local_part,
            'Fred. A.Johnson@python.org',
            'Fred. A.Johnson',
            'Fred. A.Johnson',
            [errors.ObsoleteHeaderDefect],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson')

    def test_get_local_part_complex_obsolete_1(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson."and  dogs "@python.org',
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson."and  dogs "',
            ' Fred . A. Johnson.and  dogs ',
            [errors.ObsoleteHeaderDefect],
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson.and  dogs ')

    def test_get_local_part_complex_obsolete_invalid(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson "and  dogs"@python.org',
            ' (foo )Fred (bar).(bird) A.(sheep)Johnson "and  dogs"',
            ' Fred . A. Johnson and  dogs',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, 'Fred.A.Johnson and  dogs')

    def test_get_local_part_empty_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_local_part('')

    def test_get_local_part_no_part_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_local_part(' (foo) ')

    def test_get_local_part_special_instead_raises(self):
        with self.assertRaises(errors.HeaderParseError):
            parser.get_local_part(' (foo) @python.org')

    def test_get_local_part_trailing_dot(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' borris.@python.org',
            ' borris.',
            ' borris.',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, 'borris.')

    def test_get_local_part_trailing_dot_with_ws(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' borris. @python.org',
            ' borris. ',
            ' borris. ',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, 'borris.')

    def test_get_local_part_leading_dot(self):
        local_part = self._test_get_x(parser.get_local_part,
            '.borris@python.org',
            '.borris',
            '.borris',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, '.borris')

    def test_get_local_part_leading_dot_after_ws(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' .borris@python.org',
            ' .borris',
            ' .borris',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, '.borris')

    def test_get_local_part_double_dot_raises(self):
        local_part = self._test_get_x(parser.get_local_part,
            ' borris.(foo).natasha@python.org',
            ' borris.(foo).natasha',
            ' borris. .natasha',
            [errors.InvalidHeaderDefect]*2,
            '@python.org')
        self.assertEqual(local_part.local_part, 'borris..natasha')

    def test_get_local_part_quoted_strings_in_atom_list(self):
        local_part = self._test_get_x(parser.get_local_part,
            '""example" example"@example.com',
            '""example" example"',
            'example example',
            [errors.InvalidHeaderDefect]*3,
            '@example.com')
        self.assertEqual(local_part.local_part, 'example example')

    def test_get_local_part_valid_and_invalid_qp_in_atom_list(self):
        local_part = self._test_get_x(parser.get_local_part,
            r'"\\"example\\" example"@example.com',
            r'"\\"example\\" example"',
            r'\example\\ example',
            [errors.InvalidHeaderDefect]*5,
            '@example.com')
        self.assertEqual(local_part.local_part, r'\example\\ example')

    def test_get_local_part_unicode_defect(self):
        # Currently this only happens when parsing unicode, not when parsing
        # stuff that was originally binary.
        local_part = self._test_get_x(parser.get_local_part,
            'exámple@example.com',
            'exámple',
            'exámple',
            [errors.NonASCIILocalPartDefect],
            '@example.com')
        self.assertEqual(local_part.local_part, 'exámple')

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
            [errors.ObsoleteHeaderDefect],
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
        self.assertEqual(len(address_list.all_mailboxes), 3)
        self.assertEqual([str(x) for x in address_list.all_mailboxes],
                         [str(x) for x in address_list.addresses])
        self.assertEqual(address_list.mailboxes[0].domain, 'example.com')
        self.assertEqual(address_list.mailboxes[0].token_type, 'mailbox')
        self.assertEqual(address_list.addresses[0].token_type, 'address')
        self.assertEqual(address_list.addresses[1].token_type, 'address')
        self.assertEqual(len(address_list.addresses[0].mailboxes), 1)
        self.assertEqual(len(address_list.addresses[1].mailboxes), 0)
        self.assertEqual(len(address_list.addresses[1].mailboxes), 0)
        self.assertEqual(
            address_list.addresses[1].all_mailboxes[0].local_part, 'Foo x')
        self.assertEqual(
            address_list.addresses[2].all_mailboxes[0].display_name,
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
        # Microsof Outlook generates non-standard one-off addresses:
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

    def test_address_list_with_list_separator_after_fold(self):
        a = 'x' * 66 + '@example.com'
        to = f'{a}, "Hübsch Kaktus" <beautiful@example.com>'
        self._test(parser.get_address_list(to)[0],
            f'{a},\n =?utf-8?q?H=C3=BCbsch?= Kaktus <beautiful@example.com>\n')

        a = '.' * 79
        to = f'"{a}" <xyz@example.com>, "Hübsch Kaktus" <beautiful@example.com>'
        self._test(parser.get_address_list(to)[0],
            f'{a}\n'
            ' <xyz@example.com>, =?utf-8?q?H=C3=BCbsch?= Kaktus '
            '<beautiful@example.com>\n')

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

if __name__ == '__main__':
    unittest.main()
