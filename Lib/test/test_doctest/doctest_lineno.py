# This module is used in `test_doctest`.
# It must not have a docstring.

def func_with_docstring():
    """Some unrelated info."""


def func_without_docstring():
    pass


def func_with_doctest():
    """
    This function really contains a test case.

    >>> func_with_doctest.__name__
    'func_with_doctest'
    """
    return 3


class ClassWithDocstring:
    """Some unrelated class information."""


class ClassWithoutDocstring:
    pass


class ClassWithDoctest:
    """This class really has a test case in it.

    >>> ClassWithDoctest.__name__
    'ClassWithDoctest'
    """


class MethodWrapper:
    def method_with_docstring(self):
        """Method with a docstring."""

    def method_without_docstring(self):
        pass

    def method_with_doctest(self):
        """
        This has a doctest!
        >>> MethodWrapper.method_with_doctest.__name__
        'method_with_doctest'
        """

    @classmethod
    def classmethod_with_doctest(cls):
        """
        This has a doctest!
        >>> MethodWrapper.classmethod_with_doctest.__name__
        'classmethod_with_doctest'
        """

    @property
    def property_with_doctest(self):
        """
        This has a doctest!
        >>> MethodWrapper.property_with_doctest.__name__
        'property_with_doctest'
        """

# https://github.com/python/cpython/issues/99433
str_wrapper = object().__str__


# https://github.com/python/cpython/issues/115392
from test.test_doctest.decorator_mod import decorator

@decorator
@decorator
def func_with_docstring_wrapped():
    """Some unrelated info."""
