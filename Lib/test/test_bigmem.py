"""Bigmem tests - tests for the 32-bit boundary in containers.

These tests try to exercise the 32-bit boundary that is sometimes, if
rarely, exceeded in practice, but almost never tested.  They are really only
meaningful on 64-bit builds on machines with a *lot* of memory, but the
tests are always run, usually with very low memory limits to make sure the
tests themselves don't suffer from bitrot.  To run them for real, pass a
high memory limit to regrtest, with the -M option.
"""

from test import support
from test.support import bigmemtest, _1G, _2G, _4G, import_helper
_testcapi = import_helper.import_module('_testcapi')

import unittest
import operator
import sys

# These tests all use one of the bigmemtest decorators to indicate how much
# memory they use and how much memory they need to be even meaningful.  The
# decorators take two arguments: a 'memuse' indicator declaring
# (approximate) bytes per size-unit the test will use (at peak usage), and a
# 'minsize' indicator declaring a minimum *useful* size.  A test that
# allocates a bytestring to test various operations near the end will have a
# minsize of at least 2Gb (or it wouldn't reach the 32-bit limit, so the
# test wouldn't be very useful) and a memuse of 1 (one byte per size-unit,
# if it allocates only one big string at a time.)
#
# When run with a memory limit set, both decorators skip tests that need
# more memory than available to be meaningful.  The precisionbigmemtest will
# always pass minsize as size, even if there is much more memory available.
# The bigmemtest decorator will scale size upward to fill available memory.
#
# Bigmem testing houserules:
#
#  - Try not to allocate too many large objects. It's okay to rely on
#    refcounting semantics, and don't forget that 's = create_largestring()'
#    doesn't release the old 's' (if it exists) until well after its new
#    value has been created. Use 'del s' before the create_largestring call.
#
#  - Do *not* compare large objects using assertEqual, assertIn or similar.
#    It's a lengthy operation and the errormessage will be utterly useless
#    due to its size.  To make sure whether a result has the right contents,
#    better to use the strip or count methods, or compare meaningful slices.
#
#  - Don't forget to test for large indices, offsets and results and such,
#    in addition to large sizes. Anything that probes the 32-bit boundary.
#
#  - When repeating an object (say, a substring, or a small list) to create
#    a large object, make the subobject of a length that is not a power of
#    2. That way, int-wrapping problems are more easily detected.
#
#  - Despite the bigmemtest decorator, all tests will actually be called
#    with a much smaller number too, in the normal test run (5Kb currently.)
#    This is so the tests themselves get frequent testing.
#    Consequently, always make all large allocations based on the
#    passed-in 'size', and don't rely on the size being very large. Also,
#    memuse-per-size should remain sane (less than a few thousand); if your
#    test uses more, adjust 'size' upward, instead.

# BEWARE: it seems that one failing test can yield other subsequent tests to
# fail as well. I do not know whether it is due to memory fragmentation
# issues, or other specifics of the platform malloc() routine.

ascii_char_size = 1
ucs2_char_size = 2
ucs4_char_size = 4
pointer_size = 4 if sys.maxsize < 2**32 else 8


class BaseStrTest:

    def _test_capitalize(self, size):
        _ = self.from_latin1
        SUBSTR = self.from_latin1(' abc def ghi')
        s = _('-') * size + SUBSTR
        caps = s.capitalize()
        self.assertEqual(caps[-len(SUBSTR):],
                         SUBSTR.capitalize())
        self.assertEqual(caps.lstrip(_('-')), SUBSTR)

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_center(self, size):
        SUBSTR = self.from_latin1(' abc def ghi')
        s = SUBSTR.center(size)
        self.assertEqual(len(s), size)
        lpadsize = rpadsize = (len(s) - len(SUBSTR)) // 2
        if len(s) % 2:
            lpadsize += 1
        self.assertEqual(s[lpadsize:-rpadsize], SUBSTR)
        self.assertEqual(s.strip(), SUBSTR.strip())

    @bigmemtest(size=_2G, memuse=2)
    def test_count(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = _('.') * size + SUBSTR
        self.assertEqual(s.count(_('.')), size)
        s += _('.')
        self.assertEqual(s.count(_('.')), size + 1)
        self.assertEqual(s.count(_(' ')), 3)
        self.assertEqual(s.count(_('i')), 1)
        self.assertEqual(s.count(_('j')), 0)

    @bigmemtest(size=_2G, memuse=2)
    def test_endswith(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = _('-') * size + SUBSTR
        self.assertTrue(s.endswith(SUBSTR))
        self.assertTrue(s.endswith(s))
        s2 = _('...') + s
        self.assertTrue(s2.endswith(s))
        self.assertFalse(s.endswith(_('a') + SUBSTR))
        self.assertFalse(SUBSTR.endswith(s))

    @bigmemtest(size=_2G + 10, memuse=2)
    def test_expandtabs(self, size):
        _ = self.from_latin1
        s = _('-') * size
        tabsize = 8
        self.assertTrue(s.expandtabs() == s)
        del s
        slen, remainder = divmod(size, tabsize)
        s = _('       \t') * slen
        s = s.expandtabs(tabsize)
        self.assertEqual(len(s), size - remainder)
        self.assertEqual(len(s.strip(_(' '))), 0)

    @bigmemtest(size=_2G, memuse=2)
    def test_find(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        sublen = len(SUBSTR)
        s = _('').join([SUBSTR, _('-') * size, SUBSTR])
        self.assertEqual(s.find(_(' ')), 0)
        self.assertEqual(s.find(SUBSTR), 0)
        self.assertEqual(s.find(_(' '), sublen), sublen + size)
        self.assertEqual(s.find(SUBSTR, len(SUBSTR)), sublen + size)
        self.assertEqual(s.find(_('i')), SUBSTR.find(_('i')))
        self.assertEqual(s.find(_('i'), sublen),
                         sublen + size + SUBSTR.find(_('i')))
        self.assertEqual(s.find(_('i'), size),
                         sublen + size + SUBSTR.find(_('i')))
        self.assertEqual(s.find(_('j')), -1)

    @bigmemtest(size=_2G, memuse=2)
    def test_index(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        sublen = len(SUBSTR)
        s = _('').join([SUBSTR, _('-') * size, SUBSTR])
        self.assertEqual(s.index(_(' ')), 0)
        self.assertEqual(s.index(SUBSTR), 0)
        self.assertEqual(s.index(_(' '), sublen), sublen + size)
        self.assertEqual(s.index(SUBSTR, sublen), sublen + size)
        self.assertEqual(s.index(_('i')), SUBSTR.index(_('i')))
        self.assertEqual(s.index(_('i'), sublen),
                         sublen + size + SUBSTR.index(_('i')))
        self.assertEqual(s.index(_('i'), size),
                         sublen + size + SUBSTR.index(_('i')))
        self.assertRaises(ValueError, s.index, _('j'))

    @bigmemtest(size=_2G, memuse=2)
    def test_isalnum(self, size):
        _ = self.from_latin1
        SUBSTR = _('123456')
        s = _('a') * size + SUBSTR
        self.assertTrue(s.isalnum())
        s += _('.')
        self.assertFalse(s.isalnum())

    @bigmemtest(size=_2G, memuse=2)
    def test_isalpha(self, size):
        _ = self.from_latin1
        SUBSTR = _('zzzzzzz')
        s = _('a') * size + SUBSTR
        self.assertTrue(s.isalpha())
        s += _('.')
        self.assertFalse(s.isalpha())

    @bigmemtest(size=_2G, memuse=2)
    def test_isdigit(self, size):
        _ = self.from_latin1
        SUBSTR = _('123456')
        s = _('9') * size + SUBSTR
        self.assertTrue(s.isdigit())
        s += _('z')
        self.assertFalse(s.isdigit())

    @bigmemtest(size=_2G, memuse=2)
    def test_islower(self, size):
        _ = self.from_latin1
        chars = _(''.join(
            chr(c) for c in range(255) if not chr(c).isupper()))
        repeats = size // len(chars) + 2
        s = chars * repeats
        self.assertTrue(s.islower())
        s += _('A')
        self.assertFalse(s.islower())

    @bigmemtest(size=_2G, memuse=2)
    def test_isspace(self, size):
        _ = self.from_latin1
        whitespace = _(' \f\n\r\t\v')
        repeats = size // len(whitespace) + 2
        s = whitespace * repeats
        self.assertTrue(s.isspace())
        s += _('j')
        self.assertFalse(s.isspace())

    @bigmemtest(size=_2G, memuse=2)
    def test_istitle(self, size):
        _ = self.from_latin1
        SUBSTR = _('123456')
        s = _('').join([_('A'), _('a') * size, SUBSTR])
        self.assertTrue(s.istitle())
        s += _('A')
        self.assertTrue(s.istitle())
        s += _('aA')
        self.assertFalse(s.istitle())

    @bigmemtest(size=_2G, memuse=2)
    def test_isupper(self, size):
        _ = self.from_latin1
        chars = _(''.join(
            chr(c) for c in range(255) if not chr(c).islower()))
        repeats = size // len(chars) + 2
        s = chars * repeats
        self.assertTrue(s.isupper())
        s += _('a')
        self.assertFalse(s.isupper())

    @bigmemtest(size=_2G, memuse=2)
    def test_join(self, size):
        _ = self.from_latin1
        s = _('A') * size
        x = s.join([_('aaaaa'), _('bbbbb')])
        self.assertEqual(x.count(_('a')), 5)
        self.assertEqual(x.count(_('b')), 5)
        self.assertTrue(x.startswith(_('aaaaaA')))
        self.assertTrue(x.endswith(_('Abbbbb')))

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_ljust(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = SUBSTR.ljust(size)
        self.assertTrue(s.startswith(SUBSTR + _('  ')))
        self.assertEqual(len(s), size)
        self.assertEqual(s.strip(), SUBSTR.strip())

    @bigmemtest(size=_2G + 10, memuse=2)
    def test_lower(self, size):
        _ = self.from_latin1
        s = _('A') * size
        s = s.lower()
        self.assertEqual(len(s), size)
        self.assertEqual(s.count(_('a')), size)

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_lstrip(self, size):
        _ = self.from_latin1
        SUBSTR = _('abc def ghi')
        s = SUBSTR.rjust(size)
        self.assertEqual(len(s), size)
        self.assertEqual(s.lstrip(), SUBSTR.lstrip())
        del s
        s = SUBSTR.ljust(size)
        self.assertEqual(len(s), size)
        # Type-specific optimization
        if isinstance(s, (str, bytes)):
            stripped = s.lstrip()
            self.assertTrue(stripped is s)

    @bigmemtest(size=_2G + 10, memuse=2)
    def test_replace(self, size):
        _ = self.from_latin1
        replacement = _('a')
        s = _(' ') * size
        s = s.replace(_(' '), replacement)
        self.assertEqual(len(s), size)
        self.assertEqual(s.count(replacement), size)
        s = s.replace(replacement, _(' '), size - 4)
        self.assertEqual(len(s), size)
        self.assertEqual(s.count(replacement), 4)
        self.assertEqual(s[-10:], _('      aaaa'))

    @bigmemtest(size=_2G, memuse=2)
    def test_rfind(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        sublen = len(SUBSTR)
        s = _('').join([SUBSTR, _('-') * size, SUBSTR])
        self.assertEqual(s.rfind(_(' ')), sublen + size + SUBSTR.rfind(_(' ')))
        self.assertEqual(s.rfind(SUBSTR), sublen + size)
        self.assertEqual(s.rfind(_(' '), 0, size), SUBSTR.rfind(_(' ')))
        self.assertEqual(s.rfind(SUBSTR, 0, sublen + size), 0)
        self.assertEqual(s.rfind(_('i')), sublen + size + SUBSTR.rfind(_('i')))
        self.assertEqual(s.rfind(_('i'), 0, sublen), SUBSTR.rfind(_('i')))
        self.assertEqual(s.rfind(_('i'), 0, sublen + size),
                         SUBSTR.rfind(_('i')))
        self.assertEqual(s.rfind(_('j')), -1)

    @bigmemtest(size=_2G, memuse=2)
    def test_rindex(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        sublen = len(SUBSTR)
        s = _('').join([SUBSTR, _('-') * size, SUBSTR])
        self.assertEqual(s.rindex(_(' ')),
                         sublen + size + SUBSTR.rindex(_(' ')))
        self.assertEqual(s.rindex(SUBSTR), sublen + size)
        self.assertEqual(s.rindex(_(' '), 0, sublen + size - 1),
                         SUBSTR.rindex(_(' ')))
        self.assertEqual(s.rindex(SUBSTR, 0, sublen + size), 0)
        self.assertEqual(s.rindex(_('i')),
                         sublen + size + SUBSTR.rindex(_('i')))
        self.assertEqual(s.rindex(_('i'), 0, sublen), SUBSTR.rindex(_('i')))
        self.assertEqual(s.rindex(_('i'), 0, sublen + size),
                         SUBSTR.rindex(_('i')))
        self.assertRaises(ValueError, s.rindex, _('j'))

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_rjust(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = SUBSTR.ljust(size)
        self.assertTrue(s.startswith(SUBSTR + _('  ')))
        self.assertEqual(len(s), size)
        self.assertEqual(s.strip(), SUBSTR.strip())

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_rstrip(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = SUBSTR.ljust(size)
        self.assertEqual(len(s), size)
        self.assertEqual(s.rstrip(), SUBSTR.rstrip())
        del s
        s = SUBSTR.rjust(size)
        self.assertEqual(len(s), size)
        # Type-specific optimization
        if isinstance(s, (str, bytes)):
            stripped = s.rstrip()
            self.assertTrue(stripped is s)

    # The test takes about size bytes to build a string, and then about
    # sqrt(size) substrings of sqrt(size) in size and a list to
    # hold sqrt(size) items. It's close but just over 2x size.
    @bigmemtest(size=_2G, memuse=2.1)
    def test_split_small(self, size):
        _ = self.from_latin1
        # Crudely calculate an estimate so that the result of s.split won't
        # take up an inordinate amount of memory
        chunksize = int(size ** 0.5 + 2)
        SUBSTR = _('a') + _(' ') * chunksize
        s = SUBSTR * chunksize
        l = s.split()
        self.assertEqual(len(l), chunksize)
        expected = _('a')
        for item in l:
            self.assertEqual(item, expected)
        del l
        l = s.split(_('a'))
        self.assertEqual(len(l), chunksize + 1)
        expected = _(' ') * chunksize
        for item in filter(None, l):
            self.assertEqual(item, expected)

    # Allocates a string of twice size (and briefly two) and a list of
    # size.  Because of internal affairs, the s.split() call produces a
    # list of size times the same one-character string, so we only
    # suffer for the list size. (Otherwise, it'd cost another 48 times
    # size in bytes!) Nevertheless, a list of size takes
    # 8*size bytes.
    @bigmemtest(size=_2G + 5, memuse=ascii_char_size * 2 + pointer_size)
    def test_split_large(self, size):
        _ = self.from_latin1
        s = _(' a') * size + _(' ')
        l = s.split()
        self.assertEqual(len(l), size)
        self.assertEqual(set(l), set([_('a')]))
        del l
        l = s.split(_('a'))
        self.assertEqual(len(l), size + 1)
        self.assertEqual(set(l), set([_(' ')]))

    @bigmemtest(size=_2G, memuse=2.1)
    def test_splitlines(self, size):
        _ = self.from_latin1
        # Crudely calculate an estimate so that the result of s.split won't
        # take up an inordinate amount of memory
        chunksize = int(size ** 0.5 + 2) // 2
        SUBSTR = _(' ') * chunksize + _('\n') + _(' ') * chunksize + _('\r\n')
        s = SUBSTR * (chunksize * 2)
        l = s.splitlines()
        self.assertEqual(len(l), chunksize * 4)
        expected = _(' ') * chunksize
        for item in l:
            self.assertEqual(item, expected)

    @bigmemtest(size=_2G, memuse=2)
    def test_startswith(self, size):
        _ = self.from_latin1
        SUBSTR = _(' abc def ghi')
        s = _('-') * size + SUBSTR
        self.assertTrue(s.startswith(s))
        self.assertTrue(s.startswith(_('-') * size))
        self.assertFalse(s.startswith(SUBSTR))

    @bigmemtest(size=_2G, memuse=1)
    def test_strip(self, size):
        _ = self.from_latin1
        SUBSTR = _('   abc def ghi   ')
        s = SUBSTR.rjust(size)
        self.assertEqual(len(s), size)
        self.assertEqual(s.strip(), SUBSTR.strip())
        del s
        s = SUBSTR.ljust(size)
        self.assertEqual(len(s), size)
        self.assertEqual(s.strip(), SUBSTR.strip())

    def _test_swapcase(self, size):
        _ = self.from_latin1
        SUBSTR = _("aBcDeFG12.'\xa9\x00")
        sublen = len(SUBSTR)
        repeats = size // sublen + 2
        s = SUBSTR * repeats
        s = s.swapcase()
        self.assertEqual(len(s), sublen * repeats)
        self.assertEqual(s[:sublen * 3], SUBSTR.swapcase() * 3)
        self.assertEqual(s[-sublen * 3:], SUBSTR.swapcase() * 3)

    def _test_title(self, size):
        _ = self.from_latin1
        SUBSTR = _('SpaaHAaaAaham')
        s = SUBSTR * (size // len(SUBSTR) + 2)
        s = s.title()
        self.assertTrue(s.startswith((SUBSTR * 3).title()))
        self.assertTrue(s.endswith(SUBSTR.lower() * 3))

    @bigmemtest(size=_2G, memuse=2)
    def test_translate(self, size):
        _ = self.from_latin1
        SUBSTR = _('aZz.z.Aaz.')
        trans = bytes.maketrans(b'.aZ', b'-!$')
        sublen = len(SUBSTR)
        repeats = size // sublen + 2
        s = SUBSTR * repeats
        s = s.translate(trans)
        self.assertEqual(len(s), repeats * sublen)
        self.assertEqual(s[:sublen], SUBSTR.translate(trans))
        self.assertEqual(s[-sublen:], SUBSTR.translate(trans))
        self.assertEqual(s.count(_('.')), 0)
        self.assertEqual(s.count(_('!')), repeats * 2)
        self.assertEqual(s.count(_('z')), repeats * 3)

    @bigmemtest(size=_2G + 5, memuse=2)
    def test_upper(self, size):
        _ = self.from_latin1
        s = _('a') * size
        s = s.upper()
        self.assertEqual(len(s), size)
        self.assertEqual(s.count(_('A')), size)

    @bigmemtest(size=_2G + 20, memuse=1)
    def test_zfill(self, size):
        _ = self.from_latin1
        SUBSTR = _('-568324723598234')
        s = SUBSTR.zfill(size)
        self.assertTrue(s.endswith(_('0') + SUBSTR[1:]))
        self.assertTrue(s.startswith(_('-0')))
        self.assertEqual(len(s), size)
        self.assertEqual(s.count(_('0')), size - len(SUBSTR))

    # This test is meaningful even with size < 2G, as long as the
    # doubled string is > 2G (but it tests more if both are > 2G :)
    @bigmemtest(size=_1G + 2, memuse=3)
    def test_concat(self, size):
        _ = self.from_latin1
        s = _('.') * size
        self.assertEqual(len(s), size)
        s = s + s
        self.assertEqual(len(s), size * 2)
        self.assertEqual(s.count(_('.')), size * 2)

    # This test is meaningful even with size < 2G, as long as the
    # repeated string is > 2G (but it tests more if both are > 2G :)
    @bigmemtest(size=_1G + 2, memuse=3)
    def test_repeat(self, size):
        _ = self.from_latin1
        s = _('.') * size
        self.assertEqual(len(s), size)
        s = s * 2
        self.assertEqual(len(s), size * 2)
        self.assertEqual(s.count(_('.')), size * 2)

    @bigmemtest(size=_2G + 20, memuse=2)
    def test_slice_and_getitem(self, size):
        _ = self.from_latin1
        SUBSTR = _('0123456789')
        sublen = len(SUBSTR)
        s = SUBSTR * (size // sublen)
        stepsize = len(s) // 100
        stepsize = stepsize - (stepsize % sublen)
        for i in range(0, len(s) - stepsize, stepsize):
            self.assertEqual(s[i], SUBSTR[0])
            self.assertEqual(s[i:i + sublen], SUBSTR)
            self.assertEqual(s[i:i + sublen:2], SUBSTR[::2])
            if i > 0:
                self.assertEqual(s[i + sublen - 1:i - 1:-3],
                                 SUBSTR[sublen::-3])
        # Make sure we do some slicing and indexing near the end of the
        # string, too.
        self.assertEqual(s[len(s) - 1], SUBSTR[-1])
        self.assertEqual(s[-1], SUBSTR[-1])
        self.assertEqual(s[len(s) - 10], SUBSTR[0])
        self.assertEqual(s[-sublen], SUBSTR[0])
        self.assertEqual(s[len(s):], _(''))
        self.assertEqual(s[len(s) - 1:], SUBSTR[-1:])
        self.assertEqual(s[-1:], SUBSTR[-1:])
        self.assertEqual(s[len(s) - sublen:], SUBSTR)
        self.assertEqual(s[-sublen:], SUBSTR)
        self.assertEqual(len(s[:]), len(s))
        self.assertEqual(len(s[:len(s) - 5]), len(s) - 5)
        self.assertEqual(len(s[5:-5]), len(s) - 10)

        self.assertRaises(IndexError, operator.getitem, s, len(s))
        self.assertRaises(IndexError, operator.getitem, s, len(s) + 1)
        self.assertRaises(IndexError, operator.getitem, s, len(s) + 1<<31)

    @bigmemtest(size=_2G, memuse=2)
    def test_contains(self, size):
        _ = self.from_latin1
        SUBSTR = _('0123456789')
        edge = _('-') * (size // 2)
        s = _('').join([edge, SUBSTR, edge])
        del edge
        self.assertTrue(SUBSTR in s)
        self.assertFalse(SUBSTR * 2 in s)
        self.assertTrue(_('-') in s)
        self.assertFalse(_('a') in s)
        s += _('a')
        self.assertTrue(_('a') in s)

    @bigmemtest(size=_2G + 10, memuse=2)
    def test_compare(self, size):
        _ = self.from_latin1
        s1 = _('-') * size
        s2 = _('-') * size
        self.assertTrue(s1 == s2)
        del s2
        s2 = s1 + _('a')
        self.assertFalse(s1 == s2)
        del s2
        s2 = _('.') * size
        self.assertFalse(s1 == s2)

    @bigmemtest(size=_2G + 10, memuse=1)
    def test_hash(self, size):
        # Not sure if we can do any meaningful tests here...  Even if we
        # start relying on the exact algorithm used, the result will be
        # different depending on the size of the C 'long int'.  Even this
        # test is dodgy (there's no *guarantee* that the two things should
        # have a different hash, even if they, in the current
        # implementation, almost always do.)
        _ = self.from_latin1
        s = _('\x00') * size
        h1 = hash(s)
        del s
        s = _('\x00') * (size + 1)
        self.assertNotEqual(h1, hash(s))


class StrTest(unittest.TestCase, BaseStrTest):

    def from_latin1(self, s):
        return s

    def basic_encode_test(self, size, enc, c='.', expectedsize=None):
        if expectedsize is None:
            expectedsize = size
        try:
            s = c * size
            self.assertEqual(len(s.encode(enc)), expectedsize)
        finally:
            s = None

    def setUp(self):
        # HACK: adjust memory use of tests inherited from BaseStrTest
        # according to character size.
        self._adjusted = {}
        for name in dir(BaseStrTest):
            if not name.startswith('test_'):
                continue
            meth = getattr(type(self), name)
            try:
                memuse = meth.memuse
            except AttributeError:
                continue
            meth.memuse = ascii_char_size * memuse
            self._adjusted[name] = memuse

    def tearDown(self):
        for name, memuse in self._adjusted.items():
            getattr(type(self), name).memuse = memuse

    @bigmemtest(size=_2G, memuse=ucs4_char_size * 3 + ascii_char_size * 2)
    def test_capitalize(self, size):
        self._test_capitalize(size)

    @bigmemtest(size=_2G, memuse=ucs4_char_size * 3 + ascii_char_size * 2)
    def test_title(self, size):
        self._test_title(size)

    @bigmemtest(size=_2G, memuse=ucs4_char_size * 3 + ascii_char_size * 2)
    def test_swapcase(self, size):
        self._test_swapcase(size)

    # Many codecs convert to the legacy representation first, explaining
    # why we add 'ucs4_char_size' to the 'memuse' below.

    @bigmemtest(size=_2G + 2, memuse=ascii_char_size + 1)
    def test_encode(self, size):
        return self.basic_encode_test(size, 'utf-8')

    @bigmemtest(size=_4G // 6 + 2, memuse=ascii_char_size + ucs4_char_size + 1)
    def test_encode_raw_unicode_escape(self, size):
        try:
            return self.basic_encode_test(size, 'raw_unicode_escape')
        except MemoryError:
            pass # acceptable on 32-bit

    @bigmemtest(size=_4G // 5 + 70, memuse=ascii_char_size + 8 + 1)
    def test_encode_utf7(self, size):
        try:
            return self.basic_encode_test(size, 'utf7')
        except MemoryError:
            pass # acceptable on 32-bit

    @bigmemtest(size=_4G // 4 + 5, memuse=ascii_char_size + ucs4_char_size + 4)
    def test_encode_utf32(self, size):
        try:
            return self.basic_encode_test(size, 'utf32', expectedsize=4 * size + 4)
        except MemoryError:
            pass # acceptable on 32-bit

    @bigmemtest(size=_2G - 1, memuse=ascii_char_size + 1)
    def test_encode_ascii(self, size):
        return self.basic_encode_test(size, 'ascii', c='A')

    # str % (...) uses a Py_UCS4 intermediate representation

    @bigmemtest(size=_2G + 10, memuse=ascii_char_size * 2 + ucs4_char_size)
    def test_format(self, size):
        s = '-' * size
        sf = '%s' % (s,)
        self.assertTrue(s == sf)
        del sf
        sf = '..%s..' % (s,)
        self.assertEqual(len(sf), len(s) + 4)
        self.assertTrue(sf.startswith('..-'))
        self.assertTrue(sf.endswith('-..'))
        del s, sf

        size //= 2
        edge = '-' * size
        s = ''.join([edge, '%s', edge])
        del edge
        s = s % '...'
        self.assertEqual(len(s), size * 2 + 3)
        self.assertEqual(s.count('.'), 3)
        self.assertEqual(s.count('-'), size * 2)

    @bigmemtest(size=_2G + 10, memuse=ascii_char_size * 2)
    def test_repr_small(self, size):
        s = '-' * size
        s = repr(s)
        self.assertEqual(len(s), size + 2)
        self.assertEqual(s[0], "'")
        self.assertEqual(s[-1], "'")
        self.assertEqual(s.count('-'), size)
        del s
        # repr() will create a string four times as large as this 'binary
        # string', but we don't want to allocate much more than twice
        # size in total.  (We do extra testing in test_repr_large())
        size = size // 5 * 2
        s = '\x00' * size
        s = repr(s)
        self.assertEqual(len(s), size * 4 + 2)
        self.assertEqual(s[0], "'")
        self.assertEqual(s[-1], "'")
        self.assertEqual(s.count('\\'), size)
        self.assertEqual(s.count('0'), size * 2)

    @bigmemtest(size=_2G + 10, memuse=ascii_char_size * 5)
    def test_repr_large(self, size):
        s = '\x00' * size
        s = repr(s)
        self.assertEqual(len(s), size * 4 + 2)
        self.assertEqual(s[0], "'")
        self.assertEqual(s[-1], "'")
        self.assertEqual(s.count('\\'), size)
        self.assertEqual(s.count('0'), size * 2)

    # ascii() calls encode('ascii', 'backslashreplace'), which itself
    # creates a temporary Py_UNICODE representation in addition to the
    # original (Py_UCS2) one
    # There's also some overallocation when resizing the ascii() result
    # that isn't taken into account here.
    @bigmemtest(size=_2G // 5 + 1, memuse=ucs2_char_size +
                                          ucs4_char_size + ascii_char_size * 6)
    def test_unicode_repr(self, size):
        # Use an assigned, but not printable code point.
        # It is in the range of the low surrogates \uDC00-\uDFFF.
        char = "\uDCBA"
        s = char * size
        try:
            for f in (repr, ascii):
                r = f(s)
                self.assertEqual(len(r), 2 + (len(f(char)) - 2) * size)
                self.assertTrue(r.endswith(r"\udcba'"), r[-10:])
                r = None
        finally:
            r = s = None

    @bigmemtest(size=_2G // 5 + 1, memuse=ucs4_char_size * 2 + ascii_char_size * 10)
    def test_unicode_repr_wide(self, size):
        char = "\U0001DCBA"
        s = char * size
        try:
            for f in (repr, ascii):
                r = f(s)
                self.assertEqual(len(r), 2 + (len(f(char)) - 2) * size)
                self.assertTrue(r.endswith(r"\U0001dcba'"), r[-12:])
                r = None
        finally:
            r = s = None

    # The original test_translate is overridden here, so as to get the
    # correct size estimate: str.translate() uses an intermediate Py_UCS4
    # representation.

    @bigmemtest(size=_2G, memuse=ascii_char_size * 2 + ucs4_char_size)
    def test_translate(self, size):
        _ = self.from_latin1
        SUBSTR = _('aZz.z.Aaz.')
        trans = {
            ord(_('.')): _('-'),
            ord(_('a')): _('!'),
            ord(_('Z')): _('$'),
        }
        sublen = len(SUBSTR)
        repeats = size // sublen + 2
        s = SUBSTR * repeats
        s = s.translate(trans)
        self.assertEqual(len(s), repeats * sublen)
        self.assertEqual(s[:sublen], SUBSTR.translate(trans))
        self.assertEqual(s[-sublen:], SUBSTR.translate(trans))
        self.assertEqual(s.count(_('.')), 0)
        self.assertEqual(s.count(_('!')), repeats * 2)
        self.assertEqual(s.count(_('z')), repeats * 3)


class BytesTest(unittest.TestCase, BaseStrTest):

    def from_latin1(self, s):
        return s.encode("latin-1")

    @bigmemtest(size=_2G + 2, memuse=1 + ascii_char_size)
    def test_decode(self, size):
        s = self.from_latin1('.') * size
        self.assertEqual(len(s.decode('utf-8')), size)

    @bigmemtest(size=_2G, memuse=2)
    def test_capitalize(self, size):
        self._test_capitalize(size)

    @bigmemtest(size=_2G, memuse=2)
    def test_title(self, size):
        self._test_title(size)

    @bigmemtest(size=_2G, memuse=2)
    def test_swapcase(self, size):
        self._test_swapcase(size)


class BytearrayTest(unittest.TestCase, BaseStrTest):

    def from_latin1(self, s):
        return bytearray(s.encode("latin-1"))

    @bigmemtest(size=_2G + 2, memuse=1 + ascii_char_size)
    def test_decode(self, size):
        s = self.from_latin1('.') * size
        self.assertEqual(len(s.decode('utf-8')), size)

    @bigmemtest(size=_2G, memuse=2)
    def test_capitalize(self, size):
        self._test_capitalize(size)

    @bigmemtest(size=_2G, memuse=2)
    def test_title(self, size):
        self._test_title(size)

    @bigmemtest(size=_2G, memuse=2)
    def test_swapcase(self, size):
        self._test_swapcase(size)

    test_hash = None
    test_split_large = None

class TupleTest(unittest.TestCase):

    # Tuples have a small, fixed-sized head and an array of pointers to
    # data.  Since we're testing 64-bit addressing, we can assume that the
    # pointers are 8 bytes, and that thus that the tuples take up 8 bytes
    # per size.

    # As a side-effect of testing long tuples, these tests happen to test
    # having more than 2<<31 references to any given object. Hence the
    # use of different types of objects as contents in different tests.

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 2)
    def test_compare(self, size):
        t1 = ('',) * size
        t2 = ('',) * size
        self.assertTrue(t1 == t2)
        del t2
        t2 = ('',) * (size + 1)
        self.assertFalse(t1 == t2)
        del t2
        t2 = (1,) * size
        self.assertFalse(t1 == t2)

    # Test concatenating into a single tuple of more than 2G in length,
    # and concatenating a tuple of more than 2G in length separately, so
    # the smaller test still gets run even if there isn't memory for the
    # larger test (but we still let the tester know the larger test is
    # skipped, in verbose mode.)
    def basic_concat_test(self, size):
        t = ((),) * size
        self.assertEqual(len(t), size)
        t = t + t
        self.assertEqual(len(t), size * 2)

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 3)
    def test_concat_small(self, size):
        return self.basic_concat_test(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 3)
    def test_concat_large(self, size):
        return self.basic_concat_test(size)

    @bigmemtest(size=_2G // 5 + 10, memuse=pointer_size * 5)
    def test_contains(self, size):
        t = (1, 2, 3, 4, 5) * size
        self.assertEqual(len(t), size * 5)
        self.assertTrue(5 in t)
        self.assertFalse((1, 2, 3, 4, 5) in t)
        self.assertFalse(0 in t)

    @bigmemtest(size=_2G + 10, memuse=pointer_size)
    def test_hash(self, size):
        t1 = (0,) * size
        h1 = hash(t1)
        del t1
        t2 = (0,) * (size + 1)
        self.assertFalse(h1 == hash(t2))

    @bigmemtest(size=_2G + 10, memuse=pointer_size)
    def test_index_and_slice(self, size):
        t = (None,) * size
        self.assertEqual(len(t), size)
        self.assertEqual(t[-1], None)
        self.assertEqual(t[5], None)
        self.assertEqual(t[size - 1], None)
        self.assertRaises(IndexError, operator.getitem, t, size)
        self.assertEqual(t[:5], (None,) * 5)
        self.assertEqual(t[-5:], (None,) * 5)
        self.assertEqual(t[20:25], (None,) * 5)
        self.assertEqual(t[-25:-20], (None,) * 5)
        self.assertEqual(t[size - 5:], (None,) * 5)
        self.assertEqual(t[size - 5:size], (None,) * 5)
        self.assertEqual(t[size - 6:size - 2], (None,) * 4)
        self.assertEqual(t[size:size], ())
        self.assertEqual(t[size:size+5], ())

    # Like test_concat, split in two.
    def basic_test_repeat(self, size):
        t = ('',) * size
        self.assertEqual(len(t), size)
        t = t * 2
        self.assertEqual(len(t), size * 2)

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 3)
    def test_repeat_small(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 3)
    def test_repeat_large(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(size=_1G - 1, memuse=12)
    def test_repeat_large_2(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(size=_1G - 1, memuse=pointer_size * 2)
    def test_from_2G_generator(self, size):
        try:
            t = tuple(iter([42]*size))
        except MemoryError:
            pass # acceptable on 32-bit
        else:
            self.assertEqual(len(t), size)
            self.assertEqual(t[:10], (42,) * 10)
            self.assertEqual(t[-10:], (42,) * 10)

    @bigmemtest(size=_1G - 25, memuse=pointer_size * 2)
    def test_from_almost_2G_generator(self, size):
        try:
            t = tuple(iter([42]*size))
        except MemoryError:
            pass # acceptable on 32-bit
        else:
            self.assertEqual(len(t), size)
            self.assertEqual(t[:10], (42,) * 10)
            self.assertEqual(t[-10:], (42,) * 10)

    # Like test_concat, split in two.
    def basic_test_repr(self, size):
        t = (False,) * size
        s = repr(t)
        # The repr of a tuple of Falses is exactly 7 times the tuple length.
        self.assertEqual(len(s), size * 7)
        self.assertEqual(s[:10], '(False, Fa')
        self.assertEqual(s[-10:], 'se, False)')

    @bigmemtest(size=_2G // 7 + 2, memuse=pointer_size + ascii_char_size * 7)
    def test_repr_small(self, size):
        return self.basic_test_repr(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size + ascii_char_size * 7)
    def test_repr_large(self, size):
        return self.basic_test_repr(size)

class ListTest(unittest.TestCase):

    # Like tuples, lists have a small, fixed-sized head and an array of
    # pointers to data, so 8 bytes per size. Also like tuples, we make the
    # lists hold references to various objects to test their refcount
    # limits.

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 2)
    def test_compare(self, size):
        l1 = [''] * size
        l2 = [''] * size
        self.assertTrue(l1 == l2)
        del l2
        l2 = [''] * (size + 1)
        self.assertFalse(l1 == l2)
        del l2
        l2 = [2] * size
        self.assertFalse(l1 == l2)

    # Test concatenating into a single list of more than 2G in length,
    # and concatenating a list of more than 2G in length separately, so
    # the smaller test still gets run even if there isn't memory for the
    # larger test (but we still let the tester know the larger test is
    # skipped, in verbose mode.)
    def basic_test_concat(self, size):
        l = [[]] * size
        self.assertEqual(len(l), size)
        l = l + l
        self.assertEqual(len(l), size * 2)

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 3)
    def test_concat_small(self, size):
        return self.basic_test_concat(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 3)
    def test_concat_large(self, size):
        return self.basic_test_concat(size)

    # XXX This tests suffers from overallocation, just like test_append.
    # This should be fixed in future.
    def basic_test_inplace_concat(self, size):
        l = [sys.stdout] * size
        l += l
        self.assertEqual(len(l), size * 2)
        self.assertTrue(l[0] is l[-1])
        self.assertTrue(l[size - 1] is l[size + 1])

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 2 * 9/8)
    def test_inplace_concat_small(self, size):
        return self.basic_test_inplace_concat(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 2 * 9/8)
    def test_inplace_concat_large(self, size):
        return self.basic_test_inplace_concat(size)

    @bigmemtest(size=_2G // 5 + 10, memuse=pointer_size * 5)
    def test_contains(self, size):
        l = [1, 2, 3, 4, 5] * size
        self.assertEqual(len(l), size * 5)
        self.assertTrue(5 in l)
        self.assertFalse([1, 2, 3, 4, 5] in l)
        self.assertFalse(0 in l)

    @bigmemtest(size=_2G + 10, memuse=pointer_size)
    def test_hash(self, size):
        l = [0] * size
        self.assertRaises(TypeError, hash, l)

    @bigmemtest(size=_2G + 10, memuse=pointer_size)
    def test_index_and_slice(self, size):
        l = [None] * size
        self.assertEqual(len(l), size)
        self.assertEqual(l[-1], None)
        self.assertEqual(l[5], None)
        self.assertEqual(l[size - 1], None)
        self.assertRaises(IndexError, operator.getitem, l, size)
        self.assertEqual(l[:5], [None] * 5)
        self.assertEqual(l[-5:], [None] * 5)
        self.assertEqual(l[20:25], [None] * 5)
        self.assertEqual(l[-25:-20], [None] * 5)
        self.assertEqual(l[size - 5:], [None] * 5)
        self.assertEqual(l[size - 5:size], [None] * 5)
        self.assertEqual(l[size - 6:size - 2], [None] * 4)
        self.assertEqual(l[size:size], [])
        self.assertEqual(l[size:size+5], [])

        l[size - 2] = 5
        self.assertEqual(len(l), size)
        self.assertEqual(l[-3:], [None, 5, None])
        self.assertEqual(l.count(5), 1)
        self.assertRaises(IndexError, operator.setitem, l, size, 6)
        self.assertEqual(len(l), size)

        l[size - 7:] = [1, 2, 3, 4, 5]
        size -= 2
        self.assertEqual(len(l), size)
        self.assertEqual(l[-7:], [None, None, 1, 2, 3, 4, 5])

        l[:7] = [1, 2, 3, 4, 5]
        size -= 2
        self.assertEqual(len(l), size)
        self.assertEqual(l[:7], [1, 2, 3, 4, 5, None, None])

        del l[size - 1]
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[-1], 4)

        del l[-2:]
        size -= 2
        self.assertEqual(len(l), size)
        self.assertEqual(l[-1], 2)

        del l[0]
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[0], 2)

        del l[:2]
        size -= 2
        self.assertEqual(len(l), size)
        self.assertEqual(l[0], 4)

    # Like test_concat, split in two.
    def basic_test_repeat(self, size):
        l = [] * size
        self.assertFalse(l)
        l = [''] * size
        self.assertEqual(len(l), size)
        l = l * 2
        self.assertEqual(len(l), size * 2)

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 3)
    def test_repeat_small(self, size):
        return self.basic_test_repeat(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 3)
    def test_repeat_large(self, size):
        return self.basic_test_repeat(size)

    # XXX This tests suffers from overallocation, just like test_append.
    # This should be fixed in future.
    def basic_test_inplace_repeat(self, size):
        l = ['']
        l *= size
        self.assertEqual(len(l), size)
        self.assertTrue(l[0] is l[-1])
        del l

        l = [''] * size
        l *= 2
        self.assertEqual(len(l), size * 2)
        self.assertTrue(l[size - 1] is l[-1])

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 2 * 9/8)
    def test_inplace_repeat_small(self, size):
        return self.basic_test_inplace_repeat(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 2 * 9/8)
    def test_inplace_repeat_large(self, size):
        return self.basic_test_inplace_repeat(size)

    def basic_test_repr(self, size):
        l = [False] * size
        s = repr(l)
        # The repr of a list of Falses is exactly 7 times the list length.
        self.assertEqual(len(s), size * 7)
        self.assertEqual(s[:10], '[False, Fa')
        self.assertEqual(s[-10:], 'se, False]')
        self.assertEqual(s.count('F'), size)

    @bigmemtest(size=_2G // 7 + 2, memuse=pointer_size + ascii_char_size * 7)
    def test_repr_small(self, size):
        return self.basic_test_repr(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size + ascii_char_size * 7)
    def test_repr_large(self, size):
        return self.basic_test_repr(size)

    # list overallocates ~1/8th of the total size (on first expansion) so
    # the single list.append call puts memuse at 9 bytes per size.
    @bigmemtest(size=_2G, memuse=pointer_size * 9/8)
    def test_append(self, size):
        l = [object()] * size
        l.append(object())
        self.assertEqual(len(l), size+1)
        self.assertTrue(l[-3] is l[-2])
        self.assertFalse(l[-2] is l[-1])

    @bigmemtest(size=_2G // 5 + 2, memuse=pointer_size * 5)
    def test_count(self, size):
        l = [1, 2, 3, 4, 5] * size
        self.assertEqual(l.count(1), size)
        self.assertEqual(l.count("1"), 0)

    # XXX This tests suffers from overallocation, just like test_append.
    # This should be fixed in future.
    def basic_test_extend(self, size):
        l = [object] * size
        l.extend(l)
        self.assertEqual(len(l), size * 2)
        self.assertTrue(l[0] is l[-1])
        self.assertTrue(l[size - 1] is l[size + 1])

    @bigmemtest(size=_2G // 2 + 2, memuse=pointer_size * 2 * 9/8)
    def test_extend_small(self, size):
        return self.basic_test_extend(size)

    @bigmemtest(size=_2G + 2, memuse=pointer_size * 2 * 9/8)
    def test_extend_large(self, size):
        return self.basic_test_extend(size)

    @bigmemtest(size=_2G // 5 + 2, memuse=pointer_size * 5)
    def test_index(self, size):
        l = [1, 2, 3, 4, 5] * size
        size *= 5
        self.assertEqual(l.index(1), 0)
        self.assertEqual(l.index(5, size - 5), size - 1)
        self.assertEqual(l.index(5, size - 5, size), size - 1)
        self.assertRaises(ValueError, l.index, 1, size - 4, size)
        self.assertRaises(ValueError, l.index, 6)

    # This tests suffers from overallocation, just like test_append.
    @bigmemtest(size=_2G + 10, memuse=pointer_size * 9/8)
    def test_insert(self, size):
        l = [1.0] * size
        l.insert(size - 1, "A")
        size += 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[-3:], [1.0, "A", 1.0])

        l.insert(size + 1, "B")
        size += 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[-3:], ["A", 1.0, "B"])

        l.insert(1, "C")
        size += 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[:3], [1.0, "C", 1.0])
        self.assertEqual(l[size - 3:], ["A", 1.0, "B"])

    @bigmemtest(size=_2G // 5 + 4, memuse=pointer_size * 5)
    def test_pop(self, size):
        l = ["a", "b", "c", "d", "e"] * size
        size *= 5
        self.assertEqual(len(l), size)

        item = l.pop()
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(item, "e")
        self.assertEqual(l[-2:], ["c", "d"])

        item = l.pop(0)
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(item, "a")
        self.assertEqual(l[:2], ["b", "c"])

        item = l.pop(size - 2)
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(item, "c")
        self.assertEqual(l[-2:], ["b", "d"])

    @bigmemtest(size=_2G + 10, memuse=pointer_size)
    def test_remove(self, size):
        l = [10] * size
        self.assertEqual(len(l), size)

        l.remove(10)
        size -= 1
        self.assertEqual(len(l), size)

        # Because of the earlier l.remove(), this append doesn't trigger
        # a resize.
        l.append(5)
        size += 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[-2:], [10, 5])
        l.remove(5)
        size -= 1
        self.assertEqual(len(l), size)
        self.assertEqual(l[-2:], [10, 10])

    @bigmemtest(size=_2G // 5 + 2, memuse=pointer_size * 5)
    def test_reverse(self, size):
        l = [1, 2, 3, 4, 5] * size
        l.reverse()
        self.assertEqual(len(l), size * 5)
        self.assertEqual(l[-5:], [5, 4, 3, 2, 1])
        self.assertEqual(l[:5], [5, 4, 3, 2, 1])

    @bigmemtest(size=_2G // 5 + 2, memuse=pointer_size * 5 * 1.5)
    def test_sort(self, size):
        l = [1, 2, 3, 4, 5] * size
        l.sort()
        self.assertEqual(len(l), size * 5)
        self.assertEqual(l.count(1), size)
        self.assertEqual(l[:10], [1] * 10)
        self.assertEqual(l[-10:], [5] * 10)


class DictTest(unittest.TestCase):

    @bigmemtest(size=357913941, memuse=160)
    def test_dict(self, size):
        # https://github.com/python/cpython/issues/102701
        d = dict.fromkeys(range(size))
        d[size] = 1


class ImmortalityTest(unittest.TestCase):

    @bigmemtest(size=_2G, memuse=pointer_size * 9/8)
    def test_stickiness(self, size):
        """Check that immortality is "sticky", so that
           once an object is immortal it remains so."""
        if size < _2G:
            # Not enough memory to cause immortality on overflow
            return
        o1 = o2 = o3 = o4 = o5 = o6 = o7 = o8 = object()
        l = [o1] * (size-20)
        self.assertFalse(_testcapi.is_immortal(o1))
        for _ in range(30):
            l.append(l[0])
        self.assertTrue(_testcapi.is_immortal(o1))
        del o2, o3, o4, o5, o6, o7, o8
        self.assertTrue(_testcapi.is_immortal(o1))
        del l
        self.assertTrue(_testcapi.is_immortal(o1))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        support.set_memlimit(sys.argv[1])
    unittest.main()
