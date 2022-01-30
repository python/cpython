doctests = """

Setup

    >>> class AClass:
    ...    def __init__(self):
    ...        self._setitem_name = None
    ...        self._setitem_val = None
    ...    def __setitem__(self, name, val):
    ...        self._setitem_name = name
    ...        self._setitem_val = val
    ...    def __repr__(self):
    ...        return f"A[{self._setitem_name}]={self._setitem_val}"
    ...    def __getitem__(self, name):
    ...        return ParameterisedA(name)
    >>> class ParameterisedA:
    ...    def __init__(self, name):
    ...        self._name = name
    ...    def __repr__(self):
    ...        return f"A[{self._name}]"
    ...    def __iter__(self):
    ...        for p in self._name:
    ...            yield p
    >>> class B:
    ...    def __iter__(self):
    ...        yield StarredB()
    ...    def __repr__(self):
    ...        return "B"
    >>> class StarredB:
    ...    def __repr__(self):
    ...        return "StarredB"
    >>> A = AClass()
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

Slices assignments that are supposed to work, starring our custom B class

    >>> A[*b] = 1; A
    A[(StarredB,)]=1
    >>> A[*b, *b] = 1; A
    A[(StarredB, StarredB)]=1
    >>> A[b, *b] = 1; A
    A[(B, StarredB)]=1
    >>> A[*b, b] = 1; A
    A[(StarredB, B)]=1
    >>> A[b, b, *b] = 1; A
    A[(B, B, StarredB)]=1
    >>> A[*b, b, b] = 1; A
    A[(StarredB, B, B)]=1
    >>> A[b, *b, b] = 1; A
    A[(B, StarredB, B)]=1
    >>> A[b, b, *b, b] = 1; A
    A[(B, B, StarredB, B)]=1
    >>> A[b, *b, b, b] = 1; A
    A[(B, StarredB, B, B)]=1
    >>> A[A[b, *b, b]] = 1; A
    A[A[(B, StarredB, B)]]=1
    >>> A[*A[b, *b, b]] = 1; A
    A[(B, StarredB, B)]=1
    >>> A[b, ...] = 1; A
    A[(B, Ellipsis)]=1
    >>> A[*A[b, ...]] = 1; A
    A[(B, Ellipsis)]=1

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

Slice assignments that are supposed to work, starring a list

    >>> l = [1, 2, 3]
    >>> A[*l] = 1; A
    A[(1, 2, 3)]=1
    >>> A[*l, 4] = 1; A
    A[(1, 2, 3, 4)]=1
    >>> A[0, *l] = 1; A
    A[(0, 1, 2, 3)]=1
    >>> A[1:2, *l] = 1; A
    A[(slice(1, 2, None), 1, 2, 3)]=1

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

Slice assignments that are supposed to work, starring a tuple

    >>> t = (1, 2, 3)
    >>> A[*t] = 1; A
    A[(1, 2, 3)]=1
    >>> A[*t, 4] = 1; A
    A[(1, 2, 3, 4)]=1
    >>> A[0, *t] = 1; A
    A[(0, 1, 2, 3)]=1
    >>> A[1:2, *t] = 1; A
    A[(slice(1, 2, None), 1, 2, 3)]=1

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

Starring an expression (rather than a name) in a slice assignment

    >>> def returns_list():
    ...     return [1, 2, 3]
    >>> A[returns_list()] = 1; A
    A[[1, 2, 3]]=1
    >>> A[returns_list(), 4] = 1; A
    A[([1, 2, 3], 4)]=1
    >>> A[*returns_list()] = 1; A
    A[(1, 2, 3)]=1
    >>> A[*returns_list(), 4] = 1; A
    A[(1, 2, 3, 4)]=1
    >>> A[0, *returns_list()] = 1; A
    A[(0, 1, 2, 3)]=1
    >>> A[*returns_list(), *returns_list()] = 1; A
    A[(1, 2, 3, 1, 2, 3)]=1

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

Slice assignments that should fail

    >>> A[:*b] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*b:] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[*b:*b] = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> A[**b] = 1
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

    >>> def f5(*args: *b = (1,)): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax

Other uses of starred expressions as annotations should fail

    >>> def f6(x: *b): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> def f7(x: *b = 1): pass
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> x: *b
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
    >>> x: *b = 1
    Traceback (most recent call last):
        ...
    SyntaxError: invalid syntax
"""

__test__ = {'doctests' : doctests}

def test_main(verbose=False):
    from test import support
    from test import test_pep646_syntax
    support.run_doctest(test_pep646_syntax, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
