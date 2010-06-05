"""Various utility functions."""

__unittest = True

_MAX_LENGTH = 80
def safe_repr(obj, short=False):
    try:
        result = repr(obj)
    except Exception:
        result = object.__repr__(obj)
    if not short or len(result) < _MAX_LENGTH:
        return result
    return result[:_MAX_LENGTH] + ' [truncated]...'


def strclass(cls):
    return "%s.%s" % (cls.__module__, cls.__name__)

def sorted_list_difference(expected, actual):
    """Finds elements in only one or the other of two, sorted input lists.

    Returns a two-element tuple of lists.    The first list contains those
    elements in the "expected" list but not in the "actual" list, and the
    second contains those elements in the "actual" list but not in the
    "expected" list.    Duplicate elements in either input list are ignored.
    """
    i = j = 0
    missing = []
    unexpected = []
    while True:
        try:
            e = expected[i]
            a = actual[j]
            if e < a:
                missing.append(e)
                i += 1
                while expected[i] == e:
                    i += 1
            elif e > a:
                unexpected.append(a)
                j += 1
                while actual[j] == a:
                    j += 1
            else:
                i += 1
                try:
                    while expected[i] == e:
                        i += 1
                finally:
                    j += 1
                    while actual[j] == a:
                        j += 1
        except IndexError:
            missing.extend(expected[i:])
            unexpected.extend(actual[j:])
            break
    return missing, unexpected


def unorderable_list_difference(expected, actual):
    """Same behavior as sorted_list_difference but
    for lists of unorderable items (like dicts).

    As it does a linear search per item (remove) it
    has O(n*n) performance."""
    missing = []
    while expected:
        item = expected.pop()
        try:
            actual.remove(item)
        except ValueError:
            missing.append(item)

    # anything left in actual is unexpected
    return missing, actual

def three_way_cmp(x, y):
    """Return -1 if x < y, 0 if x == y and 1 if x > y"""
    return (x > y) - (x < y)
