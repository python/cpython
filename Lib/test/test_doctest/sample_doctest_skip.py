"""This is a sample module used for testing doctest.

This module includes various scenarios involving skips.
"""

def no_skip_pass():
    """
    >>> 2 + 2
    4
    """

def no_skip_fail():
    """
    >>> 2 + 2
    5
    """

def single_skip():
    """
    >>> 2 + 2  # doctest: +SKIP
    4
    """

def double_skip():
    """
    >>> 2 + 2  # doctest: +SKIP
    4
    >>> 3 + 3  # doctest: +SKIP
    6
    """

def partial_skip_pass():
    """
    >>> 2 + 2  # doctest: +SKIP
    4
    >>> 3 + 3
    6
    """

def partial_skip_fail():
    """
    >>> 2 + 2  # doctest: +SKIP
    4
    >>> 2 + 2
    5
    """

def no_examples():
    """A docstring with no examples should not be counted as run or skipped."""
