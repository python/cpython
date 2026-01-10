"""This is a sample module used for testing doctest.

This module includes various scenarios involving errors.

>>> 2 + 2
5
>>> 1/0
1
"""

def g():
    [][0] # line 12

def errors():
    """
    >>> 2 + 2
    5
    >>> 1/0
    1
    >>> def f():
    ...     2 + '2'
    ...
    >>> f()
    1
    >>> g()
    1
    """

def syntax_error():
    """
    >>> 2+*3
    5
    """

__test__ = {
    'bad':  """
        >>> 2 + 2
        5
        >>> 1/0
        1
        """,
}

def test_suite():
    import doctest
    return doctest.DocTestSuite()
