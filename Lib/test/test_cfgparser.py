import ConfigParser
import StringIO

from test_support import TestFailed, verify


def basic(src):
    print "Testing basic accessors..."
    cf = ConfigParser.ConfigParser()
    sio = StringIO.StringIO(src)
    cf.readfp(sio)
    L = cf.sections()
    L.sort()
    verify(L == [r'Commented Bar',
                 r'Foo Bar',
                 r'Internationalized Stuff',
                 r'Section\with$weird%characters[' '\t',
                 r'Spacey Bar',
                 ],
           "unexpected list of section names")

    # The use of spaces in the section names serves as a regression test for
    # SourceForge bug #115357.
    # http://sourceforge.net/bugs/?func=detailbug&group_id=5470&bug_id=115357
    verify(cf.get('Foo Bar', 'foo', raw=1) == 'bar')
    verify(cf.get('Spacey Bar', 'foo', raw=1) == 'bar')
    verify(cf.get('Commented Bar', 'foo', raw=1) == 'bar')

    verify('__name__' not in cf.options("Foo Bar"),
           '__name__ "option" should not be exposed by the API!')

    # Make sure the right things happen for remove_option();
    # added to include check for SourceForge bug #123324:
    verify(cf.remove_option('Foo Bar', 'foo'),
           "remove_option() failed to report existance of option")
    verify(not cf.has_option('Foo Bar', 'foo'),
           "remove_option() failed to remove option")
    verify(not cf.remove_option('Foo Bar', 'foo'),
           "remove_option() failed to report non-existance of option"
           " that was removed")
    try:
        cf.remove_option('No Such Section', 'foo')
    except ConfigParser.NoSectionError:
        pass
    else:
        raise TestFailed(
            "remove_option() failed to report non-existance of option"
            " that never existed")


def case_sensitivity():
    print "Testing case sensitivity..."
    cf = ConfigParser.ConfigParser()
    cf.add_section("A")
    cf.add_section("a")
    L = cf.sections()
    L.sort()
    verify(L == ["A", "a"])
    cf.set("a", "B", "value")
    verify(cf.options("a") == ["b"])
    verify(cf.get("a", "b", raw=1) == "value",
           "could not locate option, expecting case-insensitive option names")
    verify(cf.has_option("a", "b"))
    cf.set("A", "A-B", "A-B value")
    for opt in ("a-b", "A-b", "a-B", "A-B"):
        verify(cf.has_option("A", opt),
               "has_option() returned false for option which should exist")
    verify(cf.options("A") == ["a-b"])
    verify(cf.options("a") == ["b"])
    cf.remove_option("a", "B")
    verify(cf.options("a") == [])

    # SF bug #432369:
    cf = ConfigParser.ConfigParser()
    sio = StringIO.StringIO("[MySection]\nOption: first line\n\tsecond line\n")
    cf.readfp(sio)
    verify(cf.options("MySection") == ["option"])
    verify(cf.get("MySection", "Option") == "first line\nsecond line")


def interpolation(src):
    print "Testing value interpolation..."
    cf = ConfigParser.ConfigParser({"getname": "%(__name__)s"})
    sio = StringIO.StringIO(src)
    cf.readfp(sio)
    verify(cf.get("Foo", "getname") == "Foo")
    verify(cf.get("Foo", "bar") == "something with interpolation (1 step)")
    verify(cf.get("Foo", "bar9")
           == "something with lots of interpolation (9 steps)")
    verify(cf.get("Foo", "bar10")
           == "something with lots of interpolation (10 steps)")
    expect_get_error(cf, ConfigParser.InterpolationDepthError, "Foo", "bar11")


def parse_errors():
    print "Testing parse errors..."
    expect_parse_error(ConfigParser.ParsingError,
                       """[Foo]\n  extra-spaces: splat\n""")
    expect_parse_error(ConfigParser.ParsingError,
                       """[Foo]\n  extra-spaces= splat\n""")
    expect_parse_error(ConfigParser.ParsingError,
                       """[Foo]\noption-without-value\n""")
    expect_parse_error(ConfigParser.ParsingError,
                       """[Foo]\n:value-without-option-name\n""")
    expect_parse_error(ConfigParser.ParsingError,
                       """[Foo]\n=value-without-option-name\n""")
    expect_parse_error(ConfigParser.MissingSectionHeaderError,
                       """No Section!\n""")


def query_errors():
    print "Testing query interface..."
    cf = ConfigParser.ConfigParser()
    verify(cf.sections() == [],
           "new ConfigParser should have no defined sections")
    verify(not cf.has_section("Foo"),
           "new ConfigParser should have no acknowledged sections")
    try:
        cf.options("Foo")
    except ConfigParser.NoSectionError, e:
        pass
    else:
        raise TestFailed(
            "Failed to catch expected NoSectionError from options()")
    try:
        cf.set("foo", "bar", "value")
    except ConfigParser.NoSectionError, e:
        pass
    else:
        raise TestFailed("Failed to catch expected NoSectionError from set()")
    expect_get_error(cf, ConfigParser.NoSectionError, "foo", "bar")
    cf.add_section("foo")
    expect_get_error(cf, ConfigParser.NoOptionError, "foo", "bar")


def weird_errors():
    print "Testing miscellaneous error conditions..."
    cf = ConfigParser.ConfigParser()
    cf.add_section("Foo")
    try:
        cf.add_section("Foo")
    except ConfigParser.DuplicateSectionError, e:
        pass
    else:
        raise TestFailed("Failed to catch expected DuplicateSectionError")


def expect_get_error(cf, exctype, section, option, raw=0):
    try:
        cf.get(section, option, raw=raw)
    except exctype, e:
        pass
    else:
        raise TestFailed("Failed to catch expected " + exctype.__name__)


def expect_parse_error(exctype, src):
    cf = ConfigParser.ConfigParser()
    sio = StringIO.StringIO(src)
    try:
        cf.readfp(sio)
    except exctype, e:
        pass
    else:
        raise TestFailed("Failed to catch expected " + exctype.__name__)


basic(r"""
[Foo Bar]
foo=bar
[Spacey Bar]
foo = bar
[Commented Bar]
foo: bar ; comment
[Section\with$weird%characters[""" '\t' r"""]
[Internationalized Stuff]
foo[bg]: Bulgarian
foo=Default
foo[en]=English
foo[de]=Deutsch
""")
case_sensitivity()
interpolation(r"""
[Foo]
bar=something %(with1)s interpolation (1 step)
bar9=something %(with9)s lots of interpolation (9 steps)
bar10=something %(with10)s lots of interpolation (10 steps)
bar11=something %(with11)s lots of interpolation (11 steps)
with11=%(with10)s
with10=%(with9)s
with9=%(with8)s
with8=%(with7)s
with7=%(with6)s
with6=%(with5)s
with5=%(with4)s
with4=%(with3)s
with3=%(with2)s
with2=%(with1)s
with1=with

[Mutual Recursion]
foo=%(bar)s
bar=%(foo)s
""")
parse_errors()
query_errors()
weird_errors()
