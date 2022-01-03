doctests = """

Setup

    >>> class A:
    ...    def __class_getitem__(cls, x):
    ...        return ParameterisedA(x)
    >>> class ParameterisedA:
    ...    def __init__(self, params):
    ...        self._params = params
    ...    def __repr__(self):
    ...        return f"A[{self._params}]"
    ...    def __iter__(self):
    ...        for p in self._params:
    ...            yield p
    >>> class B:
    ...    def __iter__(self):
    ...        yield StarredB()
    ...    def __repr__(self):
    ...        return "B"
    >>> class StarredB:
    ...    def __repr__(self):
    ...        return "StarredB"
    >>> b = B()

Slices that are supposed to work, starring our custom B class

    >>> A[*b]
    A[(StarredB,)]
    >>> A[*b, *b]
    A[(StarredB, StarredB)]
    >>> A[b, *b]
    A[(B, StarredB)]
    >>> A[*b, b]
    A[(StarredB, B)]
    >>> A[b, b, *b]
    A[(B, B, StarredB)]
    >>> A[*b, b, b]
    A[(StarredB, B, B)]
    >>> A[b, *b, b]
    A[(B, StarredB, B)]
    >>> A[b, b, *b, b]
    A[(B, B, StarredB, B)]
    >>> A[b, *b, b, b]
    A[(B, StarredB, B, B)]
    >>> A[A[b, *b, b]]
    A[A[(B, StarredB, B)]]
    >>> A[*A[b, *b, b]]
    A[(B, StarredB, B)]
    >>> A[b, ...]
    A[(B, Ellipsis)]
    >>> A[*A[b, ...]]
    A[(B, Ellipsis)]

Slices that are supposed to work, starring a list

    >>> l = [1, 2, 3]
    >>> A[*l]
    A[(1, 2, 3)]
    >>> A[*l, 4]
    A[(1, 2, 3, 4)]
    >>> A[0, *l]
    A[(0, 1, 2, 3)]
    >>> A[1:2, *l]
    A[(slice(1, 2, None), 1, 2, 3)]
    >>> repr(A[1:2, *l]) == repr(A[1:2, 1, 2, 3])
    True

Slices that are supposed to work, starring a tuple

    >>> t = (1, 2, 3)
    >>> A[*t]
    A[(1, 2, 3)]
    >>> A[*t, 4]
    A[(1, 2, 3, 4)]
    >>> A[0, *t]
    A[(0, 1, 2, 3)]
    >>> A[1:2, *t]
    A[(slice(1, 2, None), 1, 2, 3)]
    >>> repr(A[1:2, *t]) == repr(A[1:2, 1, 2, 3])
    True

Starring an expression (rather than a name) in a slice

    >>> def returns_list():
    ...     return [1, 2, 3]
    >>> A[returns_list()]
    A[[1, 2, 3]]
    >>> A[returns_list(), 4]
    A[([1, 2, 3], 4)]
    >>> A[*returns_list()]
    A[(1, 2, 3)]
    >>> A[*returns_list(), 4]
    A[(1, 2, 3, 4)]
    >>> A[0, *returns_list()]
    A[(0, 1, 2, 3)]
    >>> A[*returns_list(), *returns_list()]
    A[(1, 2, 3, 1, 2, 3)]

Slices that should fail

    >>> A[:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*b:]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*b:*b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[**b]
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

*args annotated as starred expression

    >>> def f1(*args: *b): pass
    >>> f1.__annotations__
    {'args': StarredB}

    >>> def f2(*args: *b, arg1): pass
    >>> f2.__annotations__
    {'args': StarredB}

    >>> def f3(*args: *b, arg1: int): pass
    >>> f3.__annotations__
    {'args': StarredB, 'arg1': <class 'int'>}

    >>> def f4(*args: *b, arg1: int = 2): pass
    >>> f4.__annotations__
    {'args': StarredB, 'arg1': <class 'int'>}

Other uses of starred expressions as annotations should fail

    >>> def f6(x: *b): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> x: *b
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

"""

__test__ = {'doctests' : doctests}

def test_main(verbose=False):
    from test import support
    from test import test_pep646
    support.run_doctest(test_pep646, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
