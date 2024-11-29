"""Module docstring"""

# Test docstring extraction
from gettext import gettext as _


# Empty docstring
def test(x):
    """"""


# Leading empty line
def test2(x):

    """docstring"""


# Multiline docstrings are cleaned with `inspect.cleandoc`.
def test3(x):
    """multiline
    docstring
    """


# Multiple docstrings - only the first should be extracted
def test4(x):
    """docstring1"""
    """docstring2"""


def test5(x):
    """Hello, {}!""".format("world!")  # This should not be extracted.


# Nested docstrings
def test6(x):
    def inner(y):
        """nested docstring"""


class Outer:
    class Inner:
        "nested class docstring"
