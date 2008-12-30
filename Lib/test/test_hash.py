# test the invariant that
#   iff a==b then hash(a)==hash(b)
#
# Also test that hash implementations are inherited as expected

import unittest
from test import test_support
from collections import Hashable


class HashEqualityTestCase(unittest.TestCase):

    def same_hash(self, *objlist):
        # Hash each object given and fail if
        # the hash values are not all the same.
        hashed = map(hash, objlist)
        for h in hashed[1:]:
            if h != hashed[0]:
                self.fail("hashed values differ: %r" % (objlist,))

    def test_numeric_literals(self):
        self.same_hash(1, 1L, 1.0, 1.0+0.0j)
        self.same_hash(0, 0L, 0.0, 0.0+0.0j)
        self.same_hash(-1, -1L, -1.0, -1.0+0.0j)
        self.same_hash(-2, -2L, -2.0, -2.0+0.0j)

    def test_coerced_integers(self):
        self.same_hash(int(1), long(1), float(1), complex(1),
                       int('1'), float('1.0'))
        self.same_hash(int(-2**31), long(-2**31), float(-2**31))
        self.same_hash(int(1-2**31), long(1-2**31), float(1-2**31))
        self.same_hash(int(2**31-1), long(2**31-1), float(2**31-1))
        # for 64-bit platforms
        self.same_hash(int(2**31), long(2**31), float(2**31))
        self.same_hash(int(-2**63), long(-2**63), float(-2**63))
        self.same_hash(int(1-2**63), long(1-2**63))
        self.same_hash(int(2**63-1), long(2**63-1))

    def test_coerced_floats(self):
        self.same_hash(long(1.23e300), float(1.23e300))
        self.same_hash(float(0.5), complex(0.5, 0.0))


_default_hash = object.__hash__
class DefaultHash(object): pass

_FIXED_HASH_VALUE = 42
class FixedHash(object):
    def __hash__(self):
        return _FIXED_HASH_VALUE

class OnlyEquality(object):
    def __eq__(self, other):
        return self is other
    # Trick to suppress Py3k warning in 2.x
    __hash__ = None
del OnlyEquality.__hash__

class OnlyInequality(object):
    def __ne__(self, other):
        return self is not other

class OnlyCmp(object):
    def __cmp__(self, other):
        return cmp(id(self), id(other))
    # Trick to suppress Py3k warning in 2.x
    __hash__ = None
del OnlyCmp.__hash__

class InheritedHashWithEquality(FixedHash, OnlyEquality): pass
class InheritedHashWithInequality(FixedHash, OnlyInequality): pass
class InheritedHashWithCmp(FixedHash, OnlyCmp): pass

class NoHash(object):
    __hash__ = None

class HashInheritanceTestCase(unittest.TestCase):
    default_expected = [object(),
                        DefaultHash(),
                        OnlyEquality(),
                        OnlyInequality(),
                        OnlyCmp(),
                       ]
    fixed_expected = [FixedHash(),
                      InheritedHashWithEquality(),
                      InheritedHashWithInequality(),
                      InheritedHashWithCmp(),
                      ]
    error_expected = [NoHash()]

    def test_default_hash(self):
        for obj in self.default_expected:
            self.assertEqual(hash(obj), _default_hash(obj))

    def test_fixed_hash(self):
        for obj in self.fixed_expected:
            self.assertEqual(hash(obj), _FIXED_HASH_VALUE)

    def test_error_hash(self):
        for obj in self.error_expected:
            self.assertRaises(TypeError, hash, obj)

    def test_hashable(self):
        objects = (self.default_expected +
                   self.fixed_expected)
        for obj in objects:
            self.assert_(isinstance(obj, Hashable), repr(obj))

    def test_not_hashable(self):
        for obj in self.error_expected:
            self.assertFalse(isinstance(obj, Hashable), repr(obj))


# Issue #4701: Check that some builtin types are correctly hashable
#  (This test only used to fail in Python 3.0, but has been included
#   in 2.x along with the lazy call to PyType_Ready in PyObject_Hash)
class DefaultIterSeq(object):
    seq = range(10)
    def __len__(self):
        return len(self.seq)
    def __getitem__(self, index):
        return self.seq[index]

class HashBuiltinsTestCase(unittest.TestCase):
    hashes_to_check = [xrange(10),
                       enumerate(xrange(10)),
                       iter(DefaultIterSeq()),
                       iter(lambda: 0, 0),
                      ]

    def test_hashes(self):
        _default_hash = object.__hash__
        for obj in self.hashes_to_check:
            self.assertEqual(hash(obj), _default_hash(obj))

def test_main():
    test_support.run_unittest(HashEqualityTestCase,
                              HashInheritanceTestCase,
                              HashBuiltinsTestCase)


if __name__ == "__main__":
    test_main()
