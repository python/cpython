#
# Test suite for Optik.  Supplied by Johannes Gijsbers
# (taradino@softhome.net) -- translated from the original Optik
# test suite to this PyUnit-based version.
#
# $Id$
#

import sys
import os
import re
import copy
import unittest

from io import StringIO
from test import support


from optparse import make_option, Option, \
     TitledHelpFormatter, OptionParser, OptionGroup, \
     SUPPRESS_USAGE, OptionError, OptionConflictError, \
     BadOptionError, OptionValueError, Values
from optparse import _match_abbrev
from optparse import _parse_num

retype = type(re.compile(''))

class InterceptedError(Exception):
    def __init__(self,
                 error_message=None,
                 exit_status=None,
                 exit_message=None):
        self.error_message = error_message
        self.exit_status = exit_status
        self.exit_message = exit_message

    def __str__(self):
        return self.error_message or self.exit_message or "intercepted error"

class InterceptingOptionParser(OptionParser):
    def exit(self, status=0, msg=None):
        raise InterceptedError(exit_status=status, exit_message=msg)

    def error(self, msg):
        raise InterceptedError(error_message=msg)


class BaseTest(unittest.TestCase):
    def assertParseOK(self, args, expected_opts, expected_positional_args):
        """Assert the options are what we expected when parsing arguments.

        Otherwise, fail with a nicely formatted message.

        Keyword arguments:
        args -- A list of arguments to parse with OptionParser.
        expected_opts -- The options expected.
        expected_positional_args -- The positional arguments expected.

        Returns the options and positional args for further testing.
        """

        (options, positional_args) = self.parser.parse_args(args)
        optdict = vars(options)

        self.assertEqual(optdict, expected_opts,
                         """
Options are %(optdict)s.
Should be %(expected_opts)s.
Args were %(args)s.""" % locals())

        self.assertEqual(positional_args, expected_positional_args,
                         """
Positional arguments are %(positional_args)s.
Should be %(expected_positional_args)s.
Args were %(args)s.""" % locals ())

        return (options, positional_args)

    def assertRaises(self,
                     func,
                     args,
                     kwargs,
                     expected_exception,
                     expected_message):
        """
        Assert that the expected exception is raised when calling a
        function, and that the right error message is included with
        that exception.

        Arguments:
          func -- the function to call
          args -- positional arguments to `func`
          kwargs -- keyword arguments to `func`
          expected_exception -- exception that should be raised
          expected_message -- expected exception message (or pattern
            if a compiled regex object)

        Returns the exception raised for further testing.
        """
        if args is None:
            args = ()
        if kwargs is None:
            kwargs = {}

        try:
            func(*args, **kwargs)
        except expected_exception as err:
            actual_message = str(err)
            if isinstance(expected_message, retype):
                self.assertTrue(expected_message.search(actual_message),
                             """\
expected exception message pattern:
/%s/
actual exception message:
'''%s'''
""" % (expected_message.pattern, actual_message))
            else:
                self.assertEqual(actual_message,
                                 expected_message,
                                 """\
expected exception message:
'''%s'''
actual exception message:
'''%s'''
""" % (expected_message, actual_message))

            return err
        else:
            self.fail("""expected exception %(expected_exception)s not raised
called %(func)r
with args %(args)r
and kwargs %(kwargs)r
""" % locals ())


    # -- Assertions used in more than one class --------------------

    def assertParseFail(self, cmdline_args, expected_output):
        """
        Assert the parser fails with the expected message.  Caller
        must ensure that self.parser is an InterceptingOptionParser.
        """
        try:
            self.parser.parse_args(cmdline_args)
        except InterceptedError as err:
            self.assertEqual(err.error_message, expected_output)
        else:
            self.assertFalse("expected parse failure")

    def assertOutput(self,
                     cmdline_args,
                     expected_output,
                     expected_status=0,
                     expected_error=None):
        """Assert the parser prints the expected output on stdout."""
        save_stdout = sys.stdout
        try:
            try:
                sys.stdout = StringIO()
                self.parser.parse_args(cmdline_args)
            finally:
                output = sys.stdout.getvalue()
                sys.stdout = save_stdout

        except InterceptedError as err:
            self.assertTrue(
                isinstance(output, str),
                "expected output to be an ordinary string, not %r"
                % type(output))

            if output != expected_output:
                self.fail("expected: \n'''\n" + expected_output +
                          "'''\nbut got \n'''\n" + output + "'''")
            self.assertEqual(err.exit_status, expected_status)
            self.assertEqual(err.exit_message, expected_error)
        else:
            self.assertFalse("expected parser.exit()")

    def assertTypeError(self, func, expected_message, *args):
        """Assert that TypeError is raised when executing func."""
        self.assertRaises(func, args, None, TypeError, expected_message)

    def assertHelp(self, parser, expected_help):
        actual_help = parser.format_help()
        if actual_help != expected_help:
            raise self.failureException(
                'help text failure; expected:\n"' +
                expected_help + '"; got:\n"' +
                actual_help + '"\n')

# -- Test make_option() aka Option -------------------------------------

# It's not necessary to test correct options here.  All the tests in the
# parser.parse_args() section deal with those, because they're needed
# there.

class TestOptionChecks(BaseTest):
    def setUp(self):
        self.parser = OptionParser(usage=SUPPRESS_USAGE)

    def assertOptionError(self, expected_message, args=[], kwargs={}):
        self.assertRaises(make_option, args, kwargs,
                          OptionError, expected_message)

    def test_opt_string_empty(self):
        self.assertTypeError(make_option,
                             "at least one option string must be supplied")

    def test_opt_string_too_short(self):
        self.assertOptionError(
            "invalid option string 'b': must be at least two characters long",
            ["b"])

    def test_opt_string_short_invalid(self):
        self.assertOptionError(
            "invalid short option string '--': must be "
            "of the form -x, (x any non-dash char)",
            ["--"])

    def test_opt_string_long_invalid(self):
        self.assertOptionError(
            "invalid long option string '---': "
            "must start with --, followed by non-dash",
            ["---"])

    def test_attr_invalid(self):
        self.assertOptionError(
            "option -b: invalid keyword arguments: bar, foo",
            ["-b"], {'foo': None, 'bar': None})

    def test_action_invalid(self):
        self.assertOptionError(
            "option -b: invalid action: 'foo'",
            ["-b"], {'action': 'foo'})

    def test_type_invalid(self):
        self.assertOptionError(
            "option -b: invalid option type: 'foo'",
            ["-b"], {'type': 'foo'})
        self.assertOptionError(
            "option -b: invalid option type: 'tuple'",
            ["-b"], {'type': tuple})

    def test_no_type_for_action(self):
        self.assertOptionError(
            "option -b: must not supply a type for action 'count'",
            ["-b"], {'action': 'count', 'type': 'int'})

    def test_no_choices_list(self):
        self.assertOptionError(
            "option -b/--bad: must supply a list of "
            "choices for type 'choice'",
            ["-b", "--bad"], {'type': "choice"})

    def test_bad_choices_list(self):
        typename = type('').__name__
        self.assertOptionError(
            "option -b/--bad: choices must be a list of "
            "strings ('%s' supplied)" % typename,
            ["-b", "--bad"],
            {'type': "choice", 'choices':"bad choices"})

    def test_no_choices_for_type(self):
        self.assertOptionError(
            "option -b: must not supply choices for type 'int'",
            ["-b"], {'type': 'int', 'choices':"bad"})

    def test_no_const_for_action(self):
        self.assertOptionError(
            "option -b: 'const' must not be supplied for action 'store'",
            ["-b"], {'action': 'store', 'const': 1})

    def test_no_nargs_for_action(self):
        self.assertOptionError(
            "option -b: 'nargs' must not be supplied for action 'count'",
            ["-b"], {'action': 'count', 'nargs': 2})

    def test_callback_not_callable(self):
        self.assertOptionError(
            "option -b: callback not callable: 'foo'",
            ["-b"], {'action': 'callback',
                     'callback': 'foo'})

    def dummy(self):
        pass

    def test_callback_args_no_tuple(self):
        self.assertOptionError(
            "option -b: callback_args, if supplied, "
            "must be a tuple: not 'foo'",
            ["-b"], {'action': 'callback',
                     'callback': self.dummy,
                     'callback_args': 'foo'})

    def test_callback_kwargs_no_dict(self):
        self.assertOptionError(
            "option -b: callback_kwargs, if supplied, "
            "must be a dict: not 'foo'",
            ["-b"], {'action': 'callback',
                     'callback': self.dummy,
                     'callback_kwargs': 'foo'})

    def test_no_callback_for_action(self):
        self.assertOptionError(
            "option -b: callback supplied ('foo') for non-callback option",
            ["-b"], {'action': 'store',
                     'callback': 'foo'})

    def test_no_callback_args_for_action(self):
        self.assertOptionError(
            "option -b: callback_args supplied for non-callback option",
            ["-b"], {'action': 'store',
                     'callback_args': 'foo'})

    def test_no_callback_kwargs_for_action(self):
        self.assertOptionError(
            "option -b: callback_kwargs supplied for non-callback option",
            ["-b"], {'action': 'store',
                     'callback_kwargs': 'foo'})

    def test_no_single_dash(self):
        self.assertOptionError(
            "invalid long option string '-debug': "
            "must start with --, followed by non-dash",
            ["-debug"])

        self.assertOptionError(
            "option -d: invalid long option string '-debug': must start with"
            " --, followed by non-dash",
            ["-d", "-debug"])

        self.assertOptionError(
            "invalid long option string '-debug': "
            "must start with --, followed by non-dash",
            ["-debug", "--debug"])

class TestOptionParser(BaseTest):
    def setUp(self):
        self.parser = OptionParser()
        self.parser.add_option("-v", "--verbose", "-n", "--noisy",
                          action="store_true", dest="verbose")
        self.parser.add_option("-q", "--quiet", "--silent",
                          action="store_false", dest="verbose")

    def test_add_option_no_Option(self):
        self.assertTypeError(self.parser.add_option,
                             "not an Option instance: None", None)

    def test_add_option_invalid_arguments(self):
        self.assertTypeError(self.parser.add_option,
                             "invalid arguments", None, None)

    def test_get_option(self):
        opt1 = self.parser.get_option("-v")
        self.assertIsInstance(opt1, Option)
        self.assertEqual(opt1._short_opts, ["-v", "-n"])
        self.assertEqual(opt1._long_opts, ["--verbose", "--noisy"])
        self.assertEqual(opt1.action, "store_true")
        self.assertEqual(opt1.dest, "verbose")

    def test_get_option_equals(self):
        opt1 = self.parser.get_option("-v")
        opt2 = self.parser.get_option("--verbose")
        opt3 = self.parser.get_option("-n")
        opt4 = self.parser.get_option("--noisy")
        self.assertTrue(opt1 is opt2 is opt3 is opt4)

    def test_has_option(self):
        self.assertTrue(self.parser.has_option("-v"))
        self.assertTrue(self.parser.has_option("--verbose"))

    def assertTrueremoved(self):
        self.assertTrue(self.parser.get_option("-v") is None)
        self.assertTrue(self.parser.get_option("--verbose") is None)
        self.assertTrue(self.parser.get_option("-n") is None)
        self.assertTrue(self.parser.get_option("--noisy") is None)

        self.assertFalse(self.parser.has_option("-v"))
        self.assertFalse(self.parser.has_option("--verbose"))
        self.assertFalse(self.parser.has_option("-n"))
        self.assertFalse(self.parser.has_option("--noisy"))

        self.assertTrue(self.parser.has_option("-q"))
        self.assertTrue(self.parser.has_option("--silent"))

    def test_remove_short_opt(self):
        self.parser.remove_option("-n")
        self.assertTrueremoved()

    def test_remove_long_opt(self):
        self.parser.remove_option("--verbose")
        self.assertTrueremoved()

    def test_remove_nonexistent(self):
        self.assertRaises(self.parser.remove_option, ('foo',), None,
                          ValueError, "no such option 'foo'")

    def test_refleak(self):
        # If an OptionParser is carrying around a reference to a large
        # object, various cycles can prevent it from being GC'd in
        # a timely fashion.  destroy() breaks the cycles to ensure stuff
        # can be cleaned up.
        big_thing = [42]
        refcount = sys.getrefcount(big_thing)
        parser = OptionParser()
        parser.add_option("-a", "--aaarggh")
        parser.big_thing = big_thing

        parser.destroy()
        #self.assertEqual(refcount, sys.getrefcount(big_thing))
        del parser
        self.assertEqual(refcount, sys.getrefcount(big_thing))


class TestOptionValues(BaseTest):
    def setUp(self):
        pass

    def test_basics(self):
        values = Values()
        self.assertEqual(vars(values), {})
        self.assertEqual(values, {})
        self.assertNotEqual(values, {"foo": "bar"})
        self.assertNotEqual(values, "")

        dict = {"foo": "bar", "baz": 42}
        values = Values(defaults=dict)
        self.assertEqual(vars(values), dict)
        self.assertEqual(values, dict)
        self.assertNotEqual(values, {"foo": "bar"})
        self.assertNotEqual(values, {})
        self.assertNotEqual(values, "")
        self.assertNotEqual(values, [])


class TestTypeAliases(BaseTest):
    def setUp(self):
        self.parser = OptionParser()

    def test_str_aliases_string(self):
        self.parser.add_option("-s", type="str")
        self.assertEqual(self.parser.get_option("-s").type, "string")

    def test_type_object(self):
        self.parser.add_option("-s", type=str)
        self.assertEqual(self.parser.get_option("-s").type, "string")
        self.parser.add_option("-x", type=int)
        self.assertEqual(self.parser.get_option("-x").type, "int")


# Custom type for testing processing of default values.
_time_units = { 's' : 1, 'm' : 60, 'h' : 60*60, 'd' : 60*60*24 }

def _check_duration(option, opt, value):
    try:
        if value[-1].isdigit():
            return int(value)
        else:
            return int(value[:-1]) * _time_units[value[-1]]
    except (ValueError, IndexError):
        raise OptionValueError(
            'option %s: invalid duration: %r' % (opt, value))

class DurationOption(Option):
    TYPES = Option.TYPES + ('duration',)
    TYPE_CHECKER = copy.copy(Option.TYPE_CHECKER)
    TYPE_CHECKER['duration'] = _check_duration

class TestDefaultValues(BaseTest):
    def setUp(self):
        self.parser = OptionParser()
        self.parser.add_option("-v", "--verbose", default=True)
        self.parser.add_option("-q", "--quiet", dest='verbose')
        self.parser.add_option("-n", type="int", default=37)
        self.parser.add_option("-m", type="int")
        self.parser.add_option("-s", default="foo")
        self.parser.add_option("-t")
        self.parser.add_option("-u", default=None)
        self.expected = { 'verbose': True,
                          'n': 37,
                          'm': None,
                          's': "foo",
                          't': None,
                          'u': None }

    def test_basic_defaults(self):
        self.assertEqual(self.parser.get_default_values(), self.expected)

    def test_mixed_defaults_post(self):
        self.parser.set_defaults(n=42, m=-100)
        self.expected.update({'n': 42, 'm': -100})
        self.assertEqual(self.parser.get_default_values(), self.expected)

    def test_mixed_defaults_pre(self):
        self.parser.set_defaults(x="barf", y="blah")
        self.parser.add_option("-x", default="frob")
        self.parser.add_option("-y")

        self.expected.update({'x': "frob", 'y': "blah"})
        self.assertEqual(self.parser.get_default_values(), self.expected)

        self.parser.remove_option("-y")
        self.parser.add_option("-y", default=None)
        self.expected.update({'y': None})
        self.assertEqual(self.parser.get_default_values(), self.expected)

    def test_process_default(self):
        self.parser.option_class = DurationOption
        self.parser.add_option("-d", type="duration", default=300)
        self.parser.add_option("-e", type="duration", default="6m")
        self.parser.set_defaults(n="42")
        self.expected.update({'d': 300, 'e': 360, 'n': 42})
        self.assertEqual(self.parser.get_default_values(), self.expected)

        self.parser.set_process_default_values(False)
        self.expected.update({'d': 300, 'e': "6m", 'n': "42"})
        self.assertEqual(self.parser.get_default_values(), self.expected)


class TestProgName(BaseTest):
    """
    Test that %prog expands to the right thing in usage, version,
    and help strings.
    """

    def assertUsage(self, parser, expected_usage):
        self.assertEqual(parser.get_usage(), expected_usage)

    def assertVersion(self, parser, expected_version):
        self.assertEqual(parser.get_version(), expected_version)


    def test_default_progname(self):
        # Make sure that program name taken from sys.argv[0] by default.
        save_argv = sys.argv[:]
        try:
            sys.argv[0] = os.path.join("foo", "bar", "baz.py")
            parser = OptionParser("%prog ...", version="%prog 1.2")
            expected_usage = "Usage: baz.py ...\n"
            self.assertUsage(parser, expected_usage)
            self.assertVersion(parser, "baz.py 1.2")
            self.assertHelp(parser,
                            expected_usage + "\n" +
                            "Options:\n"
                            "  --version   show program's version number and exit\n"
                            "  -h, --help  show this help message and exit\n")
        finally:
            sys.argv[:] = save_argv

    def test_custom_progname(self):
        parser = OptionParser(prog="thingy",
                              version="%prog 0.1",
                              usage="%prog arg arg")
        parser.remove_option("-h")
        parser.remove_option("--version")
        expected_usage = "Usage: thingy arg arg\n"
        self.assertUsage(parser, expected_usage)
        self.assertVersion(parser, "thingy 0.1")
        self.assertHelp(parser, expected_usage + "\n")


class TestExpandDefaults(BaseTest):
    def setUp(self):
        self.parser = OptionParser(prog="test")
        self.help_prefix = """\
Usage: test [options]

Options:
  -h, --help            show this help message and exit
"""
        self.file_help = "read from FILE [default: %default]"
        self.expected_help_file = self.help_prefix + \
            "  -f FILE, --file=FILE  read from FILE [default: foo.txt]\n"
        self.expected_help_none = self.help_prefix + \
            "  -f FILE, --file=FILE  read from FILE [default: none]\n"

    def test_option_default(self):
        self.parser.add_option("-f", "--file",
                               default="foo.txt",
                               help=self.file_help)
        self.assertHelp(self.parser, self.expected_help_file)

    def test_parser_default_1(self):
        self.parser.add_option("-f", "--file",
                               help=self.file_help)
        self.parser.set_default('file', "foo.txt")
        self.assertHelp(self.parser, self.expected_help_file)

    def test_parser_default_2(self):
        self.parser.add_option("-f", "--file",
                               help=self.file_help)
        self.parser.set_defaults(file="foo.txt")
        self.assertHelp(self.parser, self.expected_help_file)

    def test_no_default(self):
        self.parser.add_option("-f", "--file",
                               help=self.file_help)
        self.assertHelp(self.parser, self.expected_help_none)

    def test_default_none_1(self):
        self.parser.add_option("-f", "--file",
                               default=None,
                               help=self.file_help)
        self.assertHelp(self.parser, self.expected_help_none)

    def test_default_none_2(self):
        self.parser.add_option("-f", "--file",
                               help=self.file_help)
        self.parser.set_defaults(file=None)
        self.assertHelp(self.parser, self.expected_help_none)

    def test_float_default(self):
        self.parser.add_option(
            "-p", "--prob",
            help="blow up with probability PROB [default: %default]")
        self.parser.set_defaults(prob=0.43)
        expected_help = self.help_prefix + \
            "  -p PROB, --prob=PROB  blow up with probability PROB [default: 0.43]\n"
        self.assertHelp(self.parser, expected_help)

    def test_alt_expand(self):
        self.parser.add_option("-f", "--file",
                               default="foo.txt",
                               help="read from FILE [default: *DEFAULT*]")
        self.parser.formatter.default_tag = "*DEFAULT*"
        self.assertHelp(self.parser, self.expected_help_file)

    def test_no_expand(self):
        self.parser.add_option("-f", "--file",
                               default="foo.txt",
                               help="read from %default file")
        self.parser.formatter.default_tag = None
        expected_help = self.help_prefix + \
            "  -f FILE, --file=FILE  read from %default file\n"
        self.assertHelp(self.parser, expected_help)


# -- Test parser.parse_args() ------------------------------------------

class TestStandard(BaseTest):
    def setUp(self):
        options = [make_option("-a", type="string"),
                   make_option("-b", "--boo", type="int", dest='boo'),
                   make_option("--foo", action="append")]

        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               option_list=options)

    def test_required_value(self):
        self.assertParseFail(["-a"], "-a option requires 1 argument")

    def test_invalid_integer(self):
        self.assertParseFail(["-b", "5x"],
                             "option -b: invalid integer value: '5x'")

    def test_no_such_option(self):
        self.assertParseFail(["--boo13"], "no such option: --boo13")

    def test_long_invalid_integer(self):
        self.assertParseFail(["--boo=x5"],
                             "option --boo: invalid integer value: 'x5'")

    def test_empty(self):
        self.assertParseOK([], {'a': None, 'boo': None, 'foo': None}, [])

    def test_shortopt_empty_longopt_append(self):
        self.assertParseOK(["-a", "", "--foo=blah", "--foo="],
                           {'a': "", 'boo': None, 'foo': ["blah", ""]},
                           [])

    def test_long_option_append(self):
        self.assertParseOK(["--foo", "bar", "--foo", "", "--foo=x"],
                           {'a': None,
                            'boo': None,
                            'foo': ["bar", "", "x"]},
                           [])

    def test_option_argument_joined(self):
        self.assertParseOK(["-abc"],
                           {'a': "bc", 'boo': None, 'foo': None},
                           [])

    def test_option_argument_split(self):
        self.assertParseOK(["-a", "34"],
                           {'a': "34", 'boo': None, 'foo': None},
                           [])

    def test_option_argument_joined_integer(self):
        self.assertParseOK(["-b34"],
                           {'a': None, 'boo': 34, 'foo': None},
                           [])

    def test_option_argument_split_negative_integer(self):
        self.assertParseOK(["-b", "-5"],
                           {'a': None, 'boo': -5, 'foo': None},
                           [])

    def test_long_option_argument_joined(self):
        self.assertParseOK(["--boo=13"],
                           {'a': None, 'boo': 13, 'foo': None},
                           [])

    def test_long_option_argument_split(self):
        self.assertParseOK(["--boo", "111"],
                           {'a': None, 'boo': 111, 'foo': None},
                           [])

    def test_long_option_short_option(self):
        self.assertParseOK(["--foo=bar", "-axyz"],
                           {'a': 'xyz', 'boo': None, 'foo': ["bar"]},
                           [])

    def test_abbrev_long_option(self):
        self.assertParseOK(["--f=bar", "-axyz"],
                           {'a': 'xyz', 'boo': None, 'foo': ["bar"]},
                           [])

    def test_defaults(self):
        (options, args) = self.parser.parse_args([])
        defaults = self.parser.get_default_values()
        self.assertEqual(vars(defaults), vars(options))

    def test_ambiguous_option(self):
        self.parser.add_option("--foz", action="store",
                               type="string", dest="foo")
        self.assertParseFail(["--f=bar"],
                             "ambiguous option: --f (--foo, --foz?)")


    def test_short_and_long_option_split(self):
        self.assertParseOK(["-a", "xyz", "--foo", "bar"],
                           {'a': 'xyz', 'boo': None, 'foo': ["bar"]},
                           [])

    def test_short_option_split_long_option_append(self):
        self.assertParseOK(["--foo=bar", "-b", "123", "--foo", "baz"],
                           {'a': None, 'boo': 123, 'foo': ["bar", "baz"]},
                           [])

    def test_short_option_split_one_positional_arg(self):
        self.assertParseOK(["-a", "foo", "bar"],
                           {'a': "foo", 'boo': None, 'foo': None},
                           ["bar"])

    def test_short_option_consumes_separator(self):
        self.assertParseOK(["-a", "--", "foo", "bar"],
                           {'a': "--", 'boo': None, 'foo': None},
                           ["foo", "bar"])
        self.assertParseOK(["-a", "--", "--foo", "bar"],
                           {'a': "--", 'boo': None, 'foo': ["bar"]},
                           [])

    def test_short_option_joined_and_separator(self):
        self.assertParseOK(["-ab", "--", "--foo", "bar"],
                           {'a': "b", 'boo': None, 'foo': None},
                           ["--foo", "bar"]),

    def test_hyphen_becomes_positional_arg(self):
        self.assertParseOK(["-ab", "-", "--foo", "bar"],
                           {'a': "b", 'boo': None, 'foo': ["bar"]},
                           ["-"])

    def test_no_append_versus_append(self):
        self.assertParseOK(["-b3", "-b", "5", "--foo=bar", "--foo", "baz"],
                           {'a': None, 'boo': 5, 'foo': ["bar", "baz"]},
                           [])

    def test_option_consumes_optionlike_string(self):
        self.assertParseOK(["-a", "-b3"],
                           {'a': "-b3", 'boo': None, 'foo': None},
                           [])

    def test_combined_single_invalid_option(self):
        self.parser.add_option("-t", action="store_true")
        self.assertParseFail(["-test"],
                             "no such option: -e")

class TestBool(BaseTest):
    def setUp(self):
        options = [make_option("-v",
                               "--verbose",
                               action="store_true",
                               dest="verbose",
                               default=''),
                   make_option("-q",
                               "--quiet",
                               action="store_false",
                               dest="verbose")]
        self.parser = OptionParser(option_list = options)

    def test_bool_default(self):
        self.assertParseOK([],
                           {'verbose': ''},
                           [])

    def test_bool_false(self):
        (options, args) = self.assertParseOK(["-q"],
                                             {'verbose': 0},
                                             [])
        self.assertTrue(options.verbose is False)

    def test_bool_true(self):
        (options, args) = self.assertParseOK(["-v"],
                                             {'verbose': 1},
                                             [])
        self.assertTrue(options.verbose is True)

    def test_bool_flicker_on_and_off(self):
        self.assertParseOK(["-qvq", "-q", "-v"],
                           {'verbose': 1},
                           [])

class TestChoice(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.parser.add_option("-c", action="store", type="choice",
                               dest="choice", choices=["one", "two", "three"])

    def test_valid_choice(self):
        self.assertParseOK(["-c", "one", "xyz"],
                           {'choice': 'one'},
                           ["xyz"])

    def test_invalid_choice(self):
        self.assertParseFail(["-c", "four", "abc"],
                             "option -c: invalid choice: 'four' "
                             "(choose from 'one', 'two', 'three')")

    def test_add_choice_option(self):
        self.parser.add_option("-d", "--default",
                               choices=["four", "five", "six"])
        opt = self.parser.get_option("-d")
        self.assertEqual(opt.type, "choice")
        self.assertEqual(opt.action, "store")

class TestCount(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.v_opt = make_option("-v", action="count", dest="verbose")
        self.parser.add_option(self.v_opt)
        self.parser.add_option("--verbose", type="int", dest="verbose")
        self.parser.add_option("-q", "--quiet",
                               action="store_const", dest="verbose", const=0)

    def test_empty(self):
        self.assertParseOK([], {'verbose': None}, [])

    def test_count_one(self):
        self.assertParseOK(["-v"], {'verbose': 1}, [])

    def test_count_three(self):
        self.assertParseOK(["-vvv"], {'verbose': 3}, [])

    def test_count_three_apart(self):
        self.assertParseOK(["-v", "-v", "-v"], {'verbose': 3}, [])

    def test_count_override_amount(self):
        self.assertParseOK(["-vvv", "--verbose=2"], {'verbose': 2}, [])

    def test_count_override_quiet(self):
        self.assertParseOK(["-vvv", "--verbose=2", "-q"], {'verbose': 0}, [])

    def test_count_overriding(self):
        self.assertParseOK(["-vvv", "--verbose=2", "-q", "-v"],
                           {'verbose': 1}, [])

    def test_count_interspersed_args(self):
        self.assertParseOK(["--quiet", "3", "-v"],
                           {'verbose': 1},
                           ["3"])

    def test_count_no_interspersed_args(self):
        self.parser.disable_interspersed_args()
        self.assertParseOK(["--quiet", "3", "-v"],
                           {'verbose': 0},
                           ["3", "-v"])

    def test_count_no_such_option(self):
        self.assertParseFail(["-q3", "-v"], "no such option: -3")

    def test_count_option_no_value(self):
        self.assertParseFail(["--quiet=3", "-v"],
                             "--quiet option does not take a value")

    def test_count_with_default(self):
        self.parser.set_default('verbose', 0)
        self.assertParseOK([], {'verbose':0}, [])

    def test_count_overriding_default(self):
        self.parser.set_default('verbose', 0)
        self.assertParseOK(["-vvv", "--verbose=2", "-q", "-v"],
                           {'verbose': 1}, [])

class TestMultipleArgs(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.parser.add_option("-p", "--point",
                               action="store", nargs=3, type="float", dest="point")

    def test_nargs_with_positional_args(self):
        self.assertParseOK(["foo", "-p", "1", "2.5", "-4.3", "xyz"],
                           {'point': (1.0, 2.5, -4.3)},
                           ["foo", "xyz"])

    def test_nargs_long_opt(self):
        self.assertParseOK(["--point", "-1", "2.5", "-0", "xyz"],
                           {'point': (-1.0, 2.5, -0.0)},
                           ["xyz"])

    def test_nargs_invalid_float_value(self):
        self.assertParseFail(["-p", "1.0", "2x", "3.5"],
                             "option -p: "
                             "invalid floating-point value: '2x'")

    def test_nargs_required_values(self):
        self.assertParseFail(["--point", "1.0", "3.5"],
                             "--point option requires 3 arguments")

class TestMultipleArgsAppend(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.parser.add_option("-p", "--point", action="store", nargs=3,
                               type="float", dest="point")
        self.parser.add_option("-f", "--foo", action="append", nargs=2,
                               type="int", dest="foo")
        self.parser.add_option("-z", "--zero", action="append_const",
                               dest="foo", const=(0, 0))

    def test_nargs_append(self):
        self.assertParseOK(["-f", "4", "-3", "blah", "--foo", "1", "666"],
                           {'point': None, 'foo': [(4, -3), (1, 666)]},
                           ["blah"])

    def test_nargs_append_required_values(self):
        self.assertParseFail(["-f4,3"],
                             "-f option requires 2 arguments")

    def test_nargs_append_simple(self):
        self.assertParseOK(["--foo=3", "4"],
                           {'point': None, 'foo':[(3, 4)]},
                           [])

    def test_nargs_append_const(self):
        self.assertParseOK(["--zero", "--foo", "3", "4", "-z"],
                           {'point': None, 'foo':[(0, 0), (3, 4), (0, 0)]},
                           [])

class TestVersion(BaseTest):
    def test_version(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               version="%prog 0.1")
        save_argv = sys.argv[:]
        try:
            sys.argv[0] = os.path.join(os.curdir, "foo", "bar")
            self.assertOutput(["--version"], "bar 0.1\n")
        finally:
            sys.argv[:] = save_argv

    def test_no_version(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.assertParseFail(["--version"],
                             "no such option: --version")

# -- Test conflicting default values and parser.parse_args() -----------

class TestConflictingDefaults(BaseTest):
    """Conflicting default values: the last one should win."""
    def setUp(self):
        self.parser = OptionParser(option_list=[
            make_option("-v", action="store_true", dest="verbose", default=1)])

    def test_conflict_default(self):
        self.parser.add_option("-q", action="store_false", dest="verbose",
                               default=0)
        self.assertParseOK([], {'verbose': 0}, [])

    def test_conflict_default_none(self):
        self.parser.add_option("-q", action="store_false", dest="verbose",
                               default=None)
        self.assertParseOK([], {'verbose': None}, [])

class TestOptionGroup(BaseTest):
    def setUp(self):
        self.parser = OptionParser(usage=SUPPRESS_USAGE)

    def test_option_group_create_instance(self):
        group = OptionGroup(self.parser, "Spam")
        self.parser.add_option_group(group)
        group.add_option("--spam", action="store_true",
                         help="spam spam spam spam")
        self.assertParseOK(["--spam"], {'spam': 1}, [])

    def test_add_group_no_group(self):
        self.assertTypeError(self.parser.add_option_group,
                             "not an OptionGroup instance: None", None)

    def test_add_group_invalid_arguments(self):
        self.assertTypeError(self.parser.add_option_group,
                             "invalid arguments", None, None)

    def test_add_group_wrong_parser(self):
        group = OptionGroup(self.parser, "Spam")
        group.parser = OptionParser()
        self.assertRaises(self.parser.add_option_group, (group,), None,
                          ValueError, "invalid OptionGroup (wrong parser)")

    def test_group_manipulate(self):
        group = self.parser.add_option_group("Group 2",
                                             description="Some more options")
        group.set_title("Bacon")
        group.add_option("--bacon", type="int")
        self.assertTrue(self.parser.get_option_group("--bacon"), group)

# -- Test extending and parser.parse_args() ----------------------------

class TestExtendAddTypes(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               option_class=self.MyOption)
        self.parser.add_option("-a", None, type="string", dest="a")
        self.parser.add_option("-f", "--file", type="file", dest="file")

    def tearDown(self):
        if os.path.isdir(support.TESTFN):
            os.rmdir(support.TESTFN)
        elif os.path.isfile(support.TESTFN):
            os.unlink(support.TESTFN)

    class MyOption (Option):
        def check_file(option, opt, value):
            if not os.path.exists(value):
                raise OptionValueError("%s: file does not exist" % value)
            elif not os.path.isfile(value):
                raise OptionValueError("%s: not a regular file" % value)
            return value

        TYPES = Option.TYPES + ("file",)
        TYPE_CHECKER = copy.copy(Option.TYPE_CHECKER)
        TYPE_CHECKER["file"] = check_file

    def test_filetype_ok(self):
        support.create_empty_file(support.TESTFN)
        self.assertParseOK(["--file", support.TESTFN, "-afoo"],
                           {'file': support.TESTFN, 'a': 'foo'},
                           [])

    def test_filetype_noexist(self):
        self.assertParseFail(["--file", support.TESTFN, "-afoo"],
                             "%s: file does not exist" %
                             support.TESTFN)

    def test_filetype_notfile(self):
        os.mkdir(support.TESTFN)
        self.assertParseFail(["--file", support.TESTFN, "-afoo"],
                             "%s: not a regular file" %
                             support.TESTFN)


class TestExtendAddActions(BaseTest):
    def setUp(self):
        options = [self.MyOption("-a", "--apple", action="extend",
                                 type="string", dest="apple")]
        self.parser = OptionParser(option_list=options)

    class MyOption (Option):
        ACTIONS = Option.ACTIONS + ("extend",)
        STORE_ACTIONS = Option.STORE_ACTIONS + ("extend",)
        TYPED_ACTIONS = Option.TYPED_ACTIONS + ("extend",)

        def take_action(self, action, dest, opt, value, values, parser):
            if action == "extend":
                lvalue = value.split(",")
                values.ensure_value(dest, []).extend(lvalue)
            else:
                Option.take_action(self, action, dest, opt, parser, value,
                                   values)

    def test_extend_add_action(self):
        self.assertParseOK(["-afoo,bar", "--apple=blah"],
                           {'apple': ["foo", "bar", "blah"]},
                           [])

    def test_extend_add_action_normal(self):
        self.assertParseOK(["-a", "foo", "-abar", "--apple=x,y"],
                           {'apple': ["foo", "bar", "x", "y"]},
                           [])

# -- Test callbacks and parser.parse_args() ----------------------------

class TestCallback(BaseTest):
    def setUp(self):
        options = [make_option("-x",
                               None,
                               action="callback",
                               callback=self.process_opt),
                   make_option("-f",
                               "--file",
                               action="callback",
                               callback=self.process_opt,
                               type="string",
                               dest="filename")]
        self.parser = OptionParser(option_list=options)

    def process_opt(self, option, opt, value, parser_):
        if opt == "-x":
            self.assertEqual(option._short_opts, ["-x"])
            self.assertEqual(option._long_opts, [])
            self.assertTrue(parser_ is self.parser)
            self.assertTrue(value is None)
            self.assertEqual(vars(parser_.values), {'filename': None})

            parser_.values.x = 42
        elif opt == "--file":
            self.assertEqual(option._short_opts, ["-f"])
            self.assertEqual(option._long_opts, ["--file"])
            self.assertTrue(parser_ is self.parser)
            self.assertEqual(value, "foo")
            self.assertEqual(vars(parser_.values), {'filename': None, 'x': 42})

            setattr(parser_.values, option.dest, value)
        else:
            self.fail("Unknown option %r in process_opt." % opt)

    def test_callback(self):
        self.assertParseOK(["-x", "--file=foo"],
                           {'filename': "foo", 'x': 42},
                           [])

    def test_callback_help(self):
        # This test was prompted by SF bug #960515 -- the point is
        # not to inspect the help text, just to make sure that
        # format_help() doesn't crash.
        parser = OptionParser(usage=SUPPRESS_USAGE)
        parser.remove_option("-h")
        parser.add_option("-t", "--test", action="callback",
                          callback=lambda: None, type="string",
                          help="foo")

        expected_help = ("Options:\n"
                         "  -t TEST, --test=TEST  foo\n")
        self.assertHelp(parser, expected_help)


class TestCallbackExtraArgs(BaseTest):
    def setUp(self):
        options = [make_option("-p", "--point", action="callback",
                               callback=self.process_tuple,
                               callback_args=(3, int), type="string",
                               dest="points", default=[])]
        self.parser = OptionParser(option_list=options)

    def process_tuple(self, option, opt, value, parser_, len, type):
        self.assertEqual(len, 3)
        self.assertTrue(type is int)

        if opt == "-p":
            self.assertEqual(value, "1,2,3")
        elif opt == "--point":
            self.assertEqual(value, "4,5,6")

        value = tuple(map(type, value.split(",")))
        getattr(parser_.values, option.dest).append(value)

    def test_callback_extra_args(self):
        self.assertParseOK(["-p1,2,3", "--point", "4,5,6"],
                           {'points': [(1,2,3), (4,5,6)]},
                           [])

class TestCallbackMeddleArgs(BaseTest):
    def setUp(self):
        options = [make_option(str(x), action="callback",
                               callback=self.process_n, dest='things')
                   for x in range(-1, -6, -1)]
        self.parser = OptionParser(option_list=options)

    # Callback that meddles in rargs, largs
    def process_n(self, option, opt, value, parser_):
        # option is -3, -5, etc.
        nargs = int(opt[1:])
        rargs = parser_.rargs
        if len(rargs) < nargs:
            self.fail("Expected %d arguments for %s option." % (nargs, opt))
        dest = parser_.values.ensure_value(option.dest, [])
        dest.append(tuple(rargs[0:nargs]))
        parser_.largs.append(nargs)
        del rargs[0:nargs]

    def test_callback_meddle_args(self):
        self.assertParseOK(["-1", "foo", "-3", "bar", "baz", "qux"],
                           {'things': [("foo",), ("bar", "baz", "qux")]},
                           [1, 3])

    def test_callback_meddle_args_separator(self):
        self.assertParseOK(["-2", "foo", "--"],
                           {'things': [('foo', '--')]},
                           [2])

class TestCallbackManyArgs(BaseTest):
    def setUp(self):
        options = [make_option("-a", "--apple", action="callback", nargs=2,
                               callback=self.process_many, type="string"),
                   make_option("-b", "--bob", action="callback", nargs=3,
                               callback=self.process_many, type="int")]
        self.parser = OptionParser(option_list=options)

    def process_many(self, option, opt, value, parser_):
        if opt == "-a":
            self.assertEqual(value, ("foo", "bar"))
        elif opt == "--apple":
            self.assertEqual(value, ("ding", "dong"))
        elif opt == "-b":
            self.assertEqual(value, (1, 2, 3))
        elif opt == "--bob":
            self.assertEqual(value, (-666, 42, 0))

    def test_many_args(self):
        self.assertParseOK(["-a", "foo", "bar", "--apple", "ding", "dong",
                            "-b", "1", "2", "3", "--bob", "-666", "42",
                            "0"],
                           {"apple": None, "bob": None},
                           [])

class TestCallbackCheckAbbrev(BaseTest):
    def setUp(self):
        self.parser = OptionParser()
        self.parser.add_option("--foo-bar", action="callback",
                               callback=self.check_abbrev)

    def check_abbrev(self, option, opt, value, parser):
        self.assertEqual(opt, "--foo-bar")

    def test_abbrev_callback_expansion(self):
        self.assertParseOK(["--foo"], {}, [])

class TestCallbackVarArgs(BaseTest):
    def setUp(self):
        options = [make_option("-a", type="int", nargs=2, dest="a"),
                   make_option("-b", action="store_true", dest="b"),
                   make_option("-c", "--callback", action="callback",
                               callback=self.variable_args, dest="c")]
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               option_list=options)

    def variable_args(self, option, opt, value, parser):
        self.assertTrue(value is None)
        value = []
        rargs = parser.rargs
        while rargs:
            arg = rargs[0]
            if ((arg[:2] == "--" and len(arg) > 2) or
                (arg[:1] == "-" and len(arg) > 1 and arg[1] != "-")):
                break
            else:
                value.append(arg)
                del rargs[0]
        setattr(parser.values, option.dest, value)

    def test_variable_args(self):
        self.assertParseOK(["-a3", "-5", "--callback", "foo", "bar"],
                           {'a': (3, -5), 'b': None, 'c': ["foo", "bar"]},
                           [])

    def test_consume_separator_stop_at_option(self):
        self.assertParseOK(["-c", "37", "--", "xxx", "-b", "hello"],
                           {'a': None,
                            'b': True,
                            'c': ["37", "--", "xxx"]},
                           ["hello"])

    def test_positional_arg_and_variable_args(self):
        self.assertParseOK(["hello", "-c", "foo", "-", "bar"],
                           {'a': None,
                            'b': None,
                            'c':["foo", "-", "bar"]},
                           ["hello"])

    def test_stop_at_option(self):
        self.assertParseOK(["-c", "foo", "-b"],
                           {'a': None, 'b': True, 'c': ["foo"]},
                           [])

    def test_stop_at_invalid_option(self):
        self.assertParseFail(["-c", "3", "-5", "-a"], "no such option: -5")


# -- Test conflict handling and parser.parse_args() --------------------

class ConflictBase(BaseTest):
    def setUp(self):
        options = [make_option("-v", "--verbose", action="count",
                               dest="verbose", help="increment verbosity")]
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               option_list=options)

    def show_version(self, option, opt, value, parser):
        parser.values.show_version = 1

class TestConflict(ConflictBase):
    """Use the default conflict resolution for Optik 1.2: error."""
    def assertTrueconflict_error(self, func):
        err = self.assertRaises(
            func, ("-v", "--version"), {'action' : "callback",
                                        'callback' : self.show_version,
                                        'help' : "show version"},
            OptionConflictError,
            "option -v/--version: conflicting option string(s): -v")

        self.assertEqual(err.msg, "conflicting option string(s): -v")
        self.assertEqual(err.option_id, "-v/--version")

    def test_conflict_error(self):
        self.assertTrueconflict_error(self.parser.add_option)

    def test_conflict_error_group(self):
        group = OptionGroup(self.parser, "Group 1")
        self.assertTrueconflict_error(group.add_option)

    def test_no_such_conflict_handler(self):
        self.assertRaises(
            self.parser.set_conflict_handler, ('foo',), None,
            ValueError, "invalid conflict_resolution value 'foo'")


class TestConflictResolve(ConflictBase):
    def setUp(self):
        ConflictBase.setUp(self)
        self.parser.set_conflict_handler("resolve")
        self.parser.add_option("-v", "--version", action="callback",
                               callback=self.show_version, help="show version")

    def test_conflict_resolve(self):
        v_opt = self.parser.get_option("-v")
        verbose_opt = self.parser.get_option("--verbose")
        version_opt = self.parser.get_option("--version")

        self.assertTrue(v_opt is version_opt)
        self.assertTrue(v_opt is not verbose_opt)
        self.assertEqual(v_opt._long_opts, ["--version"])
        self.assertEqual(version_opt._short_opts, ["-v"])
        self.assertEqual(version_opt._long_opts, ["--version"])
        self.assertEqual(verbose_opt._short_opts, [])
        self.assertEqual(verbose_opt._long_opts, ["--verbose"])

    def test_conflict_resolve_help(self):
        self.assertOutput(["-h"], """\
Options:
  --verbose      increment verbosity
  -h, --help     show this help message and exit
  -v, --version  show version
""")

    def test_conflict_resolve_short_opt(self):
        self.assertParseOK(["-v"],
                           {'verbose': None, 'show_version': 1},
                           [])

    def test_conflict_resolve_long_opt(self):
        self.assertParseOK(["--verbose"],
                           {'verbose': 1},
                           [])

    def test_conflict_resolve_long_opts(self):
        self.assertParseOK(["--verbose", "--version"],
                           {'verbose': 1, 'show_version': 1},
                           [])

class TestConflictOverride(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.parser.set_conflict_handler("resolve")
        self.parser.add_option("-n", "--dry-run",
                               action="store_true", dest="dry_run",
                               help="don't do anything")
        self.parser.add_option("--dry-run", "-n",
                               action="store_const", const=42, dest="dry_run",
                               help="dry run mode")

    def test_conflict_override_opts(self):
        opt = self.parser.get_option("--dry-run")
        self.assertEqual(opt._short_opts, ["-n"])
        self.assertEqual(opt._long_opts, ["--dry-run"])

    def test_conflict_override_help(self):
        self.assertOutput(["-h"], """\
Options:
  -h, --help     show this help message and exit
  -n, --dry-run  dry run mode
""")

    def test_conflict_override_args(self):
        self.assertParseOK(["-n"],
                           {'dry_run': 42},
                           [])

# -- Other testing. ----------------------------------------------------

_expected_help_basic = """\
Usage: bar.py [options]

Options:
  -a APPLE           throw APPLEs at basket
  -b NUM, --boo=NUM  shout "boo!" NUM times (in order to frighten away all the
                     evil spirits that cause trouble and mayhem)
  --foo=FOO          store FOO in the foo list for later fooing
  -h, --help         show this help message and exit
"""

_expected_help_long_opts_first = """\
Usage: bar.py [options]

Options:
  -a APPLE           throw APPLEs at basket
  --boo=NUM, -b NUM  shout "boo!" NUM times (in order to frighten away all the
                     evil spirits that cause trouble and mayhem)
  --foo=FOO          store FOO in the foo list for later fooing
  --help, -h         show this help message and exit
"""

_expected_help_title_formatter = """\
Usage
=====
  bar.py [options]

Options
=======
-a APPLE           throw APPLEs at basket
--boo=NUM, -b NUM  shout "boo!" NUM times (in order to frighten away all the
                   evil spirits that cause trouble and mayhem)
--foo=FOO          store FOO in the foo list for later fooing
--help, -h         show this help message and exit
"""

_expected_help_short_lines = """\
Usage: bar.py [options]

Options:
  -a APPLE           throw APPLEs at basket
  -b NUM, --boo=NUM  shout "boo!" NUM times (in order to
                     frighten away all the evil spirits
                     that cause trouble and mayhem)
  --foo=FOO          store FOO in the foo list for later
                     fooing
  -h, --help         show this help message and exit
"""

_expected_very_help_short_lines = """\
Usage: bar.py [options]

Options:
  -a APPLE
    throw
    APPLEs at
    basket
  -b NUM, --boo=NUM
    shout
    "boo!" NUM
    times (in
    order to
    frighten
    away all
    the evil
    spirits
    that cause
    trouble and
    mayhem)
  --foo=FOO
    store FOO
    in the foo
    list for
    later
    fooing
  -h, --help
    show this
    help
    message and
    exit
"""

class TestHelp(BaseTest):
    def setUp(self):
        self.parser = self.make_parser(80)

    def make_parser(self, columns):
        options = [
            make_option("-a", type="string", dest='a',
                        metavar="APPLE", help="throw APPLEs at basket"),
            make_option("-b", "--boo", type="int", dest='boo',
                        metavar="NUM",
                        help=
                        "shout \"boo!\" NUM times (in order to frighten away "
                        "all the evil spirits that cause trouble and mayhem)"),
            make_option("--foo", action="append", type="string", dest='foo',
                        help="store FOO in the foo list for later fooing"),
            ]

        # We need to set COLUMNS for the OptionParser constructor, but
        # we must restore its original value -- otherwise, this test
        # screws things up for other tests when it's part of the Python
        # test suite.
        with support.EnvironmentVarGuard() as env:
            env['COLUMNS'] = str(columns)
            return InterceptingOptionParser(option_list=options)

    def assertHelpEquals(self, expected_output):
        save_argv = sys.argv[:]
        try:
            # Make optparse believe bar.py is being executed.
            sys.argv[0] = os.path.join("foo", "bar.py")
            self.assertOutput(["-h"], expected_output)
        finally:
            sys.argv[:] = save_argv

    def test_help(self):
        self.assertHelpEquals(_expected_help_basic)

    def test_help_old_usage(self):
        self.parser.set_usage("Usage: %prog [options]")
        self.assertHelpEquals(_expected_help_basic)

    def test_help_long_opts_first(self):
        self.parser.formatter.short_first = 0
        self.assertHelpEquals(_expected_help_long_opts_first)

    def test_help_title_formatter(self):
        with support.EnvironmentVarGuard() as env:
            env["COLUMNS"] = "80"
            self.parser.formatter = TitledHelpFormatter()
            self.assertHelpEquals(_expected_help_title_formatter)

    def test_wrap_columns(self):
        # Ensure that wrapping respects $COLUMNS environment variable.
        # Need to reconstruct the parser, since that's the only time
        # we look at $COLUMNS.
        self.parser = self.make_parser(60)
        self.assertHelpEquals(_expected_help_short_lines)
        self.parser = self.make_parser(0)
        self.assertHelpEquals(_expected_very_help_short_lines)

    def test_help_unicode(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE)
        self.parser.add_option("-a", action="store_true", help="ol\u00E9!")
        expect = """\
Options:
  -h, --help  show this help message and exit
  -a          ol\u00E9!
"""
        self.assertHelpEquals(expect)

    def test_help_unicode_description(self):
        self.parser = InterceptingOptionParser(usage=SUPPRESS_USAGE,
                                               description="ol\u00E9!")
        expect = """\
ol\u00E9!

Options:
  -h, --help  show this help message and exit
"""
        self.assertHelpEquals(expect)

    def test_help_description_groups(self):
        self.parser.set_description(
            "This is the program description for %prog.  %prog has "
            "an option group as well as single options.")

        group = OptionGroup(
            self.parser, "Dangerous Options",
            "Caution: use of these options is at your own risk.  "
            "It is believed that some of them bite.")
        group.add_option("-g", action="store_true", help="Group option.")
        self.parser.add_option_group(group)

        expect = """\
Usage: bar.py [options]

This is the program description for bar.py.  bar.py has an option group as
well as single options.

Options:
  -a APPLE           throw APPLEs at basket
  -b NUM, --boo=NUM  shout "boo!" NUM times (in order to frighten away all the
                     evil spirits that cause trouble and mayhem)
  --foo=FOO          store FOO in the foo list for later fooing
  -h, --help         show this help message and exit

  Dangerous Options:
    Caution: use of these options is at your own risk.  It is believed
    that some of them bite.

    -g               Group option.
"""

        self.assertHelpEquals(expect)

        self.parser.epilog = "Please report bugs to /dev/null."
        self.assertHelpEquals(expect + "\nPlease report bugs to /dev/null.\n")


class TestMatchAbbrev(BaseTest):
    def test_match_abbrev(self):
        self.assertEqual(_match_abbrev("--f",
                                       {"--foz": None,
                                        "--foo": None,
                                        "--fie": None,
                                        "--f": None}),
                         "--f")

    def test_match_abbrev_error(self):
        s = "--f"
        wordmap = {"--foz": None, "--foo": None, "--fie": None}
        self.assertRaises(
            _match_abbrev, (s, wordmap), None,
            BadOptionError, "ambiguous option: --f (--fie, --foo, --foz?)")


class TestParseNumber(BaseTest):
    def setUp(self):
        self.parser = InterceptingOptionParser()
        self.parser.add_option("-n", type=int)
        self.parser.add_option("-l", type=int)

    def test_parse_num_fail(self):
        self.assertRaises(
            _parse_num, ("", int), {},
            ValueError,
            re.compile(r"invalid literal for int().*: '?'?"))
        self.assertRaises(
            _parse_num, ("0xOoops", int), {},
            ValueError,
            re.compile(r"invalid literal for int().*: s?'?0xOoops'?"))

    def test_parse_num_ok(self):
        self.assertEqual(_parse_num("0", int), 0)
        self.assertEqual(_parse_num("0x10", int), 16)
        self.assertEqual(_parse_num("0XA", int), 10)
        self.assertEqual(_parse_num("010", int), 8)
        self.assertEqual(_parse_num("0b11", int), 3)
        self.assertEqual(_parse_num("0b", int), 0)

    def test_numeric_options(self):
        self.assertParseOK(["-n", "42", "-l", "0x20"],
                           { "n": 42, "l": 0x20 }, [])
        self.assertParseOK(["-n", "0b0101", "-l010"],
                           { "n": 5, "l": 8 }, [])
        self.assertParseFail(["-n008"],
                             "option -n: invalid integer value: '008'")
        self.assertParseFail(["-l0b0123"],
                             "option -l: invalid integer value: '0b0123'")
        self.assertParseFail(["-l", "0x12x"],
                             "option -l: invalid integer value: '0x12x'")


def test_main():
    support.run_unittest(__name__)

if __name__ == '__main__':
    test_main()
