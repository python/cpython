#!/usr/bin/env python

import unittest

# import item under test
from textwrap import TextWrapper, wrap, fill


class WrapperTestCase(unittest.TestCase):
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



# Note: a new TestCase instance is created before running each
# test method.
class WrapTestCase(WrapperTestCase):

    # called before each test method
    def setUp(self):
        self.wrapper = TextWrapper(width=45, fix_sentence_endings=True)


    # Note: any methods that start with "test" are called automatically
    # by the unittest framework.

    def testSimpleCases(self):
        '''Simple case: just words, spaces, and a bit of punctuation.'''

        t = "Hello there, how are you this fine day?  I'm glad to hear it!"

        # bizarre formatting intended to increase maintainability
        subcases = [
            ( (t, 12), [
                "Hello there,",
                "how are you",
                "this fine",
                "day?  I'm",
                "glad to hear",
                "it!" 
                ] ),
            ( (t, 42), [ 
                "Hello there, how are you this fine day?",
                "I'm glad to hear it!"
                ] ),
            ( (t, 80), [ 
                t 
                ] ),
            ]

        for test, expect in subcases:
            result = wrap(*test)
            self.check(result, expect)


    def testWhitespace(self):
        '''Whitespace munging and end-of-sentence detection.'''

        t = """\
This is a paragraph that already has
line breaks.  But some of its lines are much longer than the others,
so it needs to be wrapped.
Some lines are \ttabbed too.
What a mess!
"""

        # bizarre formatting intended to increase maintainability
        expect = [
                "This is a paragraph that already has line",
                "breaks.  But some of its lines are much",
                "longer than the others, so it needs to be",
                "wrapped.  Some lines are  tabbed too.  What a",
                "mess!"
                ]

        result = self.wrapper.wrap(t)
        self.check(result, expect)

        result = self.wrapper.fill(t)
        self.check(result, '\n'.join(expect))


    def testWrappingShortToLong(self):
        '''Wrapping to make short lines longer.'''

        t = "This is a\nshort paragraph."

        # bizarre formatting intended to increase maintainability
        subcases = [
            ( (t, 20), [
                "This is a short",
                "paragraph."
                ] ),
            ( (t, 40), [
                "This is a short paragraph."
                ] ),
            ]

        for test, expect in subcases:
            result = wrap(*test)
            self.check(result, expect)


    def testHyphenated(self):
        '''Test breaking hyphenated words.'''

        t = "this-is-a-useful-feature-for-reformatting-posts-from-tim-peters'ly"

        subcases = [
            ( (t, 40), [
                "this-is-a-useful-feature-for-",
                "reformatting-posts-from-tim-peters'ly"
                ] ),
            ( (t, 41), [
                "this-is-a-useful-feature-for-",
                "reformatting-posts-from-tim-peters'ly"
                ] ),
            ( (t, 42), [
                "this-is-a-useful-feature-for-reformatting-",
                "posts-from-tim-peters'ly"
                ] ),
            ]

        for test, expect in subcases:
            result = wrap(*test)
            self.check(result, expect)


    def test_split(self):
        '''Ensure that the standard _split() method works as advertised 
           in the comments (don't you hate it when code and comments diverge?).'''

        t = "Hello there -- you goof-ball, use the -b option!"

        result = self.wrapper._split(t)
        self.check(result,
             ["Hello", " ", "there", " ", "--", " ", "you", " ", "goof-",
              "ball,", " ", "use", " ", "the", " ", "-b", " ",  "option!"])


    def testPoppins(self):
        '''Please rename this test based on its purpose.'''

        t = '''
Did you say "supercalifragilisticexpialidocious?"
How *do* you spell that odd word, anyways?
'''
        # bizarre formatting intended to increase maintainability
        subcases = [
            ( (t, 30), [
                'Did you say "supercalifragilis',
                'ticexpialidocious?" How *do*',
                'you spell that odd word,',
                'anyways?' 
                ] ),
            ( (t, 50), [
                'Did you say "supercalifragilisticexpialidocious?"',
                'How *do* you spell that odd word, anyways?'
                ] ),
            ]

        for test, expect in subcases:
            result = wrap(*test)
            self.check(result, expect)


    def testBreakLongWordsOff(self):        
        '''Test with break_long_words disabled.'''
        t = '''
Did you say "supercalifragilisticexpialidocious?"
How *do* you spell that odd word, anyways?
'''
        self.wrapper.break_long_words = 0
        self.wrapper.width = 30
        result = self.wrapper.wrap(t)
        expect = [
                'Did you say',
                '"supercalifragilisticexpialidocious?"',
                'How *do* you spell that odd',
                'word, anyways?'
                ] 
        self.check(result, expect)

        # Same thing with kwargs passed to standalone wrap() function.
        result = wrap(t, width=30, break_long_words=0)
        self.check(result, expect)



class IndentTestCases(WrapperTestCase):

    # called before each test method
    def setUp(self):
        self.testString = '''\
This paragraph will be filled, first without any indentation,
and then with some (including a hanging indent).'''


    def testFill(self):
        '''Test the fill() method.'''

        expect = '''\
This paragraph will be filled, first
without any indentation, and then with
some (including a hanging indent).'''

        result = fill(self.testString, 40)
        self.check(result, expect)


    def testInitialIndent(self):
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


    def testSubsequentIndent(self):
        '''Test subsequent_indent parameter.'''

        expect = '''\
  * This paragraph will be filled, first
    without any indentation, and then
    with some (including a hanging
    indent).'''

        result = fill(self.testString, 40, initial_indent="  * ", 
            subsequent_indent="    ")
        self.check(result, expect)


if __name__ == '__main__':
    unittest.main()