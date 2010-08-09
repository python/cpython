import collections
import configparser
import io
import os
import unittest
import textwrap

from test import support

class SortedDict(collections.UserDict):

    def items(self):
        return sorted(self.data.items())

    def keys(self):
        return sorted(self.data.keys())

    def values(self):
        return [i[1] for i in self.items()]

    def iteritems(self): return iter(self.items())
    def iterkeys(self): return iter(self.keys())
    __iter__ = iterkeys
    def itervalues(self): return iter(self.values())


class CfgParserTestCaseClass(unittest.TestCase):
    allow_no_value = False
    delimiters = ('=', ':')
    comment_prefixes = (';', '#')
    empty_lines_in_values = True
    dict_type = configparser._default_dict
    strict = False

    def newconfig(self, defaults=None):
        arguments = dict(
            defaults=defaults,
            allow_no_value=self.allow_no_value,
            delimiters=self.delimiters,
            comment_prefixes=self.comment_prefixes,
            empty_lines_in_values=self.empty_lines_in_values,
            dict_type=self.dict_type,
            strict=self.strict,
        )
        return self.config_class(**arguments)

    def fromstring(self, string, defaults=None):
        cf = self.newconfig(defaults)
        cf.read_string(string)
        return cf

class BasicTestCase(CfgParserTestCaseClass):

    def basic_test(self, cf):
        L = cf.sections()
        L.sort()
        E = ['Commented Bar',
             'Foo Bar',
             'Internationalized Stuff',
             'Long Line',
             'Section\\with$weird%characters[\t',
             'Spaces',
             'Spacey Bar',
             'Spacey Bar From The Beginning',
             ]
        if self.allow_no_value:
            E.append(r'NoValue')
        E.sort()
        eq = self.assertEqual
        eq(L, E)

        # The use of spaces in the section names serves as a
        # regression test for SourceForge bug #583248:
        # http://www.python.org/sf/583248
        eq(cf.get('Foo Bar', 'foo'), 'bar')
        eq(cf.get('Spacey Bar', 'foo'), 'bar')
        eq(cf.get('Spacey Bar From The Beginning', 'foo'), 'bar')
        eq(cf.get('Spacey Bar From The Beginning', 'baz'), 'qwe')
        eq(cf.get('Commented Bar', 'foo'), 'bar')
        eq(cf.get('Commented Bar', 'baz'), 'qwe')
        eq(cf.get('Spaces', 'key with spaces'), 'value')
        eq(cf.get('Spaces', 'another with spaces'), 'splat!')
        if self.allow_no_value:
            eq(cf.get('NoValue', 'option-without-value'), None)

        self.assertNotIn('__name__', cf.options("Foo Bar"),
                         '__name__ "option" should not be exposed by the API!')

        # Make sure the right things happen for remove_option();
        # added to include check for SourceForge bug #123324:
        self.assertTrue(cf.remove_option('Foo Bar', 'foo'),
                        "remove_option() failed to report existence of option")
        self.assertFalse(cf.has_option('Foo Bar', 'foo'),
                    "remove_option() failed to remove option")
        self.assertFalse(cf.remove_option('Foo Bar', 'foo'),
                    "remove_option() failed to report non-existence of option"
                    " that was removed")

        with self.assertRaises(configparser.NoSectionError) as cm:
            cf.remove_option('No Such Section', 'foo')
        self.assertEqual(cm.exception.args, ('No Such Section',))

        eq(cf.get('Long Line', 'foo'),
           'this line is much, much longer than my editor\nlikes it.')

    def test_basic(self):
        config_string = """\
[Foo Bar]
foo{0[0]}bar
[Spacey Bar]
foo {0[0]} bar
[Spacey Bar From The Beginning]
  foo {0[0]} bar
  baz {0[0]} qwe
[Commented Bar]
foo{0[1]} bar {1[1]} comment
baz{0[0]}qwe {1[0]}another one
[Long Line]
foo{0[1]} this line is much, much longer than my editor
   likes it.
[Section\\with$weird%characters[\t]
[Internationalized Stuff]
foo[bg]{0[1]} Bulgarian
foo{0[0]}Default
foo[en]{0[0]}English
foo[de]{0[0]}Deutsch
[Spaces]
key with spaces {0[1]} value
another with spaces {0[0]} splat!
""".format(self.delimiters, self.comment_prefixes)
        if self.allow_no_value:
            config_string += (
                "[NoValue]\n"
                "option-without-value\n"
                )
        cf = self.fromstring(config_string)
        self.basic_test(cf)
        if self.strict:
            with self.assertRaises(configparser.DuplicateOptionError):
                cf.read_string(textwrap.dedent("""\
                    [Duplicate Options Here]
                    option {0[0]} with a value
                    option {0[1]} with another value
                """.format(self.delimiters)))
            with self.assertRaises(configparser.DuplicateSectionError):
                cf.read_string(textwrap.dedent("""\
                    [And Now For Something]
                    completely different {0[0]} True
                    [And Now For Something]
                    the larch {0[1]} 1
                """.format(self.delimiters)))
        else:
            cf.read_string(textwrap.dedent("""\
                [Duplicate Options Here]
                option {0[0]} with a value
                option {0[1]} with another value
            """.format(self.delimiters)))

            cf.read_string(textwrap.dedent("""\
                [And Now For Something]
                completely different {0[0]} True
                [And Now For Something]
                the larch {0[1]} 1
            """.format(self.delimiters)))

    def test_basic_from_dict(self):
        config = {
            "Foo Bar": {
                "foo": "bar",
            },
            "Spacey Bar": {
                "foo": "bar",
            },
            "Spacey Bar From The Beginning": {
                "foo": "bar",
                "baz": "qwe",
            },
            "Commented Bar": {
                "foo": "bar",
                "baz": "qwe",
            },
            "Long Line": {
                "foo": "this line is much, much longer than my editor\nlikes "
                       "it.",
            },
            "Section\\with$weird%characters[\t": {
            },
            "Internationalized Stuff": {
                "foo[bg]": "Bulgarian",
                "foo": "Default",
                "foo[en]": "English",
                "foo[de]": "Deutsch",
            },
            "Spaces": {
                "key with spaces": "value",
                "another with spaces": "splat!",
            }
        }
        if self.allow_no_value:
            config.update({
                "NoValue": {
                    "option-without-value": None,
                }
            })
        cf = self.newconfig()
        cf.read_dict(config)
        self.basic_test(cf)
        if self.strict:
            with self.assertRaises(configparser.DuplicateOptionError):
                cf.read_dict({
                    "Duplicate Options Here": {
                        'option': 'with a value',
                        'OPTION': 'with another value',
                    },
                })
        else:
            cf.read_dict({
                "Duplicate Options Here": {
                    'option': 'with a value',
                    'OPTION': 'with another value',
                },
            })


    def test_case_sensitivity(self):
        cf = self.newconfig()
        cf.add_section("A")
        cf.add_section("a")
        L = cf.sections()
        L.sort()
        eq = self.assertEqual
        eq(L, ["A", "a"])
        cf.set("a", "B", "value")
        eq(cf.options("a"), ["b"])
        eq(cf.get("a", "b"), "value",
           "could not locate option, expecting case-insensitive option names")
        self.assertTrue(cf.has_option("a", "b"))
        cf.set("A", "A-B", "A-B value")
        for opt in ("a-b", "A-b", "a-B", "A-B"):
            self.assertTrue(
                cf.has_option("A", opt),
                "has_option() returned false for option which should exist")
        eq(cf.options("A"), ["a-b"])
        eq(cf.options("a"), ["b"])
        cf.remove_option("a", "B")
        eq(cf.options("a"), [])

        # SF bug #432369:
        cf = self.fromstring(
            "[MySection]\nOption{} first line\n\tsecond line\n".format(
                self.delimiters[0]))
        eq(cf.options("MySection"), ["option"])
        eq(cf.get("MySection", "Option"), "first line\nsecond line")

        # SF bug #561822:
        cf = self.fromstring("[section]\n"
                             "nekey{}nevalue\n".format(self.delimiters[0]),
                             defaults={"key":"value"})
        self.assertTrue(cf.has_option("section", "Key"))


    def test_default_case_sensitivity(self):
        cf = self.newconfig({"foo": "Bar"})
        self.assertEqual(
            cf.get("DEFAULT", "Foo"), "Bar",
            "could not locate option, expecting case-insensitive option names")
        cf = self.newconfig({"Foo": "Bar"})
        self.assertEqual(
            cf.get("DEFAULT", "Foo"), "Bar",
            "could not locate option, expecting case-insensitive defaults")

    def test_parse_errors(self):
        cf = self.newconfig()
        self.parse_error(cf, configparser.ParsingError,
                         "[Foo]\n"
                         "{}val-without-opt-name\n".format(self.delimiters[0]))
        self.parse_error(cf, configparser.ParsingError,
                         "[Foo]\n"
                         "{}val-without-opt-name\n".format(self.delimiters[1]))
        e = self.parse_error(cf, configparser.MissingSectionHeaderError,
                             "No Section!\n")
        self.assertEqual(e.args, ('<???>', 1, "No Section!\n"))
        if not self.allow_no_value:
            e = self.parse_error(cf, configparser.ParsingError,
                                "[Foo]\n  wrong-indent\n")
            self.assertEqual(e.args, ('<???>',))

    def parse_error(self, cf, exc, src):
        sio = io.StringIO(src)
        with self.assertRaises(exc) as cm:
            cf.read_file(sio)
        return cm.exception

    def test_query_errors(self):
        cf = self.newconfig()
        self.assertEqual(cf.sections(), [],
                         "new ConfigParser should have no defined sections")
        self.assertFalse(cf.has_section("Foo"),
                         "new ConfigParser should have no acknowledged "
                         "sections")
        with self.assertRaises(configparser.NoSectionError):
            cf.options("Foo")
        with self.assertRaises(configparser.NoSectionError):
            cf.set("foo", "bar", "value")
        e = self.get_error(cf, configparser.NoSectionError, "foo", "bar")
        self.assertEqual(e.args, ("foo",))
        cf.add_section("foo")
        e = self.get_error(cf, configparser.NoOptionError, "foo", "bar")
        self.assertEqual(e.args, ("bar", "foo"))

    def get_error(self, cf, exc, section, option):
        try:
            cf.get(section, option)
        except exc as e:
            return e
        else:
            self.fail("expected exception type %s.%s"
                      % (exc.__module__, exc.__name__))

    def test_boolean(self):
        cf = self.fromstring(
            "[BOOLTEST]\n"
            "T1{equals}1\n"
            "T2{equals}TRUE\n"
            "T3{equals}True\n"
            "T4{equals}oN\n"
            "T5{equals}yes\n"
            "F1{equals}0\n"
            "F2{equals}FALSE\n"
            "F3{equals}False\n"
            "F4{equals}oFF\n"
            "F5{equals}nO\n"
            "E1{equals}2\n"
            "E2{equals}foo\n"
            "E3{equals}-1\n"
            "E4{equals}0.1\n"
            "E5{equals}FALSE AND MORE".format(equals=self.delimiters[0])
            )
        for x in range(1, 5):
            self.assertTrue(cf.getboolean('BOOLTEST', 't%d' % x))
            self.assertFalse(cf.getboolean('BOOLTEST', 'f%d' % x))
            self.assertRaises(ValueError,
                              cf.getboolean, 'BOOLTEST', 'e%d' % x)

    def test_weird_errors(self):
        cf = self.newconfig()
        cf.add_section("Foo")
        with self.assertRaises(configparser.DuplicateSectionError) as cm:
            cf.add_section("Foo")
        e = cm.exception
        self.assertEqual(str(e), "Section 'Foo' already exists")
        self.assertEqual(e.args, ("Foo", None, None))

        if self.strict:
            with self.assertRaises(configparser.DuplicateSectionError) as cm:
                cf.read_string(textwrap.dedent("""\
                    [Foo]
                    will this be added{equals}True
                    [Bar]
                    what about this{equals}True
                    [Foo]
                    oops{equals}this won't
                """.format(equals=self.delimiters[0])), source='<foo-bar>')
            e = cm.exception
            self.assertEqual(str(e), "While reading from <foo-bar> [line  5]: "
                                     "section 'Foo' already exists")
            self.assertEqual(e.args, ("Foo", '<foo-bar>', 5))

            with self.assertRaises(configparser.DuplicateOptionError) as cm:
                cf.read_dict({'Bar': {'opt': 'val', 'OPT': 'is really `opt`'}})
            e = cm.exception
            self.assertEqual(str(e), "While reading from <dict>: option 'opt' "
                                     "in section 'Bar' already exists")
            self.assertEqual(e.args, ("Bar", "opt", "<dict>", None))

    def test_write(self):
        config_string = (
            "[Long Line]\n"
            "foo{0[0]} this line is much, much longer than my editor\n"
            "   likes it.\n"
            "[DEFAULT]\n"
            "foo{0[1]} another very\n"
            " long line\n"
            "[Long Line - With Comments!]\n"
            "test {0[1]} we        {comment} can\n"
            "            also      {comment} place\n"
            "            comments  {comment} in\n"
            "            multiline {comment} values"
            "\n".format(self.delimiters, comment=self.comment_prefixes[0])
            )
        if self.allow_no_value:
            config_string += (
            "[Valueless]\n"
            "option-without-value\n"
            )

        cf = self.fromstring(config_string)
        output = io.StringIO()
        cf.write(output)
        expect_string = (
            "[DEFAULT]\n"
            "foo {equals} another very\n"
            "\tlong line\n"
            "\n"
            "[Long Line]\n"
            "foo {equals} this line is much, much longer than my editor\n"
            "\tlikes it.\n"
            "\n"
            "[Long Line - With Comments!]\n"
            "test {equals} we\n"
            "\talso\n"
            "\tcomments\n"
            "\tmultiline\n"
            "\n".format(equals=self.delimiters[0])
            )
        if self.allow_no_value:
            expect_string += (
                "[Valueless]\n"
                "option-without-value\n"
                "\n"
                )
        self.assertEqual(output.getvalue(), expect_string)

    def test_set_string_types(self):
        cf = self.fromstring("[sect]\n"
                             "option1{eq}foo\n".format(eq=self.delimiters[0]))
        # Check that we don't get an exception when setting values in
        # an existing section using strings:
        class mystr(str):
            pass
        cf.set("sect", "option1", "splat")
        cf.set("sect", "option1", mystr("splat"))
        cf.set("sect", "option2", "splat")
        cf.set("sect", "option2", mystr("splat"))
        cf.set("sect", "option1", "splat")
        cf.set("sect", "option2", "splat")

    def test_read_returns_file_list(self):
        if self.delimiters[0] != '=':
            # skip reading the file if we're using an incompatible format
            return
        file1 = support.findfile("cfgparser.1")
        # check when we pass a mix of readable and non-readable files:
        cf = self.newconfig()
        parsed_files = cf.read([file1, "nonexistent-file"])
        self.assertEqual(parsed_files, [file1])
        self.assertEqual(cf.get("Foo Bar", "foo"), "newbar")
        # check when we pass only a filename:
        cf = self.newconfig()
        parsed_files = cf.read(file1)
        self.assertEqual(parsed_files, [file1])
        self.assertEqual(cf.get("Foo Bar", "foo"), "newbar")
        # check when we pass only missing files:
        cf = self.newconfig()
        parsed_files = cf.read(["nonexistent-file"])
        self.assertEqual(parsed_files, [])
        # check when we pass no files:
        cf = self.newconfig()
        parsed_files = cf.read([])
        self.assertEqual(parsed_files, [])

    # shared by subclasses
    def get_interpolation_config(self):
        return self.fromstring(
            "[Foo]\n"
            "bar{equals}something %(with1)s interpolation (1 step)\n"
            "bar9{equals}something %(with9)s lots of interpolation (9 steps)\n"
            "bar10{equals}something %(with10)s lots of interpolation (10 steps)\n"
            "bar11{equals}something %(with11)s lots of interpolation (11 steps)\n"
            "with11{equals}%(with10)s\n"
            "with10{equals}%(with9)s\n"
            "with9{equals}%(with8)s\n"
            "with8{equals}%(With7)s\n"
            "with7{equals}%(WITH6)s\n"
            "with6{equals}%(with5)s\n"
            "With5{equals}%(with4)s\n"
            "WITH4{equals}%(with3)s\n"
            "with3{equals}%(with2)s\n"
            "with2{equals}%(with1)s\n"
            "with1{equals}with\n"
            "\n"
            "[Mutual Recursion]\n"
            "foo{equals}%(bar)s\n"
            "bar{equals}%(foo)s\n"
            "\n"
            "[Interpolation Error]\n"
            "name{equals}%(reference)s\n".format(equals=self.delimiters[0]),
            # no definition for 'reference'
            defaults={"getname": "%(__name__)s"})

    def check_items_config(self, expected):
        cf = self.fromstring(
            "[section]\n"
            "name {0[0]} value\n"
            "key{0[1]} |%(name)s| \n"
            "getdefault{0[1]} |%(default)s|\n"
            "getname{0[1]} |%(__name__)s|".format(self.delimiters),
            defaults={"default": "<default>"})
        L = list(cf.items("section"))
        L.sort()
        self.assertEqual(L, expected)


class StrictTestCase(BasicTestCase):
    config_class = configparser.RawConfigParser
    strict = True


class ConfigParserTestCase(BasicTestCase):
    config_class = configparser.ConfigParser

    def test_interpolation(self):
        rawval = {
            configparser.ConfigParser: ("something %(with11)s "
                                        "lots of interpolation (11 steps)"),
            configparser.SafeConfigParser: "%(with1)s",
        }
        cf = self.get_interpolation_config()
        eq = self.assertEqual
        eq(cf.get("Foo", "getname"), "Foo")
        eq(cf.get("Foo", "bar"), "something with interpolation (1 step)")
        eq(cf.get("Foo", "bar9"),
           "something with lots of interpolation (9 steps)")
        eq(cf.get("Foo", "bar10"),
           "something with lots of interpolation (10 steps)")
        e = self.get_error(cf, configparser.InterpolationDepthError, "Foo", "bar11")
        self.assertEqual(e.args, ("bar11", "Foo", rawval[self.config_class]))

    def test_interpolation_missing_value(self):
        rawval = {
            configparser.ConfigParser: '%(reference)s',
            configparser.SafeConfigParser: '',
        }
        cf = self.get_interpolation_config()
        e = self.get_error(cf, configparser.InterpolationMissingOptionError,
                           "Interpolation Error", "name")
        self.assertEqual(e.reference, "reference")
        self.assertEqual(e.section, "Interpolation Error")
        self.assertEqual(e.option, "name")
        self.assertEqual(e.args, ('name', 'Interpolation Error',
                                  rawval[self.config_class], 'reference'))

    def test_items(self):
        self.check_items_config([('default', '<default>'),
                                 ('getdefault', '|<default>|'),
                                 ('getname', '|section|'),
                                 ('key', '|value|'),
                                 ('name', 'value')])

    def test_set_nonstring_types(self):
        cf = self.newconfig()
        cf.add_section('non-string')
        cf.set('non-string', 'int', 1)
        cf.set('non-string', 'list', [0, 1, 1, 2, 3, 5, 8, 13, '%('])
        cf.set('non-string', 'dict', {'pi': 3.14159, '%(': 1,
                                      '%(list)': '%(list)'})
        cf.set('non-string', 'string_with_interpolation', '%(list)s')
        self.assertEqual(cf.get('non-string', 'int', raw=True), 1)
        self.assertRaises(TypeError, cf.get, 'non-string', 'int')
        self.assertEqual(cf.get('non-string', 'list', raw=True),
                         [0, 1, 1, 2, 3, 5, 8, 13, '%('])
        self.assertRaises(TypeError, cf.get, 'non-string', 'list')
        self.assertEqual(cf.get('non-string', 'dict', raw=True),
                         {'pi': 3.14159, '%(': 1, '%(list)': '%(list)'})
        self.assertRaises(TypeError, cf.get, 'non-string', 'dict')
        self.assertEqual(cf.get('non-string', 'string_with_interpolation',
                                raw=True), '%(list)s')
        self.assertRaises(ValueError, cf.get, 'non-string',
                          'string_with_interpolation', raw=False)

class ConfigParserTestCaseNonStandardDelimiters(ConfigParserTestCase):
    delimiters = (':=', '$')
    comment_prefixes = ('//', '"')

class MultilineValuesTestCase(BasicTestCase):
    config_class = configparser.ConfigParser
    wonderful_spam = ("I'm having spam spam spam spam "
                      "spam spam spam beaked beans spam "
                      "spam spam and spam!").replace(' ', '\t\n')

    def setUp(self):
        cf = self.newconfig()
        for i in range(100):
            s = 'section{}'.format(i)
            cf.add_section(s)
            for j in range(10):
                cf.set(s, 'lovely_spam{}'.format(j), self.wonderful_spam)
        with open(support.TESTFN, 'w') as f:
            cf.write(f)

    def tearDown(self):
        os.unlink(support.TESTFN)

    def test_dominating_multiline_values(self):
        # We're reading from file because this is where the code changed
        # during performance updates in Python 3.2
        cf_from_file = self.newconfig()
        with open(support.TESTFN) as f:
            cf_from_file.read_file(f)
        self.assertEqual(cf_from_file.get('section8', 'lovely_spam4'),
                         self.wonderful_spam.replace('\t\n', '\n'))

class RawConfigParserTestCase(BasicTestCase):
    config_class = configparser.RawConfigParser

    def test_interpolation(self):
        cf = self.get_interpolation_config()
        eq = self.assertEqual
        eq(cf.get("Foo", "getname"), "%(__name__)s")
        eq(cf.get("Foo", "bar"),
           "something %(with1)s interpolation (1 step)")
        eq(cf.get("Foo", "bar9"),
           "something %(with9)s lots of interpolation (9 steps)")
        eq(cf.get("Foo", "bar10"),
           "something %(with10)s lots of interpolation (10 steps)")
        eq(cf.get("Foo", "bar11"),
           "something %(with11)s lots of interpolation (11 steps)")

    def test_items(self):
        self.check_items_config([('default', '<default>'),
                                 ('getdefault', '|%(default)s|'),
                                 ('getname', '|%(__name__)s|'),
                                 ('key', '|%(name)s|'),
                                 ('name', 'value')])

    def test_set_nonstring_types(self):
        cf = self.newconfig()
        cf.add_section('non-string')
        cf.set('non-string', 'int', 1)
        cf.set('non-string', 'list', [0, 1, 1, 2, 3, 5, 8, 13])
        cf.set('non-string', 'dict', {'pi': 3.14159})
        self.assertEqual(cf.get('non-string', 'int'), 1)
        self.assertEqual(cf.get('non-string', 'list'),
                         [0, 1, 1, 2, 3, 5, 8, 13])
        self.assertEqual(cf.get('non-string', 'dict'), {'pi': 3.14159})

class RawConfigParserTestCaseNonStandardDelimiters(RawConfigParserTestCase):
    delimiters = (':=', '$')
    comment_prefixes = ('//', '"')

class RawConfigParserTestSambaConf(BasicTestCase):
    config_class = configparser.RawConfigParser
    comment_prefixes = ('#', ';', '//', '----')
    empty_lines_in_values = False

    def test_reading(self):
        smbconf = support.findfile("cfgparser.2")
        # check when we pass a mix of readable and non-readable files:
        cf = self.newconfig()
        parsed_files = cf.read([smbconf, "nonexistent-file"], encoding='utf-8')
        self.assertEqual(parsed_files, [smbconf])
        sections = ['global', 'homes', 'printers',
                    'print$', 'pdf-generator', 'tmp', 'Agustin']
        self.assertEqual(cf.sections(), sections)
        self.assertEqual(cf.get("global", "workgroup"), "MDKGROUP")
        self.assertEqual(cf.getint("global", "max log size"), 50)
        self.assertEqual(cf.get("global", "hosts allow"), "127.")
        self.assertEqual(cf.get("tmp", "echo command"), "cat %s; rm %s")

class SafeConfigParserTestCase(ConfigParserTestCase):
    config_class = configparser.SafeConfigParser

    def test_safe_interpolation(self):
        # See http://www.python.org/sf/511737
        cf = self.fromstring("[section]\n"
                             "option1{eq}xxx\n"
                             "option2{eq}%(option1)s/xxx\n"
                             "ok{eq}%(option1)s/%%s\n"
                             "not_ok{eq}%(option2)s/%%s".format(
                                 eq=self.delimiters[0]))
        self.assertEqual(cf.get("section", "ok"), "xxx/%s")
        self.assertEqual(cf.get("section", "not_ok"), "xxx/xxx/%s")

    def test_set_malformatted_interpolation(self):
        cf = self.fromstring("[sect]\n"
                             "option1{eq}foo\n".format(eq=self.delimiters[0]))

        self.assertEqual(cf.get('sect', "option1"), "foo")

        self.assertRaises(ValueError, cf.set, "sect", "option1", "%foo")
        self.assertRaises(ValueError, cf.set, "sect", "option1", "foo%")
        self.assertRaises(ValueError, cf.set, "sect", "option1", "f%oo")

        self.assertEqual(cf.get('sect', "option1"), "foo")

        # bug #5741: double percents are *not* malformed
        cf.set("sect", "option2", "foo%%bar")
        self.assertEqual(cf.get("sect", "option2"), "foo%bar")

    def test_set_nonstring_types(self):
        cf = self.fromstring("[sect]\n"
                             "option1{eq}foo\n".format(eq=self.delimiters[0]))
        # Check that we get a TypeError when setting non-string values
        # in an existing section:
        self.assertRaises(TypeError, cf.set, "sect", "option1", 1)
        self.assertRaises(TypeError, cf.set, "sect", "option1", 1.0)
        self.assertRaises(TypeError, cf.set, "sect", "option1", object())
        self.assertRaises(TypeError, cf.set, "sect", "option2", 1)
        self.assertRaises(TypeError, cf.set, "sect", "option2", 1.0)
        self.assertRaises(TypeError, cf.set, "sect", "option2", object())

    def test_add_section_default_1(self):
        cf = self.newconfig()
        self.assertRaises(ValueError, cf.add_section, "default")

    def test_add_section_default_2(self):
        cf = self.newconfig()
        self.assertRaises(ValueError, cf.add_section, "DEFAULT")

class SafeConfigParserTestCaseNonStandardDelimiters(SafeConfigParserTestCase):
    delimiters = (':=', '$')
    comment_prefixes = ('//', '"')

class SafeConfigParserTestCaseNoValue(SafeConfigParserTestCase):
    allow_no_value = True

class SafeConfigParserTestCaseTrickyFile(CfgParserTestCaseClass):
    config_class = configparser.SafeConfigParser
    delimiters = {'='}
    comment_prefixes = {'#'}
    allow_no_value = True

    def test_cfgparser_dot_3(self):
        tricky = support.findfile("cfgparser.3")
        cf = self.newconfig()
        self.assertEqual(len(cf.read(tricky, encoding='utf-8')), 1)
        self.assertEqual(cf.sections(), ['strange',
                                         'corruption',
                                         'yeah, sections can be '
                                         'indented as well',
                                         'another one!',
                                         'no values here',
                                         'tricky interpolation',
                                         'more interpolation'])
        #self.assertEqual(cf.getint('DEFAULT', 'go', vars={'interpolate': '-1'}),
        #                 -1)
        self.assertEqual(len(cf.get('strange', 'other').split('\n')), 4)
        self.assertEqual(len(cf.get('corruption', 'value').split('\n')), 10)
        longname = 'yeah, sections can be indented as well'
        self.assertFalse(cf.getboolean(longname, 'are they subsections'))
        self.assertEquals(cf.get(longname, 'lets use some Unicode'),
                                           '片仮名')
        self.assertEqual(len(cf.items('another one!')), 5) # 4 in section and
                                                           # `go` from DEFAULT
        with self.assertRaises(configparser.InterpolationMissingOptionError):
            cf.items('no values here')
        self.assertEqual(cf.get('tricky interpolation', 'lets'), 'do this')
        self.assertEqual(cf.get('tricky interpolation', 'lets'),
                         cf.get('tricky interpolation', 'go'))
        self.assertEqual(cf.get('more interpolation', 'lets'), 'go shopping')

    def test_unicode_failure(self):
        tricky = support.findfile("cfgparser.3")
        cf = self.newconfig()
        with self.assertRaises(UnicodeDecodeError):
            cf.read(tricky, encoding='ascii')

class SortedTestCase(RawConfigParserTestCase):
    dict_type = SortedDict

    def test_sorted(self):
        cf = self.fromstring("[b]\n"
                             "o4=1\n"
                             "o3=2\n"
                             "o2=3\n"
                             "o1=4\n"
                             "[a]\n"
                             "k=v\n")
        output = io.StringIO()
        cf.write(output)
        self.assertEquals(output.getvalue(),
                          "[a]\n"
                          "k = v\n\n"
                          "[b]\n"
                          "o1 = 4\n"
                          "o2 = 3\n"
                          "o3 = 2\n"
                          "o4 = 1\n\n")


class CompatibleTestCase(CfgParserTestCaseClass):
    config_class = configparser.RawConfigParser
    comment_prefixes = configparser.RawConfigParser._COMPATIBLE

    def test_comment_handling(self):
        config_string = textwrap.dedent("""\
        [Commented Bar]
        baz=qwe ; a comment
        foo: bar # not a comment!
        # but this is a comment
        ; another comment
        quirk: this;is not a comment
        ; a space must precede an inline comment
        """)
        cf = self.fromstring(config_string)
        self.assertEqual(cf.get('Commented Bar', 'foo'), 'bar # not a comment!')
        self.assertEqual(cf.get('Commented Bar', 'baz'), 'qwe')
        self.assertEqual(cf.get('Commented Bar', 'quirk'), 'this;is not a comment')


def test_main():
    support.run_unittest(
        ConfigParserTestCase,
        ConfigParserTestCaseNonStandardDelimiters,
        MultilineValuesTestCase,
        RawConfigParserTestCase,
        RawConfigParserTestCaseNonStandardDelimiters,
        RawConfigParserTestSambaConf,
        SafeConfigParserTestCase,
        SafeConfigParserTestCaseNonStandardDelimiters,
        SafeConfigParserTestCaseNoValue,
        SafeConfigParserTestCaseTrickyFile,
        SortedTestCase,
        StrictTestCase,
        CompatibleTestCase,
        )


if __name__ == "__main__":
    test_main()
