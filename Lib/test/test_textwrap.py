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
            'Expected:\n%s\nbut got:\n%s' % (
                self.show(result), self.show(expect)))

    def check_wrap (self, text, width, expect):
        result = wrap(text, width)
        self.check(result, expect)


class WrapTestCase(BaseTestCase):

    def setUp(self):
        self.wrapper = TextWrapper(width=45, fix_sentence_endings=True)

    def test_simple(self):
        '''Simple case: just words, spaces, and a bit of punctuation.'''

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
        '''Whitespace munging and end-of-sentence detection.'''

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
        '''Wrapping to make short lines longer.'''

        text = "This is a\nshort paragraph."

        self.check_wrap(text, 20, ["This is a short",
                                   "paragraph."])
        self.check_wrap(text, 40, ["This is a short paragraph."])


    def test_hyphenated(self):
        '''Test breaking hyphenated words.'''

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


    def test_split(self):
        '''Ensure that the standard _split() method works as advertised 
           in the comments.'''

        text = "Hello there -- you goof-ball, use the -b option!"

        result = self.wrapper._split(text)
        self.check(result,
             ["Hello", " ", "there", " ", "--", " ", "you", " ", "goof-",
              "ball,", " ", "use", " ", "the", " ", "-b", " ",  "option!"])


    def test_funky_punc(self):
        '''Wrap text with long words and lots of punctuation.'''

        text = '''
Did you say "supercalifragilisticexpialidocious?"
How *do* you spell that odd word, anyways?
'''
        self.check_wrap(text, 30,
                        ['Did you say "supercalifragilis',
                         'ticexpialidocious?" How *do*',
                         'you spell that odd word,',
                         'anyways?'])
        self.check_wrap(text, 50,
                        ['Did you say "supercalifragilisticexpialidocious?"',
                         'How *do* you spell that odd word, anyways?'])


    def test_long_words(self):        
        '''Test with break_long_words disabled.'''
        text = '''
Did you say "supercalifragilisticexpialidocious?"
How *do* you spell that odd word, anyways?
'''
        self.wrapper.break_long_words = 0
        self.wrapper.width = 30
        expect = ['Did you say',
                  '"supercalifragilisticexpialidocious?"',
                  'How *do* you spell that odd',
                  'word, anyways?'
                  ] 
        result = self.wrapper.wrap(text)
        self.check(result, expect)

        # Same thing with kwargs passed to standalone wrap() function.
        result = wrap(text, width=30, break_long_words=0)
        self.check(result, expect)



class IndentTestCases(BaseTestCase):

    # called before each test method
    def setUp(self):
        self.testString = '''\
This paragraph will be filled, first without any indentation,
and then with some (including a hanging indent).'''


    def test_fill(self):
        '''Test the fill() method.'''

        expect = '''\
This paragraph will be filled, first
without any indentation, and then with
some (including a hanging indent).'''

        result = fill(self.testString, 40)
        self.check(result, expect)


    def test_initial_indent(self):
        '''Test initial_indent parameter.'''

        expect = [
            "     This paragraph will be filled,",
            "first without any indentation, and then",
            "with some (including a hanging indent)."]

        result = wrap(self.testString, 40, initial_indent="     ")
        self.check(result, expect)

        expect = '''\
     This paragraph will be filled,
first without any indentation, and then
with some (including a hanging indent).'''

        result = fill(self.testString, 40, initial_indent="     ")
        self.check(result, expect)


    def test_subsequent_indent(self):
        '''Test subsequent_indent parameter.'''

        expect = '''\
  * This paragraph will be filled, first
    without any indentation, and then
    with some (including a hanging
    indent).'''

        result = fill(self.testString, 40, initial_indent="  * ", 
            subsequent_indent="    ")
        self.check(result, expect)


def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(WrapTestCase))
    suite.addTest(unittest.makeSuite(IndentTestCases))
    test_support.run_suite(suite)

if __name__ == '__main__':
    test_main()
