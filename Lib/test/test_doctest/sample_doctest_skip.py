"""This is a sample module used for testing doctest.

This module includes various scenarios involving skips.
"""

# This test will pass.
def no_skip():
    """
    >>> 2 + 2
    4
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

# This test will fail.
def partial_skip():
    """
    >>> 2 + 2  # doctest: +SKIP
    4
    >>> 2 + 2
    5
    """

def no_examples():
    """A docstring with no examples should not be counted as a skip."""
