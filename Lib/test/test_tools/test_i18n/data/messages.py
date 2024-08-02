# Test message extraction
from gettext import gettext as _


_("Hello, world!")

_("Hello, "
  "world!")

_("""Hello,
    multiline!
""")

(_("parentheses"))

_(r"Raw string \ \n \t")

_(u"unicode string")

_(r"raw" """doc""" u"unicode" "standard")

_(f"f-strings should not get extracted")

_(f"f-strings should not get {'extracted'}")

text = 'extracted'
_(f"f-strings should not get {text}")

_(b"bytes should not be extracted")

_("Some non-ascii text: ěščř αβγδ")

_("Some very long text which should wrap correctly into multiple lines while respecting the maximum line length")

_("foo"), _("bar")

_("baz"), _("baz")

_("")

_()

_(None)

_(1)

_(False)

_(x="no kw arguments")

_("foo", "bar")

_("something", x="something else")

_("Hello, {}!").format("world")

_("Hello, {}!".format("world"))

arr = [_("A"), _("B")]
obj = {'a': _("A"), 'b': _("B")}


def test():
    print(_("nested"))


def test2(x=_("param")):
    pass


class Foo:
    def bar(self):
        return _("baz")


def _(x):
    pass


def _(x="don't extract me"):
    pass
