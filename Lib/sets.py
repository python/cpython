"""Classes to represent arbitrary sets (including sets of sets).

This module implements sets using dictionaries whose values are
ignored.  The usual operations (union, intersection, deletion, etc.)
are provided as both methods and operators.

Important: sets are not sequences!  While they support 'x in s',
'len(s)', and 'for x in s', none of those operations are unique for
sequences; for example, mappings support all three as well.  The
characteristic operation for sequences is subscripting with small
integers: s[i], for i in range(len(s)).  Sets don't support
subscripting at all.  Also, sequences allow multiple occurrences and
their elements have a definite order; sets on the other hand don't
record multiple occurrences and don't remember the order of element
insertion (which is why they don't support s[i]).

The following classes are provided:

BaseSet -- All the operations common to both mutable and immutable
    sets. This is an abstract class, not meant to be directly
    instantiated.

Set -- Mutable sets, subclass of BaseSet; not hashable.

ImmutableSet -- Immutable sets, subclass of BaseSet; hashable.
    An iterable argument is mandatory to create an ImmutableSet.

_TemporarilyImmutableSet -- Not a subclass of BaseSet: just a wrapper
    around a Set, hashable, giving the same hash value as the
    immutable set equivalent would have.  Do not use this class
    directly.

Only hashable objects can be added to a Set. In particular, you cannot
really add a Set as an element to another Set; if you try, what is
actually added is an ImmutableSet built from it (it compares equal to
the one you tried adding).

When you ask if `x in y' where x is a Set and y is a Set or
ImmutableSet, x is wrapped into a _TemporarilyImmutableSet z, and
what's tested is actually `z in y'.

"""

# Code history:
#
# - Greg V. Wilson wrote the first version, using a different approach
#   to the mutable/immutable problem, and inheriting from dict.
#
# - Alex Martelli modified Greg's version to implement the current
#   Set/ImmutableSet approach, and make the data an attribute.
#
# - Guido van Rossum rewrote much of the code, made some API changes,
#   and cleaned up the docstrings.


__all__ = ['BaseSet', 'Set', 'ImmutableSet']


class BaseSet(object):
    """Common base class for mutable and immutable sets."""

    __slots__ = ['_data']

    # Constructor

    def __init__(self):
        """This is an abstract class."""
        # Don't call this from a concrete subclass!
        if self.__class__ is BaseSet:
            raise NotImplementedError, ("BaseSet is an abstract class.  "
                                        "Use Set or ImmutableSet.")

    # Standard protocols: __len__, __repr__, __str__, __iter__

    def __len__(self):
        """Return the number of elements of a set."""
        return len(self._data)

    def __repr__(self):
        """Return string representation of a set.

        This looks like 'Set([<list of elements>])'.
        """
        return self._repr()

    # __str__ is the same as __repr__
    __str__ = __repr__

    def _repr(self, sorted=False):
        elements = self._data.keys()
        if sorted:
            elements.sort()
        return '%s(%r)' % (self.__class__.__name__, elements)

    def __iter__(self):
        """Return an iterator over the elements or a set.

        This is the keys iterator for the underlying dict.
        """
        return self._data.iterkeys()

    # Comparisons.  Ordering is determined by the ordering of the
    # underlying dicts (which is consistent though unpredictable).

    def __lt__(self, other):
        self._binary_sanity_check(other)
        return self._data < other._data

    def __le__(self, other):
        self._binary_sanity_check(other)
        return self._data <= other._data

    def __eq__(self, other):
        self._binary_sanity_check(other)
        return self._data == other._data

    def __ne__(self, other):
        self._binary_sanity_check(other)
        return self._data != other._data

    def __gt__(self, other):
        self._binary_sanity_check(other)
        return self._data > other._data

    def __ge__(self, other):
        self._binary_sanity_check(other)
        return self._data >= other._data

    # Copying operations

    def copy(self):
        """Return a shallow copy of a set."""
        return self.__class__(self)

    __copy__ = copy # For the copy module

    def __deepcopy__(self, memo):
        """Return a deep copy of a set; used by copy module."""
        # This pre-creates the result and inserts it in the memo
        # early, in case the deep copy recurses into another reference
        # to this same set.  A set can't be an element of itself, but
        # it can certainly contain an object that has a reference to
        # itself.
        from copy import deepcopy
        result = self.__class__([])
        memo[id(self)] = result
        data = result._data
        value = True
        for elt in self:
            data[deepcopy(elt, memo)] = value
        return result

    # Standard set operations: union, intersection, both differences

    def union(self, other):
        """Return the union of two sets as a new set.

        (I.e. all elements that are in either set.)
        """
        self._binary_sanity_check(other)
        result = self.__class__(self._data)
        result._data.update(other._data)
        return result

    __or__ = union

    def intersection(self, other):
        """Return the intersection of two sets as a new set.

        (I.e. all elements that are in both sets.)
        """
        self._binary_sanity_check(other)
        if len(self) <= len(other):
            little, big = self, other
        else:
            little, big = other, self
        result = self.__class__([])
        data = result._data
        value = True
        for elt in little:
            if elt in big:
                data[elt] = value
        return result

    __and__ = intersection

    def symmetric_difference(self, other):
        """Return the symmetric difference of two sets as a new set.

        (I.e. all elements that are in exactly one of the sets.)
        """
        self._binary_sanity_check(other)
        result = self.__class__([])
        data = result._data
        value = True
        for elt in self:
            if elt not in other:
                data[elt] = value
        for elt in other:
            if elt not in self:
                data[elt] = value
        return result

    __xor__ = symmetric_difference

    def difference(self, other):
        """Return the difference of two sets as a new Set.

        (I.e. all elements that are in this set and not in the other.)
        """
        self._binary_sanity_check(other)
        result = self.__class__([])
        data = result._data
        value = True
        for elt in self:
            if elt not in other:
                data[elt] = value
        return result

    __sub__ = difference

    # Membership test

    def __contains__(self, element):
        """Report whether an element is a member of a set.

        (Called in response to the expression `element in self'.)
        """
        try:
            return element in self._data
        except TypeError:
            transform = getattr(element, "_as_temporary_immutable", None)
            if transform is None:
                raise # re-raise the TypeError exception we caught
            return transform() in self._data

    # Subset and superset test

    def issubset(self, other):
        """Report whether another set contains this set."""
        self._binary_sanity_check(other)
        if len(self) > len(other):  # Fast check for obvious cases
            return False
        for elt in self:
            if elt not in other:
                return False
        return True

    def issuperset(self, other):
        """Report whether this set contains another set."""
        self._binary_sanity_check(other)
        if len(self) < len(other):  # Fast check for obvious cases
            return False
        for elt in other:
            if elt not in self:
                return False
        return True

    # Assorted helpers

    def _binary_sanity_check(self, other):
        # Check that the other argument to a binary operation is also
        # a set, raising a TypeError otherwise.
        if not isinstance(other, BaseSet):
            raise TypeError, "Binary operation only permitted between sets"

    def _compute_hash(self):
        # Calculate hash code for a set by xor'ing the hash codes of
        # the elements.  This algorithm ensures that the hash code
        # does not depend on the order in which elements are added to
        # the code.  This is not called __hash__ because a BaseSet
        # should not be hashable; only an ImmutableSet is hashable.
        result = 0
        for elt in self:
            result ^= hash(elt)
        return result


class ImmutableSet(BaseSet):
    """Immutable set class."""

    __slots__ = ['_hashcode']

    # BaseSet + hashing

    def __init__(self, seq):
        """Construct an immutable set from a sequence."""
        self._hashcode = None
        self._data = data = {}
        # I don't know a faster way to do this in pure Python.
        # Custom code written in C only did it 65% faster,
        # preallocating the dict to len(seq); without
        # preallocation it was only 25% faster.  So the speed of
        # this Python code is respectable.  Just copying True into
        # a local variable is responsible for a 7-8% speedup.
        value = True
        # XXX Should this perhaps look for _as_immutable?
        # XXX If so, should use self.update(seq).
        for key in seq:
            data[key] = value

    def __hash__(self):
        if self._hashcode is None:
            self._hashcode = self._compute_hash()
        return self._hashcode


class Set(BaseSet):
    """ Mutable set class."""

    __slots__ = []

    # BaseSet + operations requiring mutability; no hashing

    def __init__(self, seq=None):
        """Construct an immutable set from a sequence."""
        self._data = data = {}
        if seq is not None:
            value = True
            # XXX Should this perhaps look for _as_immutable?
            # XXX If so, should use self.update(seq).
            for key in seq:
                data[key] = value

    # In-place union, intersection, differences

    def union_update(self, other):
        """Update a set with the union of itself and another."""
        self._binary_sanity_check(other)
        self._data.update(other._data)
        return self

    __ior__ = union_update

    def intersection_update(self, other):
        """Update a set with the intersection of itself and another."""
        self._binary_sanity_check(other)
        for elt in self._data.keys():
            if elt not in other:
                del self._data[elt]
        return self

    __iand__ = intersection_update

    def symmetric_difference_update(self, other):
        """Update a set with the symmetric difference of itself and another."""
        self._binary_sanity_check(other)
        data = self._data
        value = True
        for elt in other:
            if elt in data:
                del data[elt]
            else:
                data[elt] = value
        return self

    __ixor__ = symmetric_difference_update

    def difference_update(self, other):
        """Remove all elements of another set from this set."""
        self._binary_sanity_check(other)
        data = self._data
        for elt in other:
            if elt in data:
                del data[elt]
        return self

    __isub__ = difference_update

    # Python dict-like mass mutations: update, clear

    def update(self, iterable):
        """Add all values from an iterable (such as a list or file)."""
        data = self._data
        value = True
        for element in iterable:
            try:
                data[element] = value
            except TypeError:
                transform = getattr(element, "_as_temporary_immutable", None)
                if transform is None:
                    raise # re-raise the TypeError exception we caught
                data[transform()] = value

    def clear(self):
        """Remove all elements from this set."""
        self._data.clear()

    # Single-element mutations: add, remove, discard

    def add(self, element):
        """Add an element to a set.

        This has no effect if the element is already present.
        """
        try:
            self._data[element] = True
        except TypeError:
            transform = getattr(element, "_as_temporary_immutable", None)
            if transform is None:
                raise # re-raise the TypeError exception we caught
            self._data[transform()] = True

    def remove(self, element):
        """Remove an element from a set; it must be a member.

        If the element is not a member, raise a KeyError.
        """
        try:
            del self._data[element]
        except TypeError:
            transform = getattr(element, "_as_temporary_immutable", None)
            if transform is None:
                raise # re-raise the TypeError exception we caught
            del self._data[transform()]

    def discard(self, element):
        """Remove an element from a set if it is a member.

        If the element is not a member, do nothing.
        """
        try:
            del self._data[element]
        except KeyError:
            pass

    def pop(self):
        """Remove and return a randomly-chosen set element."""
        return self._data.popitem()[0]

    def _as_immutable(self):
        # Return a copy of self as an immutable set
        return ImmutableSet(self)

    def _as_temporarily_immutable(self):
        # Return self wrapped in a temporarily immutable set
        return _TemporarilyImmutableSet(self)


class _TemporarilyImmutableSet(object):
    # Wrap a mutable set as if it was temporarily immutable.
    # This only supplies hashing and equality comparisons.

    _hashcode = None

    def __init__(self, set):
        self._set = set

    def __hash__(self):
        if self._hashcode is None:
            self._hashcode = self._set._compute_hash()
        return self._hashcode

    def __eq__(self, other):
        return self._set == other

    def __ne__(self, other):
        return self._set != other


# Rudimentary self-tests

def _test():

    # Empty set
    red = Set()
    assert `red` == "Set([])", "Empty set: %s" % `red`

    # Unit set
    green = Set((0,))
    assert `green` == "Set([0])", "Unit set: %s" % `green`

    # 3-element set
    blue = Set([0, 1, 2])
    assert blue._repr(True) == "Set([0, 1, 2])", "3-element set: %s" % `blue`

    # 2-element set with other values
    black = Set([0, 5])
    assert black._repr(True) == "Set([0, 5])", "2-element set: %s" % `black`

    # All elements from all sets
    white = Set([0, 1, 2, 5])
    assert white._repr(True) == "Set([0, 1, 2, 5])", "4-element set: %s" % `white`

    # Add element to empty set
    red.add(9)
    assert `red` == "Set([9])", "Add to empty set: %s" % `red`

    # Remove element from unit set
    red.remove(9)
    assert `red` == "Set([])", "Remove from unit set: %s" % `red`

    # Remove element from empty set
    try:
        red.remove(0)
        assert 0, "Remove element from empty set: %s" % `red`
    except LookupError:
        pass

    # Length
    assert len(red) == 0,   "Length of empty set"
    assert len(green) == 1, "Length of unit set"
    assert len(blue) == 3,  "Length of 3-element set"

    # Compare
    assert green == Set([0]), "Equality failed"
    assert green != Set([1]), "Inequality failed"

    # Union
    assert blue  | red   == blue,  "Union non-empty with empty"
    assert red   | blue  == blue,  "Union empty with non-empty"
    assert green | blue  == blue,  "Union non-empty with non-empty"
    assert blue  | black == white, "Enclosing union"

    # Intersection
    assert blue  & red   == red,   "Intersect non-empty with empty"
    assert red   & blue  == red,   "Intersect empty with non-empty"
    assert green & blue  == green, "Intersect non-empty with non-empty"
    assert blue  & black == green, "Enclosing intersection"

    # Symmetric difference
    assert red ^ green == green,        "Empty symdiff non-empty"
    assert green ^ blue == Set([1, 2]), "Non-empty symdiff"
    assert white ^ white == red,        "Self symdiff"

    # Difference
    assert red - green == red,           "Empty - non-empty"
    assert blue - red == blue,           "Non-empty - empty"
    assert white - black == Set([1, 2]), "Non-empty - non-empty"

    # In-place union
    orange = Set([])
    orange |= Set([1])
    assert orange == Set([1]), "In-place union"

    # In-place intersection
    orange = Set([1, 2])
    orange &= Set([2])
    assert orange == Set([2]), "In-place intersection"

    # In-place difference
    orange = Set([1, 2, 3])
    orange -= Set([2, 4])
    assert orange == Set([1, 3]), "In-place difference"

    # In-place symmetric difference
    orange = Set([1, 2, 3])
    orange ^= Set([3, 4])
    assert orange == Set([1, 2, 4]), "In-place symmetric difference"

    print "All tests passed"


if __name__ == "__main__":
    _test()
