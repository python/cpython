import ConfigParser
import StringIO

def basic(src):
    print
    print "Testing basic accessors..."
    cf = ConfigParser.ConfigParser()
    sio = StringIO.StringIO(src)
    cf.readfp(sio)
    L = cf.sections()
    L.sort()
    print L
    for s in L:
        print "%s: %s" % (s, cf.options(s))

    # The use of spaces in the section names serves as a regression test for
    # SourceForge bug #115357.
    # http://sourceforge.net/bugs/?func=detailbug&group_id=5470&bug_id=115357
    print `cf.get('Foo Bar', 'foo', raw=1)`
    print `cf.get('Spacey Bar', 'foo', raw=1)`
    print `cf.get('Commented Bar', 'foo', raw=1)`

    if '__name__' in cf.options("Foo Bar"):
        print '__name__ "option" should not be exposed by the API!'
    else:
        print '__name__ "option" properly hidden by the API.'

def interpolation(src):
    print
    print "Testing value interpolation..."
    cf = ConfigParser.ConfigParser({"getname": "%(__name__)s"})
    sio = StringIO.StringIO(src)
    cf.readfp(sio)
    print `cf.get("Foo", "getname")`
    print `cf.get("Foo", "bar")`
    print `cf.get("Foo", "bar9")`
    print `cf.get("Foo", "bar10")`
    expect_get_error(cf, ConfigParser.InterpolationDepthError, "Foo", "bar11")

def parse_errors():
    print
    print "Testing for parsing errors..."
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
    print
    print "Testing query interface..."
    cf = ConfigParser.ConfigParser()
    print cf.sections()
    print "Has section 'Foo'?", cf.has_section("Foo")
    try:
        cf.options("Foo")
    except ConfigParser.NoSectionError, e:
        print "Caught expected NoSectionError:", e
    else:
        print "Failed to catch expected NoSectionError from options()"
    try:
        cf.set("foo", "bar", "value")
    except ConfigParser.NoSectionError, e:
        print "Caught expected NoSectionError:", e
    else:
        print "Failed to catch expected NoSectionError from set()"
    expect_get_error(cf, ConfigParser.NoSectionError, "foo", "bar")
    cf.add_section("foo")
    expect_get_error(cf, ConfigParser.NoOptionError, "foo", "bar")

def weird_errors():
    print
    print "Testing miscellaneous error conditions..."
    cf = ConfigParser.ConfigParser()
    cf.add_section("Foo")
    try:
        cf.add_section("Foo")
    except ConfigParser.DuplicateSectionError, e:
        print "Caught expected DuplicateSectionError:", e
    else:
        print "Failed to catch expected DuplicateSectionError"

def expect_get_error(cf, exctype, section, option, raw=0):
    try:
        cf.get(section, option, raw=raw)
    except exctype, e:
        print "Caught expected", exctype.__name__, ":"
        print e
    else:
        print "Failed to catch expected", exctype.__name__

def expect_parse_error(exctype, src):
    cf = ConfigParser.ConfigParser()
    sio = StringIO.StringIO(src)
    try:
        cf.readfp(sio)
    except exctype, e:
        print "Caught expected exception:", e
    else:
        print "Failed to catch expected", exctype.__name__

basic(r"""
[Foo Bar]
foo=bar
[Spacey Bar]
foo = bar 
[Commented Bar]
foo: bar ; comment
""")
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
