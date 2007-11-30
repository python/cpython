from test import test_support
from test.test_support import bigmemtest, _1G, _2G

import unittest
import operator
import sys

# Bigmem testing houserules:
#
#  - Try not to allocate too many large objects. It's okay to rely on
#    refcounting semantics, but don't forget that 's = create_largestring()'
#    doesn't release the old 's' (if it exists) until well after its new
#    value has been created. Use 'del s' before the create_largestring call.
#
#  - Do *not* compare large objects using assertEquals or similar. It's a
#    lengty operation and the errormessage will be utterly useless due to
#    its size. To make sure whether a result has the right contents, better
#    to use the strip or count methods, or compare meaningful slices.
#
#  - Don't forget to test for large indices, offsets and results and such,
#    in addition to large sizes.
#
#  - When repeating an object (say, a substring, or a small list) to create
#    a large object, make the subobject of a length that is not a power of
#    2. That way, int-wrapping problems are more easily detected.
#
#  - While the bigmemtest decorator speaks of 'minsize', all tests will
#    actually be called with a much smaller number too, in the normal
#    test run (5Kb currently.) This is so the tests themselves get frequent
#    testing. Consequently, always make all large allocations based on the
#    passed-in 'size', and don't rely on the size being very large. Also,
#    memuse-per-size should remain sane (less than a few thousand); if your
#    test uses more, adjust 'size' upward, instead.

class StrTest(unittest.TestCase):
    @bigmemtest(minsize=_2G, memuse=2)
    def test_capitalize(self, size):
        SUBSTR = ' abc def ghi'
        s = '-' * size + SUBSTR
        caps = s.capitalize()
        self.assertEquals(caps[-len(SUBSTR):],
                         SUBSTR.capitalize())
        self.assertEquals(caps.lstrip('-'), SUBSTR)

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_center(self, size):
        SUBSTR = ' abc def ghi'
        s = SUBSTR.center(size)
        self.assertEquals(len(s), size)
        lpadsize = rpadsize = (len(s) - len(SUBSTR)) // 2
        if len(s) % 2:
            lpadsize += 1
        self.assertEquals(s[lpadsize:-rpadsize], SUBSTR)
        self.assertEquals(s.strip(), SUBSTR.strip())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_count(self, size):
        SUBSTR = ' abc def ghi'
        s = '.' * size + SUBSTR
        self.assertEquals(s.count('.'), size)
        s += '.'
        self.assertEquals(s.count('.'), size + 1)
        self.assertEquals(s.count(' '), 3)
        self.assertEquals(s.count('i'), 1)
        self.assertEquals(s.count('j'), 0)

    @bigmemtest(minsize=_2G + 2, memuse=3)
    def test_decode(self, size):
        s = '.' * size
        self.assertEquals(len(s.decode('utf-8')), size)

    @bigmemtest(minsize=_2G + 2, memuse=3)
    def test_encode(self, size):
        s = u'.' * size
        self.assertEquals(len(s.encode('utf-8')), size)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_endswith(self, size):
        SUBSTR = ' abc def ghi'
        s = '-' * size + SUBSTR
        self.failUnless(s.endswith(SUBSTR))
        self.failUnless(s.endswith(s))
        s2 = '...' + s
        self.failUnless(s2.endswith(s))
        self.failIf(s.endswith('a' + SUBSTR))
        self.failIf(SUBSTR.endswith(s))

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_expandtabs(self, size):
        s = '-' * size
        tabsize = 8
        self.assertEquals(s.expandtabs(), s)
        del s
        slen, remainder = divmod(size, tabsize)
        s = '       \t' * slen
        s = s.expandtabs(tabsize)
        self.assertEquals(len(s), size - remainder)
        self.assertEquals(len(s.strip(' ')), 0)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_find(self, size):
        SUBSTR = ' abc def ghi'
        sublen = len(SUBSTR)
        s = ''.join([SUBSTR, '-' * size, SUBSTR])
        self.assertEquals(s.find(' '), 0)
        self.assertEquals(s.find(SUBSTR), 0)
        self.assertEquals(s.find(' ', sublen), sublen + size)
        self.assertEquals(s.find(SUBSTR, len(SUBSTR)), sublen + size)
        self.assertEquals(s.find('i'), SUBSTR.find('i'))
        self.assertEquals(s.find('i', sublen),
                         sublen + size + SUBSTR.find('i'))
        self.assertEquals(s.find('i', size),
                         sublen + size + SUBSTR.find('i'))
        self.assertEquals(s.find('j'), -1)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_index(self, size):
        SUBSTR = ' abc def ghi'
        sublen = len(SUBSTR)
        s = ''.join([SUBSTR, '-' * size, SUBSTR])
        self.assertEquals(s.index(' '), 0)
        self.assertEquals(s.index(SUBSTR), 0)
        self.assertEquals(s.index(' ', sublen), sublen + size)
        self.assertEquals(s.index(SUBSTR, sublen), sublen + size)
        self.assertEquals(s.index('i'), SUBSTR.index('i'))
        self.assertEquals(s.index('i', sublen),
                         sublen + size + SUBSTR.index('i'))
        self.assertEquals(s.index('i', size),
                         sublen + size + SUBSTR.index('i'))
        self.assertRaises(ValueError, s.index, 'j')

    @bigmemtest(minsize=_2G, memuse=2)
    def test_isalnum(self, size):
        SUBSTR = '123456'
        s = 'a' * size + SUBSTR
        self.failUnless(s.isalnum())
        s += '.'
        self.failIf(s.isalnum())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_isalpha(self, size):
        SUBSTR = 'zzzzzzz'
        s = 'a' * size + SUBSTR
        self.failUnless(s.isalpha())
        s += '.'
        self.failIf(s.isalpha())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_isdigit(self, size):
        SUBSTR = '123456'
        s = '9' * size + SUBSTR
        self.failUnless(s.isdigit())
        s += 'z'
        self.failIf(s.isdigit())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_islower(self, size):
        chars = ''.join([ chr(c) for c in range(255) if not chr(c).isupper() ])
        repeats = size // len(chars) + 2
        s = chars * repeats
        self.failUnless(s.islower())
        s += 'A'
        self.failIf(s.islower())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_isspace(self, size):
        whitespace = ' \f\n\r\t\v'
        repeats = size // len(whitespace) + 2
        s = whitespace * repeats
        self.failUnless(s.isspace())
        s += 'j'
        self.failIf(s.isspace())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_istitle(self, size):
        SUBSTR = '123456'
        s = ''.join(['A', 'a' * size, SUBSTR])
        self.failUnless(s.istitle())
        s += 'A'
        self.failUnless(s.istitle())
        s += 'aA'
        self.failIf(s.istitle())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_isupper(self, size):
        chars = ''.join([ chr(c) for c in range(255) if not chr(c).islower() ])
        repeats = size // len(chars) + 2
        s = chars * repeats
        self.failUnless(s.isupper())
        s += 'a'
        self.failIf(s.isupper())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_join(self, size):
        s = 'A' * size
        x = s.join(['aaaaa', 'bbbbb'])
        self.assertEquals(x.count('a'), 5)
        self.assertEquals(x.count('b'), 5)
        self.failUnless(x.startswith('aaaaaA'))
        self.failUnless(x.endswith('Abbbbb'))

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_ljust(self, size):
        SUBSTR = ' abc def ghi'
        s = SUBSTR.ljust(size)
        self.failUnless(s.startswith(SUBSTR + '  '))
        self.assertEquals(len(s), size)
        self.assertEquals(s.strip(), SUBSTR.strip())

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_lower(self, size):
        s = 'A' * size
        s = s.lower()
        self.assertEquals(len(s), size)
        self.assertEquals(s.count('a'), size)

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_lstrip(self, size):
        SUBSTR = 'abc def ghi'
        s = SUBSTR.rjust(size)
        self.assertEquals(len(s), size)
        self.assertEquals(s.lstrip(), SUBSTR.lstrip())
        del s
        s = SUBSTR.ljust(size)
        self.assertEquals(len(s), size)
        stripped = s.lstrip()
        self.failUnless(stripped is s)

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_replace(self, size):
        replacement = 'a'
        s = ' ' * size
        s = s.replace(' ', replacement)
        self.assertEquals(len(s), size)
        self.assertEquals(s.count(replacement), size)
        s = s.replace(replacement, ' ', size - 4)
        self.assertEquals(len(s), size)
        self.assertEquals(s.count(replacement), 4)
        self.assertEquals(s[-10:], '      aaaa')

    @bigmemtest(minsize=_2G, memuse=2)
    def test_rfind(self, size):
        SUBSTR = ' abc def ghi'
        sublen = len(SUBSTR)
        s = ''.join([SUBSTR, '-' * size, SUBSTR])
        self.assertEquals(s.rfind(' '), sublen + size + SUBSTR.rfind(' '))
        self.assertEquals(s.rfind(SUBSTR), sublen + size)
        self.assertEquals(s.rfind(' ', 0, size), SUBSTR.rfind(' '))
        self.assertEquals(s.rfind(SUBSTR, 0, sublen + size), 0)
        self.assertEquals(s.rfind('i'), sublen + size + SUBSTR.rfind('i'))
        self.assertEquals(s.rfind('i', 0, sublen), SUBSTR.rfind('i'))
        self.assertEquals(s.rfind('i', 0, sublen + size),
                          SUBSTR.rfind('i'))
        self.assertEquals(s.rfind('j'), -1)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_rindex(self, size):
        SUBSTR = ' abc def ghi'
        sublen = len(SUBSTR)
        s = ''.join([SUBSTR, '-' * size, SUBSTR])
        self.assertEquals(s.rindex(' '),
                          sublen + size + SUBSTR.rindex(' '))
        self.assertEquals(s.rindex(SUBSTR), sublen + size)
        self.assertEquals(s.rindex(' ', 0, sublen + size - 1),
                          SUBSTR.rindex(' '))
        self.assertEquals(s.rindex(SUBSTR, 0, sublen + size), 0)
        self.assertEquals(s.rindex('i'),
                          sublen + size + SUBSTR.rindex('i'))
        self.assertEquals(s.rindex('i', 0, sublen), SUBSTR.rindex('i'))
        self.assertEquals(s.rindex('i', 0, sublen + size),
                          SUBSTR.rindex('i'))
        self.assertRaises(ValueError, s.rindex, 'j')

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_rjust(self, size):
        SUBSTR = ' abc def ghi'
        s = SUBSTR.ljust(size)
        self.failUnless(s.startswith(SUBSTR + '  '))
        self.assertEquals(len(s), size)
        self.assertEquals(s.strip(), SUBSTR.strip())

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_rstrip(self, size):
        SUBSTR = ' abc def ghi'
        s = SUBSTR.ljust(size)
        self.assertEquals(len(s), size)
        self.assertEquals(s.rstrip(), SUBSTR.rstrip())
        del s
        s = SUBSTR.rjust(size)
        self.assertEquals(len(s), size)
        stripped = s.rstrip()
        self.failUnless(stripped is s)

    # The test takes about size bytes to build a string, and then about
    # sqrt(size) substrings of sqrt(size) in size and a list to
    # hold sqrt(size) items. It's close but just over 2x size.
    @bigmemtest(minsize=_2G, memuse=2.1)
    def test_split_small(self, size):
        # Crudely calculate an estimate so that the result of s.split won't
        # take up an inordinate amount of memory
        chunksize = int(size ** 0.5 + 2)
        SUBSTR = 'a' + ' ' * chunksize
        s = SUBSTR * chunksize
        l = s.split()
        self.assertEquals(len(l), chunksize)
        self.assertEquals(set(l), set(['a']))
        del l
        l = s.split('a')
        self.assertEquals(len(l), chunksize + 1)
        self.assertEquals(set(l), set(['', ' ' * chunksize]))

    # Allocates a string of twice size (and briefly two) and a list of
    # size.  Because of internal affairs, the s.split() call produces a
    # list of size times the same one-character string, so we only
    # suffer for the list size. (Otherwise, it'd cost another 48 times
    # size in bytes!) Nevertheless, a list of size takes
    # 8*size bytes.
    @bigmemtest(minsize=_2G + 5, memuse=10)
    def test_split_large(self, size):
        s = ' a' * size + ' '
        l = s.split()
        self.assertEquals(len(l), size)
        self.assertEquals(set(l), set(['a']))
        del l
        l = s.split('a')
        self.assertEquals(len(l), size + 1)
        self.assertEquals(set(l), set([' ']))

    @bigmemtest(minsize=_2G, memuse=2.1)
    def test_splitlines(self, size):
        # Crudely calculate an estimate so that the result of s.split won't
        # take up an inordinate amount of memory
        chunksize = int(size ** 0.5 + 2) // 2
        SUBSTR = ' ' * chunksize + '\n' + ' ' * chunksize + '\r\n'
        s = SUBSTR * chunksize
        l = s.splitlines()
        self.assertEquals(len(l), chunksize * 2)
        self.assertEquals(set(l), set([' ' * chunksize]))

    @bigmemtest(minsize=_2G, memuse=2)
    def test_startswith(self, size):
        SUBSTR = ' abc def ghi'
        s = '-' * size + SUBSTR
        self.failUnless(s.startswith(s))
        self.failUnless(s.startswith('-' * size))
        self.failIf(s.startswith(SUBSTR))

    @bigmemtest(minsize=_2G, memuse=1)
    def test_strip(self, size):
        SUBSTR = '   abc def ghi   '
        s = SUBSTR.rjust(size)
        self.assertEquals(len(s), size)
        self.assertEquals(s.strip(), SUBSTR.strip())
        del s
        s = SUBSTR.ljust(size)
        self.assertEquals(len(s), size)
        self.assertEquals(s.strip(), SUBSTR.strip())

    @bigmemtest(minsize=_2G, memuse=2)
    def test_swapcase(self, size):
        SUBSTR = "aBcDeFG12.'\xa9\x00"
        sublen = len(SUBSTR)
        repeats = size // sublen + 2
        s = SUBSTR * repeats
        s = s.swapcase()
        self.assertEquals(len(s), sublen * repeats)
        self.assertEquals(s[:sublen * 3], SUBSTR.swapcase() * 3)
        self.assertEquals(s[-sublen * 3:], SUBSTR.swapcase() * 3)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_title(self, size):
        SUBSTR = 'SpaaHAaaAaham'
        s = SUBSTR * (size // len(SUBSTR) + 2)
        s = s.title()
        self.failUnless(s.startswith((SUBSTR * 3).title()))
        self.failUnless(s.endswith(SUBSTR.lower() * 3))

    @bigmemtest(minsize=_2G, memuse=2)
    def test_translate(self, size):
        trans = {ord('.'):'-', ord('a'):'!', ord('Z'):'$'}
        SUBSTR = 'aZz.z.Aaz.'
        sublen = len(SUBSTR)
        repeats = size // sublen + 2
        s = SUBSTR * repeats
        s = s.translate(trans)
        self.assertEquals(len(s), repeats * sublen)
        self.assertEquals(s[:sublen], SUBSTR.translate(trans))
        self.assertEquals(s[-sublen:], SUBSTR.translate(trans))
        self.assertEquals(s.count('.'), 0)
        self.assertEquals(s.count('!'), repeats * 2)
        self.assertEquals(s.count('z'), repeats * 3)

    @bigmemtest(minsize=_2G + 5, memuse=2)
    def test_upper(self, size):
        s = 'a' * size
        s = s.upper()
        self.assertEquals(len(s), size)
        self.assertEquals(s.count('A'), size)

    @bigmemtest(minsize=_2G + 20, memuse=1)
    def test_zfill(self, size):
        SUBSTR = '-568324723598234'
        s = SUBSTR.zfill(size)
        self.failUnless(s.endswith('0' + SUBSTR[1:]))
        self.failUnless(s.startswith('-0'))
        self.assertEquals(len(s), size)
        self.assertEquals(s.count('0'), size - len(SUBSTR))

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_format(self, size):
        s = '-' * size
        sf = '%s' % (s,)
        self.assertEqual(s, sf)
        del sf
        sf = '..%s..' % (s,)
        self.assertEquals(len(sf), len(s) + 4)
        self.failUnless(sf.startswith('..-'))
        self.failUnless(sf.endswith('-..'))
        del s, sf

        size //= 2
        edge = '-' * size
        s = ''.join([edge, '%s', edge])
        del edge
        s = s % '...'
        self.assertEquals(len(s), size * 2 + 3)
        self.assertEquals(s.count('.'), 3)
        self.assertEquals(s.count('-'), size * 2)

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_repr_small(self, size):
        s = '-' * size
        s = repr(s)
        self.assertEquals(len(s), size + 2)
        self.assertEquals(s[0], "'")
        self.assertEquals(s[-1], "'")
        self.assertEquals(s.count('-'), size)
        del s
        # repr() will create a string four times as large as this 'binary
        # string', but we don't want to allocate much more than twice
        # size in total.  (We do extra testing in test_repr_large())
        size = size // 5 * 2
        s = '\x00' * size
        s = repr(s)
        self.assertEquals(len(s), size * 4 + 2)
        self.assertEquals(s[0], "'")
        self.assertEquals(s[-1], "'")
        self.assertEquals(s.count('\\'), size)
        self.assertEquals(s.count('0'), size * 2)

    @bigmemtest(minsize=_2G + 10, memuse=5)
    def test_repr_large(self, size):
        s = '\x00' * size
        s = repr(s)
        self.assertEquals(len(s), size * 4 + 2)
        self.assertEquals(s[0], "'")
        self.assertEquals(s[-1], "'")
        self.assertEquals(s.count('\\'), size)
        self.assertEquals(s.count('0'), size * 2)

    # This test is meaningful even with size < 2G, as long as the
    # doubled string is > 2G (but it tests more if both are > 2G :)
    @bigmemtest(minsize=_1G + 2, memuse=3)
    def test_concat(self, size):
        s = '.' * size
        self.assertEquals(len(s), size)
        s = s + s
        self.assertEquals(len(s), size * 2)
        self.assertEquals(s.count('.'), size * 2)

    # This test is meaningful even with size < 2G, as long as the
    # repeated string is > 2G (but it tests more if both are > 2G :)
    @bigmemtest(minsize=_1G + 2, memuse=3)
    def test_repeat(self, size):
        s = '.' * size
        self.assertEquals(len(s), size)
        s = s * 2
        self.assertEquals(len(s), size * 2)
        self.assertEquals(s.count('.'), size * 2)

    @bigmemtest(minsize=_2G + 20, memuse=1)
    def test_slice_and_getitem(self, size):
        SUBSTR = '0123456789'
        sublen = len(SUBSTR)
        s = SUBSTR * (size // sublen)
        stepsize = len(s) // 100
        stepsize = stepsize - (stepsize % sublen)
        for i in range(0, len(s) - stepsize, stepsize):
            self.assertEquals(s[i], SUBSTR[0])
            self.assertEquals(s[i:i + sublen], SUBSTR)
            self.assertEquals(s[i:i + sublen:2], SUBSTR[::2])
            if i > 0:
                self.assertEquals(s[i + sublen - 1:i - 1:-3],
                                  SUBSTR[sublen::-3])
        # Make sure we do some slicing and indexing near the end of the
        # string, too.
        self.assertEquals(s[len(s) - 1], SUBSTR[-1])
        self.assertEquals(s[-1], SUBSTR[-1])
        self.assertEquals(s[len(s) - 10], SUBSTR[0])
        self.assertEquals(s[-sublen], SUBSTR[0])
        self.assertEquals(s[len(s):], '')
        self.assertEquals(s[len(s) - 1:], SUBSTR[-1])
        self.assertEquals(s[-1:], SUBSTR[-1])
        self.assertEquals(s[len(s) - sublen:], SUBSTR)
        self.assertEquals(s[-sublen:], SUBSTR)
        self.assertEquals(len(s[:]), len(s))
        self.assertEquals(len(s[:len(s) - 5]), len(s) - 5)
        self.assertEquals(len(s[5:-5]), len(s) - 10)

        self.assertRaises(IndexError, operator.getitem, s, len(s))
        self.assertRaises(IndexError, operator.getitem, s, len(s) + 1)
        self.assertRaises(IndexError, operator.getitem, s, len(s) + 1<<31)

    @bigmemtest(minsize=_2G, memuse=2)
    def test_contains(self, size):
        SUBSTR = '0123456789'
        edge = '-' * (size // 2)
        s = ''.join([edge, SUBSTR, edge])
        del edge
        self.failUnless(SUBSTR in s)
        self.failIf(SUBSTR * 2 in s)
        self.failUnless('-' in s)
        self.failIf('a' in s)
        s += 'a'
        self.failUnless('a' in s)

    @bigmemtest(minsize=_2G + 10, memuse=2)
    def test_compare(self, size):
        s1 = '-' * size
        s2 = '-' * size
        self.assertEqual(s1, s2)
        del s2
        s2 = s1 + 'a'
        self.failIf(s1 == s2)
        del s2
        s2 = '.' * size
        self.failIf(s1 == s2)

    @bigmemtest(minsize=_2G + 10, memuse=1)
    def test_hash(self, size):
        # Not sure if we can do any meaningful tests here...  Even if we
        # start relying on the exact algorithm used, the result will be
        # different depending on the size of the C 'long int'.  Even this
        # test is dodgy (there's no *guarantee* that the two things should
        # have a different hash, even if they, in the current
        # implementation, almost always do.)
        s = '\x00' * size
        h1 = hash(s)
        del s
        s = '\x00' * (size + 1)
        self.failIf(h1 == hash(s))

class TupleTest(unittest.TestCase):

    # Tuples have a small, fixed-sized head and an array of pointers to
    # data.  Since we're testing 64-bit addressing, we can assume that the
    # pointers are 8 bytes, and that thus that the tuples take up 8 bytes
    # per size.

    # As a side-effect of testing long tuples, these tests happen to test
    # having more than 2<<31 references to any given object. Hence the
    # use of different types of objects as contents in different tests.

    @bigmemtest(minsize=_2G + 2, memuse=16)
    def test_compare(self, size):
        t1 = ('',) * size
        t2 = ('',) * size
        self.assertEqual(t1, t2)
        del t2
        t2 = ('',) * (size + 1)
        self.failIf(t1 == t2)
        del t2
        t2 = (1,) * size
        self.failIf(t1 == t2)

    # Test concatenating into a single tuple of more than 2G in length,
    # and concatenating a tuple of more than 2G in length separately, so
    # the smaller test still gets run even if there isn't memory for the
    # larger test (but we still let the tester know the larger test is
    # skipped, in verbose mode.)
    def basic_concat_test(self, size):
        t = ((),) * size
        self.assertEquals(len(t), size)
        t = t + t
        self.assertEquals(len(t), size * 2)

    @bigmemtest(minsize=_2G // 2 + 2, memuse=24)
    def test_concat_small(self, size):
        return self.basic_concat_test(size)

    @bigmemtest(minsize=_2G + 2, memuse=24)
    def test_concat_large(self, size):
        return self.basic_concat_test(size)

    @bigmemtest(minsize=_2G // 5 + 10, memuse=8 * 5)
    def test_contains(self, size):
        t = (1, 2, 3, 4, 5) * size
        self.assertEquals(len(t), size * 5)
        self.failUnless(5 in t)
        self.failIf((1, 2, 3, 4, 5) in t)
        self.failIf(0 in t)

    @bigmemtest(minsize=_2G + 10, memuse=8)
    def test_hash(self, size):
        t1 = (0,) * size
        h1 = hash(t1)
        del t1
        t2 = (0,) * (size + 1)
        self.failIf(h1 == hash(t2))

    @bigmemtest(minsize=_2G + 10, memuse=8)
    def test_index_and_slice(self, size):
        t = (None,) * size
        self.assertEquals(len(t), size)
        self.assertEquals(t[-1], None)
        self.assertEquals(t[5], None)
        self.assertEquals(t[size - 1], None)
        self.assertRaises(IndexError, operator.getitem, t, size)
        self.assertEquals(t[:5], (None,) * 5)
        self.assertEquals(t[-5:], (None,) * 5)
        self.assertEquals(t[20:25], (None,) * 5)
        self.assertEquals(t[-25:-20], (None,) * 5)
        self.assertEquals(t[size - 5:], (None,) * 5)
        self.assertEquals(t[size - 5:size], (None,) * 5)
        self.assertEquals(t[size - 6:size - 2], (None,) * 4)
        self.assertEquals(t[size:size], ())
        self.assertEquals(t[size:size+5], ())

    # Like test_concat, split in two.
    def basic_test_repeat(self, size):
        t = ('',) * size
        self.assertEquals(len(t), size)
        t = t * 2
        self.assertEquals(len(t), size * 2)

    @bigmemtest(minsize=_2G // 2 + 2, memuse=24)
    def test_repeat_small(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(minsize=_2G + 2, memuse=24)
    def test_repeat_large(self, size):
        return self.basic_test_repeat(size)

    # Like test_concat, split in two.
    def basic_test_repr(self, size):
        t = (0,) * size
        s = repr(t)
        # The repr of a tuple of 0's is exactly three times the tuple length.
        self.assertEquals(len(s), size * 3)
        self.assertEquals(s[:5], '(0, 0')
        self.assertEquals(s[-5:], '0, 0)')
        self.assertEquals(s.count('0'), size)

    @bigmemtest(minsize=_2G // 3 + 2, memuse=8 + 3)
    def test_repr_small(self, size):
        return self.basic_test_repr(size)

    @bigmemtest(minsize=_2G + 2, memuse=8 + 3)
    def test_repr_large(self, size):
        return self.basic_test_repr(size)

class ListTest(unittest.TestCase):

    # Like tuples, lists have a small, fixed-sized head and an array of
    # pointers to data, so 8 bytes per size. Also like tuples, we make the
    # lists hold references to various objects to test their refcount
    # limits.

    @bigmemtest(minsize=_2G + 2, memuse=16)
    def test_compare(self, size):
        l1 = [''] * size
        l2 = [''] * size
        self.assertEqual(l1, l2)
        del l2
        l2 = [''] * (size + 1)
        self.failIf(l1 == l2)
        del l2
        l2 = [2] * size
        self.failIf(l1 == l2)

    # Test concatenating into a single list of more than 2G in length,
    # and concatenating a list of more than 2G in length separately, so
    # the smaller test still gets run even if there isn't memory for the
    # larger test (but we still let the tester know the larger test is
    # skipped, in verbose mode.)
    def basic_test_concat(self, size):
        l = [[]] * size
        self.assertEquals(len(l), size)
        l = l + l
        self.assertEquals(len(l), size * 2)

    @bigmemtest(minsize=_2G // 2 + 2, memuse=24)
    def test_concat_small(self, size):
        return self.basic_test_concat(size)

    @bigmemtest(minsize=_2G + 2, memuse=24)
    def test_concat_large(self, size):
        return self.basic_test_concat(size)

    def basic_test_inplace_concat(self, size):
        l = [sys.stdout] * size
        l += l
        self.assertEquals(len(l), size * 2)
        self.failUnless(l[0] is l[-1])
        self.failUnless(l[size - 1] is l[size + 1])

    @bigmemtest(minsize=_2G // 2 + 2, memuse=24)
    def test_inplace_concat_small(self, size):
        return self.basic_test_inplace_concat(size)

    @bigmemtest(minsize=_2G + 2, memuse=24)
    def test_inplace_concat_large(self, size):
        return self.basic_test_inplace_concat(size)

    @bigmemtest(minsize=_2G // 5 + 10, memuse=8 * 5)
    def test_contains(self, size):
        l = [1, 2, 3, 4, 5] * size
        self.assertEquals(len(l), size * 5)
        self.failUnless(5 in l)
        self.failIf([1, 2, 3, 4, 5] in l)
        self.failIf(0 in l)

    @bigmemtest(minsize=_2G + 10, memuse=8)
    def test_hash(self, size):
        l = [0] * size
        self.failUnlessRaises(TypeError, hash, l)

    @bigmemtest(minsize=_2G + 10, memuse=8)
    def test_index_and_slice(self, size):
        l = [None] * size
        self.assertEquals(len(l), size)
        self.assertEquals(l[-1], None)
        self.assertEquals(l[5], None)
        self.assertEquals(l[size - 1], None)
        self.assertRaises(IndexError, operator.getitem, l, size)
        self.assertEquals(l[:5], [None] * 5)
        self.assertEquals(l[-5:], [None] * 5)
        self.assertEquals(l[20:25], [None] * 5)
        self.assertEquals(l[-25:-20], [None] * 5)
        self.assertEquals(l[size - 5:], [None] * 5)
        self.assertEquals(l[size - 5:size], [None] * 5)
        self.assertEquals(l[size - 6:size - 2], [None] * 4)
        self.assertEquals(l[size:size], [])
        self.assertEquals(l[size:size+5], [])

        l[size - 2] = 5
        self.assertEquals(len(l), size)
        self.assertEquals(l[-3:], [None, 5, None])
        self.assertEquals(l.count(5), 1)
        self.assertRaises(IndexError, operator.setitem, l, size, 6)
        self.assertEquals(len(l), size)

        l[size - 7:] = [1, 2, 3, 4, 5]
        size -= 2
        self.assertEquals(len(l), size)
        self.assertEquals(l[-7:], [None, None, 1, 2, 3, 4, 5])

        l[:7] = [1, 2, 3, 4, 5]
        size -= 2
        self.assertEquals(len(l), size)
        self.assertEquals(l[:7], [1, 2, 3, 4, 5, None, None])

        del l[size - 1]
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[-1], 4)

        del l[-2:]
        size -= 2
        self.assertEquals(len(l), size)
        self.assertEquals(l[-1], 2)

        del l[0]
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[0], 2)

        del l[:2]
        size -= 2
        self.assertEquals(len(l), size)
        self.assertEquals(l[0], 4)

    # Like test_concat, split in two.
    def basic_test_repeat(self, size):
        l = [] * size
        self.failIf(l)
        l = [''] * size
        self.assertEquals(len(l), size)
        l = l * 2
        self.assertEquals(len(l), size * 2)

    @bigmemtest(minsize=_2G // 2 + 2, memuse=24)
    def test_repeat_small(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(minsize=_2G + 2, memuse=24)
    def test_repeat_large(self, size):
        return self.basic_test_repeat(size)

    def basic_test_inplace_repeat(self, size):
        l = ['']
        l *= size
        self.assertEquals(len(l), size)
        self.failUnless(l[0] is l[-1])
        del l

        l = [''] * size
        l *= 2
        self.assertEquals(len(l), size * 2)
        self.failUnless(l[size - 1] is l[-1])

    @bigmemtest(minsize=_2G // 2 + 2, memuse=16)
    def test_inplace_repeat_small(self, size):
        return self.basic_test_inplace_repeat(size)

    @bigmemtest(minsize=_2G + 2, memuse=16)
    def test_inplace_repeat_large(self, size):
        return self.basic_test_inplace_repeat(size)

    def basic_test_repr(self, size):
        l = [0] * size
        s = repr(l)
        # The repr of a list of 0's is exactly three times the list length.
        self.assertEquals(len(s), size * 3)
        self.assertEquals(s[:5], '[0, 0')
        self.assertEquals(s[-5:], '0, 0]')
        self.assertEquals(s.count('0'), size)

    @bigmemtest(minsize=_2G // 3 + 2, memuse=8 + 3)
    def test_repr_small(self, size):
        return self.basic_test_repr(size)

    @bigmemtest(minsize=_2G + 2, memuse=8 + 3)
    def test_repr_large(self, size):
        return self.basic_test_repr(size)

    # list overallocates ~1/8th of the total size (on first expansion) so
    # the single list.append call puts memuse at 9 bytes per size.
    @bigmemtest(minsize=_2G, memuse=9)
    def test_append(self, size):
        l = [object()] * size
        l.append(object())
        self.assertEquals(len(l), size+1)
        self.failUnless(l[-3] is l[-2])
        self.failIf(l[-2] is l[-1])

    @bigmemtest(minsize=_2G // 5 + 2, memuse=8 * 5)
    def test_count(self, size):
        l = [1, 2, 3, 4, 5] * size
        self.assertEquals(l.count(1), size)
        self.assertEquals(l.count("1"), 0)

    def basic_test_extend(self, size):
        l = [object] * size
        l.extend(l)
        self.assertEquals(len(l), size * 2)
        self.failUnless(l[0] is l[-1])
        self.failUnless(l[size - 1] is l[size + 1])

    @bigmemtest(minsize=_2G // 2 + 2, memuse=16)
    def test_extend_small(self, size):
        return self.basic_test_extend(size)

    @bigmemtest(minsize=_2G + 2, memuse=16)
    def test_extend_large(self, size):
        return self.basic_test_extend(size)

    @bigmemtest(minsize=_2G // 5 + 2, memuse=8 * 5)
    def test_index(self, size):
        l = [1, 2, 3, 4, 5] * size
        size *= 5
        self.assertEquals(l.index(1), 0)
        self.assertEquals(l.index(5, size - 5), size - 1)
        self.assertEquals(l.index(5, size - 5, size), size - 1)
        self.assertRaises(ValueError, l.index, 1, size - 4, size)
        self.assertRaises(ValueError, l.index, 6)

    # This tests suffers from overallocation, just like test_append.
    @bigmemtest(minsize=_2G + 10, memuse=9)
    def test_insert(self, size):
        l = [1.0] * size
        l.insert(size - 1, "A")
        size += 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[-3:], [1.0, "A", 1.0])

        l.insert(size + 1, "B")
        size += 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[-3:], ["A", 1.0, "B"])

        l.insert(1, "C")
        size += 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[:3], [1.0, "C", 1.0])
        self.assertEquals(l[size - 3:], ["A", 1.0, "B"])

    @bigmemtest(minsize=_2G // 5 + 4, memuse=8 * 5)
    def test_pop(self, size):
        l = ["a", "b", "c", "d", "e"] * size
        size *= 5
        self.assertEquals(len(l), size)

        item = l.pop()
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(item, "e")
        self.assertEquals(l[-2:], ["c", "d"])

        item = l.pop(0)
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(item, "a")
        self.assertEquals(l[:2], ["b", "c"])

        item = l.pop(size - 2)
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(item, "c")
        self.assertEquals(l[-2:], ["b", "d"])

    @bigmemtest(minsize=_2G + 10, memuse=8)
    def test_remove(self, size):
        l = [10] * size
        self.assertEquals(len(l), size)

        l.remove(10)
        size -= 1
        self.assertEquals(len(l), size)

        # Because of the earlier l.remove(), this append doesn't trigger
        # a resize.
        l.append(5)
        size += 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[-2:], [10, 5])
        l.remove(5)
        size -= 1
        self.assertEquals(len(l), size)
        self.assertEquals(l[-2:], [10, 10])

    @bigmemtest(minsize=_2G // 5 + 2, memuse=8 * 5)
    def test_reverse(self, size):
        l = [1, 2, 3, 4, 5] * size
        l.reverse()
        self.assertEquals(len(l), size * 5)
        self.assertEquals(l[-5:], [5, 4, 3, 2, 1])
        self.assertEquals(l[:5], [5, 4, 3, 2, 1])

    @bigmemtest(minsize=_2G // 5 + 2, memuse=8 * 5)
    def test_sort(self, size):
        l = [1, 2, 3, 4, 5] * size
        l.sort()
        self.assertEquals(len(l), size * 5)
        self.assertEquals(l.count(1), size)
        self.assertEquals(l[:10], [1] * 10)
        self.assertEquals(l[-10:], [5] * 10)

def test_main():
    test_support.run_unittest(StrTest, TupleTest, ListTest)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        test_support.set_memlimit(sys.argv[1])
    test_main()
