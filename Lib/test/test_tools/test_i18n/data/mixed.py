# Test messages and docstrings with the same msgid
from gettext import gettext as _


def test():
    """Hello, world!"""

    print(_("Hello, world!"))


_("foo")


class Foo:
    """foo"""
