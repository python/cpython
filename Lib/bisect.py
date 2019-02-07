"""Bisection algorithms."""

def insort_right(a, x, lo=0, hi=None, key=None):
    """Insert item x in list a, and keep it sorted assuming a is sorted.

    If x is already in a, insert it to the right of the rightmost x.

    Optional args lo (default 0) and hi (default len(a)) bound the
    slice of a to be searched.

    Optional argument key is a function of one argument used to
    customize the order.
    """

    # the python implementation does not
    # need to optimize for performance
    if key is None:
        key = lambda e: e

    # we avoid computing key(x) in each iteration
    x_value = key(x)

    if lo < 0:
        raise ValueError('lo must be non-negative')
    if hi is None:
        hi = len(a)
    while lo < hi:
        mid = (lo+hi)//2
        if x_value < key(a[mid]):
            hi = mid
        else:
            lo = mid+1
    a.insert(lo, x)

def bisect_right(a, x, lo=0, hi=None, key=None):
    """Return the index where to insert item x in list a, assuming a is sorted.

    The return value i is such that all e in a[:i] have e <= x, and all e in
    a[i:] have e > x.  So if x already appears in the list, a.insert(x) will
    insert just after the rightmost x already there.

    Optional args lo (default 0) and hi (default len(a)) bound the
    slice of a to be searched.

    Optional argument key is a function of one argument used to
    customize the order.
    """

    if key is None:
        key = lambda e: e

    x_value = key(x)

    if lo < 0:
        raise ValueError('lo must be non-negative')
    if hi is None:
        hi = len(a)
    while lo < hi:
        mid = (lo+hi)//2
        if x_value < key(a[mid]):
            hi = mid
        else:
            lo = mid+1
    return lo

def insort_left(a, x, lo=0, hi=None, key=None):
    """Insert item x in list a, and keep it sorted assuming a is sorted.

    If x is already in a, insert it to the left of the leftmost x.

    Optional args lo (default 0) and hi (default len(a)) bound the
    slice of a to be searched.

    Optional argument key is a function of one argument used to
    customize the order.
    """

    if key is None:
        key = lambda e: e

    x_value = key(x)

    if lo < 0:
        raise ValueError('lo must be non-negative')
    if hi is None:
        hi = len(a)
    while lo < hi:
        mid = (lo+hi)//2
        if key(a[mid]) < x_value:
            lo = mid+1
        else:
            hi = mid
    a.insert(lo, x)


def bisect_left(a, x, lo=0, hi=None, key=None):
    """Return the index where to insert item x in list a, assuming a is sorted.

    The return value i is such that all e in a[:i] have e < x, and all e in
    a[i:] have e >= x.  So if x already appears in the list, a.insert(x) will
    insert just before the leftmost x already there.

    Optional args lo (default 0) and hi (default len(a)) bound the
    slice of a to be searched.

    Optional argument key is a function of one argument used to
    customize the order.
    """

    if key is None:
        key = lambda e: e

    x_value = key(x)

    if lo < 0:
        raise ValueError('lo must be non-negative')
    if hi is None:
        hi = len(a)
    while lo < hi:
        mid = (lo+hi)//2
        if key(a[mid]) < x_value:
            lo = mid+1
        else:
            hi = mid
    return lo

# Overwrite above definitions with a fast C implementation
try:
    from _bisect import *
except ImportError:
    pass

# Create aliases
bisect = bisect_right
insort = insort_right
