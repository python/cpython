#
# Test script for the textwrap module.
#
# Original tests written by Greg Ward <gward@python.net>.
# Converted to PyUnit by Peter Hansen <peter@engcorp.com>.
# Currently maintained by Greg Ward.
#
# $Id$
#

import unittest
from test import test_support

from textwrap import TextWrapper, wrap, fill


class BaseTestCase(unittest.TestCase):
    '''Parent class with utility methods for textwrap tests.'''

    def show(self, textin):
        if isinstance(textin, list):
            result = []
            for i in range(len(textin)):
                result.append("  %d: %r" % (i, textin[i]))
            result = '\n'.join(result)
        elif isinstance(textin, (str, unicode)):
            result = "  %s\n" % repr(textin)
        return result


    def check(self, result, expect):
        self.assertEquals(result, expect,
            'expected:\n%s\nbut got:\n%s' % (
                self.show(expect), self.show(result)))

    def check_wrap (self, text, width, expect):
        result = wrap(text, width)
        self.check(result, expect)


class WrapTestCase(BaseTestCase):

    def setUp(self):
        self.wrapper = TextWrapper(width=45, fix_sentence_endings=True)

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

        result = self.wrapper.wrap(text)
        self.check(result, expect)

        result = self.wrapper.fill(text)
        self.check(result, '\n'.join(expect))


    def test_wrap_short(self):
        # Wrapping to make short lines longer

        text = "This is a\nshort paragraph."

        self.check_wrap(text, 20, ["This is a short",
                                   "paragraph."])
        self.check_wrap(text, 40, ["This is a short paragraph."])


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
        result = self.wrapper._split(text)
        expect = ["Here's", " ", "an", " ", "--", " ", "em-", "dash", " ",
                  "and", "--", "here's", " ", "another", "---",
                  "and", " ", "another!"]
        self.assertEquals(result, expect,
                          "\nexpected %r\n"
                          "but got  %r" % (expect, result))

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

    def test_split(self):
        # Ensure that the standard _split() method works as advertised
        # in the comments

        text = "Hello there -- you goof-ball, use the -b option!"

        result = self.wrapper._split(text)
        self.check(result,
             ["Hello", " ", "there", " ", "--", " ", "you", " ", "goof-",
              "ball,", " ", "use", " ", "the", " ", "-b", " ",  "option!"])


class LongWordTestCase (BaseTestCase):
    def setUp(self):
        self.wrapper = TextWrapper()
        self.text = '''
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


def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(WrapTestCase))
    suite.addTest(unittest.makeSuite(LongWordTestCase))
    suite.addTest(unittest.makeSuite(IndentTestCases))
    test_support.run_suite(suite)

if __name__ == '__main__':
    test_main()
