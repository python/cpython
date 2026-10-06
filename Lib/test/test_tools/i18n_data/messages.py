# Test message extraction
from gettext import (
    gettext,
    ngettext,
    pgettext,
    npgettext,
    dgettext,
    dngettext,
    dpgettext,
    dnpgettext
)

_ = gettext

# Empty string
_("")

# Extra parentheses
(_("parentheses"))
((_("parentheses")))
_(("parentheses"))

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
_(["invalid"])
_({"invalid"})
_("string"[3])
_("string"[:3])
_({"string": "foo"})

# pygettext does not allow keyword arguments, but both xgettext and pybabel do
_(x="kwargs are not allowed!")

# Unusual, but valid arguments
_("foo", "bar")
_("something", x="something else")

# .format()
_("Hello, {}!").format("world")  # valid
_("Hello, {}!".format("world"))  # invalid, but xgettext extracts the first string

# Nested structures
_("1"), _("2")
arr = [_("A"), _("B")]
obj = {'a': _("A"), 'b': _("B")}
{{{_('set')}}}


# Nested functions and classes
def test():
    _("nested string")
    [_("nested string")]


class Foo:
    def bar(self):
        return _("baz")


def bar(x=_('default value')):
    pass


def baz(x=[_('default value')]):
    pass


# Shadowing _()
def _(x):
    pass


def _(x="don't extract me"):
    pass


# Other gettext functions
gettext("foo")
ngettext("foo", "foos", 1)
pgettext("context", "foo")
npgettext("context", "foo", "foos", 1)
dgettext("domain", "foo")
dngettext("domain", "foo", "foos", 1)
dpgettext("domain", "context", "foo")
dnpgettext("domain", "context", "foo", "foos", 1)

# Complex arguments
ngettext("foo", "foos", 42 + (10 - 20))
ngettext("foo", "foos", *args)
ngettext("foo", "foos", **kwargs)
dgettext(["some", {"complex"}, ("argument",)], "domain foo")

# Invalid calls which are not extracted
gettext()
ngettext('foo')
pgettext('context')
npgettext('context', 'foo')
dgettext('domain')
dngettext('domain', 'foo')
dpgettext('domain', 'context')
dnpgettext('domain', 'context', 'foo')
dgettext(*args, 'foo')
dpgettext(*args, 'context', 'foo')
dnpgettext(*args, 'context', 'foo', 'foos')

# f-strings
f"Hello, {_('world')}!"
f"Hello, {ngettext('world', 'worlds', 3)}!"
