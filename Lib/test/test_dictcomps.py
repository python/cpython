
doctests = """

    >>> k = "old value"
    >>> { k: None for k in range(10) }
    {0: None, 1: None, 2: None, 3: None, 4: None, 5: None, 6: None, 7: None, 8: None, 9: None}
    >>> k
    'old value'

    >>> { k: k+10 for k in range(10) }
    {0: 10, 1: 11, 2: 12, 3: 13, 4: 14, 5: 15, 6: 16, 7: 17, 8: 18, 9: 19}

    >>> g = "Global variable"
    >>> { k: g for k in range(10) }
    {0: 'Global variable', 1: 'Global variable', 2: 'Global variable', 3: 'Global variable', 4: 'Global variable', 5: 'Global variable', 6: 'Global variable', 7: 'Global variable', 8: 'Global variable', 9: 'Global variable'}

    >>> { k: v for k in range(10) for v in range(10) if k == v }
    {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9}

    >>> { k: v for v in range(10) for k in range(v*9, v*10) }
    {9: 1, 18: 2, 19: 2, 27: 3, 28: 3, 29: 3, 36: 4, 37: 4, 38: 4, 39: 4, 45: 5, 46: 5, 47: 5, 48: 5, 49: 5, 54: 6, 55: 6, 56: 6, 57: 6, 58: 6, 59: 6, 63: 7, 64: 7, 65: 7, 66: 7, 67: 7, 68: 7, 69: 7, 72: 8, 73: 8, 74: 8, 75: 8, 76: 8, 77: 8, 78: 8, 79: 8, 81: 9, 82: 9, 83: 9, 84: 9, 85: 9, 86: 9, 87: 9, 88: 9, 89: 9}

    >>> { x: y for y, x in ((1, 2), (3, 4)) } = 5 # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...

    >>> { x: y for y, x in ((1, 2), (3, 4)) } += 5 # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...

"""

__test__ = {'doctests' : doctests}

def test_main(verbose=None):
    import sys
    from test import test_support
    from test import test_dictcomps
    test_support.run_doctest(test_dictcomps, verbose)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            test_support.run_doctest(test_dictcomps, verbose)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == "__main__":
    test_main(verbose=True)
