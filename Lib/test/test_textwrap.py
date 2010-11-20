#
# Test suite for the textwrap module.
#
# Original tests written by Greg Ward <gward@python.net>.
# Converted to PyUnit by Peter Hansen <peter@engcorp.com>.
# Currently maintained by Greg Ward.
#
# $Id$
#

import unittest
from test import support

from textwrap import TextWrapper, wrap, fill, dedent


class BaseTestCase(unittest.TestCase):
    '''Parent class with utility methods for textwrap tests.'''

    def show(self, textin):
        if isinstance(textin, list):
            result = []
            for i in range(len(textin)):
                result.append("  %d: %r" % (i, textin[i]))
            result = '\n'.join(result)
        elif isinstance(textin, str):
            result = "  %s\n" % repr(textin)
        return result


    def check(self, result, expect):
        self.assertEqual(result, expect,
            'expected:\n%s\nbut got:\n%s' % (
                self.show(expect), self.show(result)))

    def check_wrap(self, text, width, expect, **kwargs):
        result = wrap(text, width, **kwargs)
        self.check(result, expect)

    def check_split(self, text, expect):
        result = self.wrapper._split(text)
        self.assertEqual(result, expect,
                         "\nexpected %r\n"
                         "but got  %r" % (expect, result))


class WrapTestCase(BaseTestCase):

    def setUp(self):
        self.wrapper = TextWrapper(width=45)

    def test_simple(self):
        # Simple case: just words, spaces, and a bit of punctuation

        text = "Hello there, how are you this fine day?  I'm glad to hear it!"

        self.check_wrap(text, 12,
                        ["Hello there,",
                         "how are you",
                         "this fine",
                         "day?  I'm",
                         "glad to hear",
                         "it!"])
        self.check_wrap(text, 42,
                        ["Hello there, how are you this fine day?",
                         "I'm glad to hear it!"])
        self.check_wrap(text, 80, [text])


    def test_whitespace(self):
        # Whitespace munging and end-of-sentence detection

        text = """\
This is a paragraph that already has
line breaks.  But some of its lines are much longer than the others,
so it needs to be wrapped.
Some lines are \ttabbed too.
What a mess!
"""

        expect = ["This is a paragraph that already has line",
                  "breaks.  But some of its lines are much",
                  "longer than the others, so it needs to be",
                  "wrapped.  Some lines are  tabbed too.  What a",
                  "mess!"]

        wrapper = TextWrapper(45, fix_sentence_endings=True)
        result = wrapper.wrap(text)
        self.check(result, expect)

        result = wrapper.fill(text)
        self.check(result, '\n'.join(expect))

    def test_fix_sentence_endings(self):
        wrapper = TextWrapper(60, fix_sentence_endings=True)

        # SF #847346: ensure that fix_sentence_endings=True does the
        # right thing even on input short enough that it doesn't need to
        # be wrapped.
        text = "A short line. Note the single space."
        expect = ["A short line.  Note the single space."]
        self.check(wrapper.wrap(text), expect)

        # Test some of the hairy end cases that _fix_sentence_endings()
        # is supposed to handle (the easy stuff is tested in
        # test_whitespace() above).
        text = "Well, Doctor? What do you think?"
        expect = ["Well, Doctor?  What do you think?"]
        self.check(wrapper.wrap(text), expect)

        text = "Well, Doctor?\nWhat do you think?"
        self.check(wrapper.wrap(text), expect)

        text = 'I say, chaps! Anyone for "tennis?"\nHmmph!'
        expect = ['I say, chaps!  Anyone for "tennis?"  Hmmph!']
        self.check(wrapper.wrap(text), expect)

        wrapper.width = 20
        expect = ['I say, chaps!', 'Anyone for "tennis?"', 'Hmmph!']
        self.check(wrapper.wrap(text), expect)

        text = 'And she said, "Go to hell!"\nCan you believe that?'
        expect = ['And she said, "Go to',
                  'hell!"  Can you',
                  'believe that?']
        self.check(wrapper.wrap(text), expect)

        wrapper.width = 60
        expect = ['And she said, "Go to hell!"  Can you believe that?']
        self.check(wrapper.wrap(text), expect)

        text = 'File stdio.h is nice.'
        expect = ['File stdio.h is nice.']
        self.check(wrapper.wrap(text), expect)

    def test_wrap_short(self):
        # Wrapping to make short lines longer

        text = "This is a\nshort paragraph."

        self.check_wrap(text, 20, ["This is a short",
                                   "paragraph."])
        self.check_wrap(text, 40, ["This is a short paragraph."])


    def test_wrap_short_1line(self):
        # Test endcases

        text = "This is a short line."

        self.check_wrap(text, 30, ["This is a short line."])
        self.check_wrap(text, 30, ["(1) This is a short line."],
                        initial_indent="(1) ")


    def test_hyphenated(self):
        # Test breaking hyphenated words

        text = ("this-is-a-useful-feature-for-"
                "reformatting-posts-from-tim-peters'ly")

        self.check_wrap(text, 40,
                        ["this-is-a-useful-feature-for-",
                         "reformatting-posts-from-tim-peters'ly"])
        self.check_wrap(text, 41,
                        ["this-is-a-useful-feature-for-",
                         "reformatting-posts-from-tim-peters'ly"])
        self.check_wrap(text, 42,
                        ["this-is-a-useful-feature-for-reformatting-",
                         "posts-from-tim-peters'ly"])

    def test_hyphenated_numbers(self):
        # Test that hyphenated numbers (eg. dates) are not broken like words.
        text = ("Python 1.0.0 was released on 1994-01-26.  Python 1.0.1 was\n"
                "released on 1994-02-15.")

        self.check_wrap(text, 30, ['Python 1.0.0 was released on',
                                   '1994-01-26.  Python 1.0.1 was',
                                   'released on 1994-02-15.'])
        self.check_wrap(text, 40, ['Python 1.0.0 was released on 1994-01-26.',
                                   'Python 1.0.1 was released on 1994-02-15.'])

        text = "I do all my shopping at 7-11."
        self.check_wrap(text, 25, ["I do all my shopping at",
                                   "7-11."])
        self.check_wrap(text, 27, ["I do all my shopping at",
                                   "7-11."])
        self.check_wrap(text, 29, ["I do all my shopping at 7-11."])

    def test_em_dash(self):
        # Test text with em-dashes
        text = "Em-dashes should be written -- thus."
        self.check_wrap(text, 25,
                        ["Em-dashes should be",
                         "written -- thus."])

        # Probe the boundaries of the properly written em-dash,
        # ie. " -- ".
        self.check_wrap(text, 29,
                        ["Em-dashes should be written",
                         "-- thus."])
        expect = ["Em-dashes should be written --",
                  "thus."]
        self.check_wrap(text, 30, expect)
        self.check_wrap(text, 35, expect)
        self.check_wrap(text, 36,
                        ["Em-dashes should be written -- thus."])

        # The improperly written em-dash is handled too, because
        # it's adjacent to non-whitespace on both sides.
        text = "You can also do--this or even---this."
        expect = ["You can also do",
                  "--this or even",
                  "---this."]
        self.check_wrap(text, 15, expect)
        self.check_wrap(text, 16, expect)
        expect = ["You can also do--",
                  "this or even---",
                  "this."]
        self.check_wrap(text, 17, expect)
        self.check_wrap(text, 19, expect)
        expect = ["You can also do--this or even",
                  "---this."]
        self.check_wrap(text, 29, expect)
        self.check_wrap(text, 31, expect)
        expect = ["You can also do--this or even---",
                  "this."]
        self.check_wrap(text, 32, expect)
        self.check_wrap(text, 35, expect)

        # All of the above behaviour could be deduced by probing the
        # _split() method.
        text = "Here's an -- em-dash and--here's another---and another!"
        expect = ["Here's", " ", "an", " ", "--", " ", "em-", "dash", " ",
                  "and", "--", "here's", " ", "another", "---",
                  "and", " ", "another!"]
        self.check_split(text, expect)

        text = "and then--bam!--he was gone"
        expect = ["and", " ", "then", "--", "bam!", "--",
                  "he", " ", "was", " ", "gone"]
        self.check_split(text, expect)


    def test_unix_options (self):
        # Test that Unix-style command-line options are wrapped correctly.
        # Both Optik (OptionParser) and Docutils rely on this behaviour!

        text = "You should use the -n option, or --dry-run in its long form."
        self.check_wrap(text, 20,
                        ["You should use the",
                         "-n option, or --dry-",
                         "run in its long",
                         "form."])
        self.check_wrap(text, 21,
                        ["You should use the -n",
                         "option, or --dry-run",
                         "in its long form."])
        expect = ["You should use the -n option, or",
                  "--dry-run in its long form."]
        self.check_wrap(text, 32, expect)
        self.check_wrap(text, 34, expect)
        self.check_wrap(text, 35, expect)
        self.check_wrap(text, 38, expect)
        expect = ["You should use the -n option, or --dry-",
                  "run in its long form."]
        self.check_wrap(text, 39, expect)
        self.check_wrap(text, 41, expect)
        expect = ["You should use the -n option, or --dry-run",
                  "in its long form."]
        self.check_wrap(text, 42, expect)

        # Again, all of the above can be deduced from _split().
        text = "the -n option, or --dry-run or --dryrun"
        expect = ["the", " ", "-n", " ", "option,", " ", "or", " ",
                  "--dry-", "run", " ", "or", " ", "--dryrun"]
        self.check_split(text, expect)

    def test_funky_hyphens (self):
        # Screwy edge cases cooked up by David Goodger.  All reported
        # in SF bug #596434.
        self.check_split("what the--hey!", ["what", " ", "the", "--", "hey!"])
        self.check_split("what the--", ["what", " ", "the--"])
        self.check_split("what the--.", ["what", " ", "the--."])
        self.check_split("--text--.", ["--text--."])

        # When I first read bug #596434, this is what I thought David
        # was talking about.  I was wrong; these have always worked
        # fine.  The real problem is tested in test_funky_parens()
        # below...
        self.check_split("--option", ["--option"])
        self.check_split("--option-opt", ["--option-", "opt"])
        self.check_split("foo --option-opt bar",
                         ["foo", " ", "--option-", "opt", " ", "bar"])

    def test_punct_hyphens(self):
        # Oh bother, SF #965425 found another problem with hyphens --
        # hyphenated words in single quotes weren't handled correctly.
        # In fact, the bug is that *any* punctuation around a hyphenated
        # word was handled incorrectly, except for a leading "--", which
        # was special-cased for Optik and Docutils.  So test a variety
        # of styles of punctuation around a hyphenated word.
        # (Actually this is based on an Optik bug report, #813077).
        self.check_split("the 'wibble-wobble' widget",
                         ['the', ' ', "'wibble-", "wobble'", ' ', 'widget'])
        self.check_split('the "wibble-wobble" widget',
                         ['the', ' ', '"wibble-', 'wobble"', ' ', 'widget'])
        self.check_split("the (wibble-wobble) widget",
                         ['the', ' ', "(wibble-", "wobble)", ' ', 'widget'])
        self.check_split("the ['wibble-wobble'] widget",
                         ['the', ' ', "['wibble-", "wobble']", ' ', 'widget'])

    def test_funky_parens (self):
        # Second part of SF bug #596434: long option strings inside
        # parentheses.
        self.check_split("foo (--option) bar",
                         ["foo", " ", "(--option)", " ", "bar"])

        # Related stuff -- make sure parens work in simpler contexts.
        self.check_split("foo (bar) baz",
                         ["foo", " ", "(bar)", " ", "baz"])
        self.check_split("blah (ding dong), wubba",
                         ["blah", " ", "(ding", " ", "dong),",
                          " ", "wubba"])

    def test_initial_whitespace(self):
        # SF bug #622849 reported inconsistent handling of leading
        # whitespace; let's test that a bit, shall we?
        text = " This is a sentence with leading whitespace."
        self.check_wrap(text, 50,
                        [" This is a sentence with leading whitespace."])
        self.check_wrap(text, 30,
                        [" This is a sentence with", "leading whitespace."])

    def test_no_drop_whitespace(self):
        # SF patch #1581073
        text = " This is a    sentence with     much whitespace."
        self.check_wrap(text, 10,
                        [" This is a", "    ", "sentence ",
                         "with     ", "much white", "space."],
                        drop_whitespace=False)

    def test_split(self):
        # Ensure that the standard _split() method works as advertised
        # in the comments

        text = "Hello there -- you goof-ball, use the -b option!"

        result = self.wrapper._split(text)
        self.check(result,
             ["Hello", " ", "there", " ", "--", " ", "you", " ", "goof-",
              "ball,", " ", "use", " ", "the", " ", "-b", " ",  "option!"])

    def test_break_on_hyphens(self):
        # Ensure that the break_on_hyphens attributes work
        text = "yaba daba-doo"
        self.check_wrap(text, 10, ["yaba daba-", "doo"],
                        break_on_hyphens=True)
        self.check_wrap(text, 10, ["yaba", "daba-doo"],
                        break_on_hyphens=False)

    def test_bad_width(self):
        # Ensure that width <= 0 is caught.
        text = "Whatever, it doesn't matter."
        self.assertRaises(ValueError, wrap, text, 0)
        self.assertRaises(ValueError, wrap, text, -1)

    def test_no_split_at_umlaut(self):
        text = "Die Empf\xe4nger-Auswahl"
        self.check_wrap(text, 13, ["Die", "Empf\xe4nger-", "Auswahl"])

    def test_umlaut_followed_by_dash(self):
        text = "aa \xe4\xe4-\xe4\xe4"
        self.check_wrap(text, 7, ["aa \xe4\xe4-", "\xe4\xe4"])


class LongWordTestCase (BaseTestCase):
    def setUp(self):
        self.wrapper = TextWrapper()
        self.text = '''\
Did you say "supercalifragilisticexpialidocious?"
How *do* you spell that odd word, anyways?
'''

    def test_break_long(self):
        # Wrap text with long words and lots of punctuation

        self.check_wrap(self.text, 30,
                        ['Did you say "supercalifragilis',
                         'ticexpialidocious?" How *do*',
                         'you spell that odd word,',
                         'anyways?'])
        self.check_wrap(self.text, 50,
                        ['Did you say "supercalifragilisticexpialidocious?"',
                         'How *do* you spell that odd word, anyways?'])

        # SF bug 797650.  Prevent an infinite loop by making sure that at
        # least one character gets split off on every pass.
        self.check_wrap('-'*10+'hello', 10,
                        ['----------',
                         '               h',
                         '               e',
                         '               l',
                         '               l',
                         '               o'],
                        subsequent_indent = ' '*15)

        # bug 1146.  Prevent a long word to be wrongly wrapped when the
        # preceding word is exactly one character shorter than the width
        self.check_wrap(self.text, 12,
                        ['Did you say ',
                         '"supercalifr',
                         'agilisticexp',
                         'ialidocious?',
                         '" How *do*',
                         'you spell',
                         'that odd',
                         'word,',
                         'anyways?'])

    def test_nobreak_long(self):
        # Test with break_long_words disabled
        self.wrapper.break_long_words = 0
        self.wrapper.width = 30
        expect = ['Did you say',
                  '"supercalifragilisticexpialidocious?"',
                  'How *do* you spell that odd',
                  'word, anyways?'
                  ]
        result = self.wrapper.wrap(self.text)
        self.check(result, expect)

        # Same thing with kwargs passed to standalone wrap() function.
        result = wrap(self.text, width=30, break_long_words=0)
        self.check(result, expect)


class IndentTestCases(BaseTestCase):

    # called before each test method
    def setUp(self):
        self.text = '''\
This paragraph will be filled, first without any indentation,
and then with some (including a hanging indent).'''


    def test_fill(self):
        # Test the fill() method

        expect = '''\
This paragraph will be filled, first
without any indentation, and then with
some (including a hanging indent).'''

        result = fill(self.text, 40)
        self.check(result, expect)


    def test_initial_indent(self):
        # Test initial_indent parameter

        expect = ["     This paragraph will be filled,",
                  "first without any indentation, and then",
                  "with some (including a hanging indent)."]
        result = wrap(self.text, 40, initial_indent="     ")
        self.check(result, expect)

        expect = "\n".join(expect)
        result = fill(self.text, 40, initial_indent="     ")
        self.check(result, expect)


    def test_subsequent_indent(self):
        # Test subsequent_indent parameter

        expect = '''\
  * This paragraph will be filled, first
    without any indentation, and then
    with some (including a hanging
    indent).'''

        result = fill(self.text, 40,
                      initial_indent="  * ", subsequent_indent="    ")
        self.check(result, expect)


# Despite the similar names, DedentTestCase is *not* the inverse
# of IndentTestCase!
class DedentTestCase(unittest.TestCase):

    def assertUnchanged(self, text):
        """assert that dedent() has no effect on 'text'"""
        self.assertEqual(text, dedent(text))

    def test_dedent_nomargin(self):
        # No lines indented.
        text = "Hello there.\nHow are you?\nOh good, I'm glad."
        self.assertUnchanged(text)

        # Similar, with a blank line.
        text = "Hello there.\n\nBoo!"
        self.assertUnchanged(text)

        # Some lines indented, but overall margin is still zero.
        text = "Hello there.\n  This is indented."
        self.assertUnchanged(text)

        # Again, add a blank line.
        text = "Hello there.\n\n  Boo!\n"
        self.assertUnchanged(text)

    def test_dedent_even(self):
        # All lines indented by two spaces.
        text = "  Hello there.\n  How are ya?\n  Oh good."
        expect = "Hello there.\nHow are ya?\nOh good."
        self.assertEqual(expect, dedent(text))

        # Same, with blank lines.
        text = "  Hello there.\n\n  How are ya?\n  Oh good.\n"
        expect = "Hello there.\n\nHow are ya?\nOh good.\n"
        self.assertEqual(expect, dedent(text))

        # Now indent one of the blank lines.
        text = "  Hello there.\n  \n  How are ya?\n  Oh good.\n"
        expect = "Hello there.\n\nHow are ya?\nOh good.\n"
        self.assertEqual(expect, dedent(text))

    def test_dedent_uneven(self):
        # Lines indented unevenly.
        text = '''\
        def foo():
            while 1:
                return foo
        '''
        expect = '''\
def foo():
    while 1:
        return foo
'''
        self.assertEqual(expect, dedent(text))

        # Uneven indentation with a blank line.
        text = "  Foo\n    Bar\n\n   Baz\n"
        expect = "Foo\n  Bar\n\n Baz\n"
        self.assertEqual(expect, dedent(text))

        # Uneven indentation with a whitespace-only line.
        text = "  Foo\n    Bar\n \n   Baz\n"
        expect = "Foo\n  Bar\n\n Baz\n"
        self.assertEqual(expect, dedent(text))

    # dedent() should not mangle internal tabs
    def test_dedent_preserve_internal_tabs(self):
        text = "  hello\tthere\n  how are\tyou?"
        expect = "hello\tthere\nhow are\tyou?"
        self.assertEqual(expect, dedent(text))

        # make sure that it preserves tabs when it's not making any
        # changes at all
        self.assertEqual(expect, dedent(expect))

    # dedent() should not mangle tabs in the margin (i.e.
    # tabs and spaces both count as margin, but are *not*
    # considered equivalent)
    def test_dedent_preserve_margin_tabs(self):
        text = "  hello there\n\thow are you?"
        self.assertUnchanged(text)

        # same effect even if we have 8 spaces
        text = "        hello there\n\thow are you?"
        self.assertUnchanged(text)

        # dedent() only removes whitespace that can be uniformly removed!
        text = "\thello there\n\thow are you?"
        expect = "hello there\nhow are you?"
        self.assertEqual(expect, dedent(text))

        text = "  \thello there\n  \thow are you?"
        self.assertEqual(expect, dedent(text))

        text = "  \t  hello there\n  \t  how are you?"
        self.assertEqual(expect, dedent(text))

        text = "  \thello there\n  \t  how are you?"
        expect = "hello there\n  how are you?"
        self.assertEqual(expect, dedent(text))


def test_main():
    support.run_unittest(WrapTestCase,
                              LongWordTestCase,
                              IndentTestCases,
                              DedentTestCase)

if __name__ == '__main__':
    test_main()
