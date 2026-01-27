# Test file locations
from gettext import gettext as _

# Duplicate strings
_('foo')
_('foo')

# Duplicate strings on the same line should only add one location to the output
_('bar'), _('bar')


# Duplicate docstrings
class A:
    """docstring"""


def f():
    """docstring"""


# Duplicate message and docstring
_('baz')


def g():
    """baz"""
