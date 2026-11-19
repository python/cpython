import sys
import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None
SSIZE_MAX = sys.maxsize
SSIZE_MIN = -sys.maxsize - 1

VALUES = [None, 0, 1, 3, 7, -1, -3, -7]
STEPS = [None, 1, 3, 5, -1, -3, -5]
LENGTHS = [0, 1, 3, 10]


class Index:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value


class BadIndex:
    def __index__(self):
        raise RuntimeError('bad index')


class PopIndex:
    # __index__() removes the last item of the list.
    def __init__(self, value, seq):
        self.value = value
        self.seq = seq

    def __index__(self):
        self.seq.pop()
        return self.value


class SliceTest(unittest.TestCase):

    def test_check(self):
        # Test PySlice_Check()
        check = _testlimitedcapi.slice_check
        self.assertTrue(check(slice(1, 7, 2)))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_new(self):
        # Test PySlice_New()
        new = _testlimitedcapi.slice_new
        self.assertEqual(new(1, 7, 2), slice(1, 7, 2))
        self.assertEqual(new(7, 1, -2), slice(7, 1, -2))
        self.assertEqual(new('a', 'b', 'c'), slice('a', 'b', 'c'))
        self.assertEqual(new(NULL, NULL, NULL), slice(None, None, None))

    def test_getindices(self):
        # Test PySlice_GetIndices()
        getindices = _testlimitedcapi.slice_getindices
        self.assertEqual(getindices(slice(1, 7, 2), 10), (1, 7, 2))
        self.assertEqual(getindices(slice(None), 10), (0, 10, 1))
        self.assertEqual(getindices(slice(None, None, -1), 10), (9, -1, -1))
        self.assertEqual(getindices(slice(-3, -1), 10), (7, 9, 1))
        self.assertEqual(getindices(slice(-1, -3, -1), 10), (9, 7, -1))
        self.assertEqual(getindices(slice(None, None, -2), 0), (-1, -1, -2))
        self.assertEqual(getindices(slice(-3, -5, 1), 0), (-3, -5, 1))

        # It fails without setting an exception for out of bounds indices,
        # a zero step and non-integer indices.
        self.assertIsNone(getindices(slice(1, 11), 10))
        self.assertIsNone(getindices(slice(10, 1), 10))
        self.assertIsNone(getindices(slice(1, 7, 0), 10))
        self.assertIsNone(getindices(slice(Index(1)), 10))
        self.assertIsNone(getindices(slice('a'), 10))
        self.assertIsNone(getindices(slice(1, 'a'), 10))
        self.assertIsNone(getindices(slice(1, 7, 'a'), 10))

        # Negative length is not supported, but does not fail.
        self.assertIsNone(getindices(slice(None), -3))
        self.assertIsNone(getindices(slice(1, 7, 2), -3))
        self.assertEqual(getindices(slice(-10, -5, 1), -3), (-13, -8, 1))
        self.assertEqual(getindices(slice(-5, -10, -2), -3), (-8, -13, -2))

        # CRASHES getindices(NULL, 10)
        # CRASHES getindices(object(), 10)

    def test_unpack(self):
        # Test PySlice_Unpack()
        unpack = _testlimitedcapi.slice_unpack
        self.assertEqual(unpack(slice(1, 7)), (1, 7, 1))
        self.assertEqual(unpack(slice(1, 7, 2)), (1, 7, 2))
        self.assertEqual(unpack(slice(7, 1, -2)), (7, 1, -2))
        self.assertEqual(unpack(slice(None, 7, 2)), (0, 7, 2))
        self.assertEqual(unpack(slice(None, 7, -2)), (SSIZE_MAX, 7, -2))
        self.assertEqual(unpack(slice(1, None, 2)), (1, SSIZE_MAX, 2))
        self.assertEqual(unpack(slice(1, None, -2)), (1, SSIZE_MIN, -2))
        self.assertEqual(unpack(slice(None)), (0, SSIZE_MAX, 1))
        self.assertEqual(unpack(slice(None, None, -1)),
                         (SSIZE_MAX, SSIZE_MIN, -1))
        # Negative indices are not adjusted.
        self.assertEqual(unpack(slice(-3, -1)), (-3, -1, 1))
        self.assertEqual(unpack(slice(Index(1), Index(7), Index(2))),
                         (1, 7, 2))

        # Values which do not fit in Py_ssize_t are silently clipped.
        self.assertEqual(unpack(slice(1, 2**1000)), (1, SSIZE_MAX, 1))
        self.assertEqual(unpack(slice(1, -2**1000)), (1, SSIZE_MIN, 1))
        self.assertEqual(unpack(slice(2**1000, 7)), (SSIZE_MAX, 7, 1))
        self.assertEqual(unpack(slice(-2**1000, 7)), (SSIZE_MIN, 7, 1))
        self.assertEqual(unpack(slice(1, 7, 2**1000)), (1, 7, SSIZE_MAX))
        # The step is boosted to -PY_SSIZE_T_MAX, not PY_SSIZE_T_MIN, so
        # that negating it is safe.
        self.assertEqual(unpack(slice(7, 1, -2**1000)), (7, 1, -SSIZE_MAX))
        self.assertEqual(unpack(slice(7, 1, SSIZE_MIN)), (7, 1, -SSIZE_MAX))

        with self.assertRaisesRegex(ValueError, 'slice step cannot be zero'):
            unpack(slice(1, 1, 0))
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            unpack(slice('a', 7))
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            unpack(slice(1, 'a'))
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            unpack(slice(1, 7, 'a'))
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            unpack(slice(BadIndex(), 7))
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            unpack(slice(1, BadIndex()))
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            unpack(slice(1, 7, BadIndex()))

        # CRASHES unpack(NULL)
        # CRASHES unpack(object())

    def test_adjustindices(self):
        # Test PySlice_AdjustIndices()
        adjust = _testlimitedcapi.slice_adjustindices
        self.assertEqual(adjust(10, 1, 7, 1), (6, 1, 7))
        self.assertEqual(adjust(10, 1, 7, 2), (3, 1, 7))
        self.assertEqual(adjust(10, 7, 1, -1), (6, 7, 1))
        self.assertEqual(adjust(10, 7, 1, -2), (3, 7, 1))
        # An empty slice keeps the adjusted indices.
        self.assertEqual(adjust(10, 7, 1, 1), (0, 7, 1))
        self.assertEqual(adjust(10, 1, 7, -1), (0, 1, 7))

        # Negative indices are added to the length.
        self.assertEqual(adjust(10, -9, -3, 1), (6, 1, 7))
        self.assertEqual(adjust(10, -3, -9, -1), (6, 7, 1))

        # Out of bounds indices are clipped.
        self.assertEqual(adjust(10, -100, 100, 1), (10, 0, 10))
        self.assertEqual(adjust(10, 100, -100, -1), (10, 9, -1))
        self.assertEqual(adjust(10, SSIZE_MIN, SSIZE_MAX, 1), (10, 0, 10))
        self.assertEqual(adjust(10, SSIZE_MAX, SSIZE_MIN, -1), (10, 9, -1))
        self.assertEqual(adjust(0, 1, 7, 1), (0, 0, 0))
        self.assertEqual(adjust(0, 7, 1, -1), (0, -1, -1))

        # The returned length is the length of the corresponding range.
        for length in LENGTHS:
            for start in VALUES[1:]:
                for stop in VALUES[1:]:
                    for step in STEPS[1:]:
                        with self.subTest(length=length, start=start,
                                          stop=stop, step=step):
                            slicelength, start2, stop2 = adjust(length, start,
                                                                stop, step)
                            self.assertEqual(slicelength,
                                             len(range(start2, stop2, step)))

        # Negative length is not supported, but does not fail.
        self.assertEqual(adjust(-3, 1, 7, 1), (0, -3, -3))
        self.assertEqual(adjust(-3, 7, 1, -1), (0, -4, -4))
        self.assertEqual(adjust(-3, -10, -5, 1), (0, 0, 0))

        # The step is asserted to be neither zero nor less than
        # -PY_SSIZE_T_MAX.
        # CRASHES adjust(10, 0, 10, 0)
        # CRASHES adjust(10, 0, 10, SSIZE_MIN)


class GetIndicesExMacroTest(unittest.TestCase):
    # PySlice_GetIndicesEx() is a macro using PySlice_Unpack() and
    # PySlice_AdjustIndices().  It is also a deprecated function, exported
    # for the stable ABI.
    getindicesex = staticmethod(_testlimitedcapi.slice_getindicesex_macro)
    getindicesex_seq = staticmethod(
        _testlimitedcapi.slice_getindicesex_seq_macro)
    # The macro evaluates the length after calling PySlice_Unpack(), so the
    # size of the list after removing an item is used.
    resized = (6, 8, 1, 2)

    def test_getindicesex(self):
        # Test PySlice_GetIndicesEx()
        getindicesex = self.getindicesex
        self.assertEqual(getindicesex(slice(1, 7, 2), 10), (1, 7, 2, 3))
        self.assertEqual(getindicesex(slice(7, 1, -2), 10), (7, 1, -2, 3))
        self.assertEqual(getindicesex(slice(Index(1), Index(7), Index(2)), 10),
                         (1, 7, 2, 3))

        # The result agrees with slice.indices() and the slice length is
        # the length of the corresponding range.
        for length in LENGTHS:
            for start in VALUES:
                for stop in VALUES:
                    for step in STEPS:
                        s = slice(start, stop, step)
                        with self.subTest(slice=s, length=length):
                            indices = s.indices(length)
                            self.assertEqual(getindicesex(s, length),
                                             indices + (len(range(*indices)),))

        # Negative indices are added to the length.
        self.assertEqual(getindicesex(slice(-9, -3), 10), (1, 7, 1, 6))
        self.assertEqual(getindicesex(slice(-3, -9, -1), 10), (7, 1, -1, 6))

        # Out of bounds indices are clipped.
        self.assertEqual(getindicesex(slice(-100, 100), 10), (0, 10, 1, 10))
        self.assertEqual(getindicesex(slice(100, -100, -1), 10),
                         (9, -1, -1, 10))
        self.assertEqual(getindicesex(slice(None), 0), (0, 0, 1, 0))
        self.assertEqual(getindicesex(slice(1, 7, 2), 0), (0, 0, 2, 0))
        self.assertEqual(getindicesex(slice(None, None, -1), 0),
                         (-1, -1, -1, 0))

        # Indices which do not fit in Py_ssize_t are clipped, not rejected.
        # Note that slice.indices() does not clip the step.
        self.assertEqual(getindicesex(slice(1, 2**1000), 10), (1, 10, 1, 9))
        self.assertEqual(getindicesex(slice(2**1000, 7), 10), (10, 7, 1, 0))
        self.assertEqual(getindicesex(slice(1, 7, 2**1000), 10),
                         (1, 7, SSIZE_MAX, 1))
        # -PY_SSIZE_T_MAX-1 is replaced with -PY_SSIZE_T_MAX.
        self.assertEqual(getindicesex(slice(7, 1, -2**1000), 10),
                         (7, 1, -SSIZE_MAX, 1))
        self.assertEqual(getindicesex(slice(7, 1, SSIZE_MIN), 10),
                         (7, 1, -SSIZE_MAX, 1))

        with self.assertRaisesRegex(ValueError, 'slice step cannot be zero'):
            getindicesex(slice(1, 7, 0), 10)
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            getindicesex(slice('a', 7), 10)
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            getindicesex(slice(1, 'a'), 10)
        with self.assertRaisesRegex(TypeError,
                                    'slice indices must be integers'):
            getindicesex(slice(1, 7, 'a'), 10)
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            getindicesex(slice(BadIndex(), 7), 10)
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            getindicesex(slice(1, BadIndex()), 10)
        with self.assertRaisesRegex(RuntimeError, 'bad index'):
            getindicesex(slice(1, 7, BadIndex()), 10)

        # Negative length is not supported, but does not fail.
        self.assertEqual(getindicesex(slice(None), -3), (-3, -3, 1, 0))
        self.assertEqual(getindicesex(slice(1, 7, 2), -3), (-3, -3, 2, 0))
        self.assertEqual(getindicesex(slice(7, 1, -2), -3), (-4, -4, -2, 0))

        # CRASHES getindicesex(NULL, 10)
        # CRASHES getindicesex(object(), 10)

    def test_getindicesex_seq(self):
        # The length is the size of a sequence.
        getindicesex_seq = self.getindicesex_seq
        seq = list(range(10))
        self.assertEqual(getindicesex_seq(slice(-3, -1), seq), (7, 9, 1, 2))
        self.assertEqual(getindicesex_seq(slice(-3, -1), []), (0, 0, 1, 0))

        # gh-72054: __index__() can resize the sequence.  Negative indices
        # are adjusted by the length, so the result depends on when it is
        # evaluated.
        seq = list(range(10))
        self.assertEqual(getindicesex_seq(slice(PopIndex(-3, seq), -1), seq),
                         self.resized)
        self.assertEqual(len(seq), 9, seq)

        seq = list(range(10))
        self.assertEqual(getindicesex_seq(slice(-3, PopIndex(-1, seq)), seq),
                         self.resized)
        self.assertEqual(len(seq), 9, seq)

        seq = list(range(10))
        self.assertEqual(
            getindicesex_seq(slice(-3, -1, PopIndex(1, seq)), seq),
            self.resized)
        self.assertEqual(len(seq), 9, seq)

        # CRASHES getindicesex_seq(slice(None), NULL)
        # CRASHES getindicesex_seq(slice(None), object())


class GetIndicesExFuncTest(GetIndicesExMacroTest):
    # The deprecated function is equivalent to the macro, except that the
    # length is evaluated before the call.
    getindicesex = staticmethod(_testlimitedcapi.slice_getindicesex_func)
    getindicesex_seq = staticmethod(
        _testlimitedcapi.slice_getindicesex_seq_func)
    # The size of the list before removing an item is used.
    resized = (7, 9, 1, 2)


if __name__ == "__main__":
    unittest.main()
