# Test message extraction
from gettext import gettext as _

# Empty string
_("")

# Extra parentheses
(_("parentheses"))
((_("parentheses")))

# Multiline strings
_("Hello, "
  "world!")

_("""Hello,
    multiline!
""")

# Invalid arguments
_()
_(None)
_(1)
_(False)
_(x="kwargs are not allowed")
_("foo", "bar")
_("something", x="something else")

# .format()
_("Hello, {}!").format("world")  # valid
_("Hello, {}!".format("world"))  # invalid

# Nested structures
_("1"), _("2")
arr = [_("A"), _("B")]
obj = {'a': _("A"), 'b': _("B")}
{{{_('set')}}}


# Nested functions and classes
def test():
    _("nested string")  # XXX This should be extracted but isn't.
    [_("nested string")]


class Foo:
    def bar(self):
        return _("baz")


def bar(x=_('default value')):  # XXX This should be extracted but isn't.
    pass


def baz(x=[_('default value')]):  # XXX This should be extracted but isn't.
    pass


# Shadowing _()
def _(x):
    pass


def _(x="don't extract me"):
    pass
