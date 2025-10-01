import datetime
import os
import sys
import contextlib
import importlib.util
import inspect
import io
import pydoc
import py_compile
import keyword
import _pickle
import pkgutil
import re
import stat
import tempfile
import test.support
import time
import types
import typing
import unittest
import unittest.mock
import urllib.parse
import xml.etree
import xml.etree.ElementTree
import textwrap
from io import StringIO
from collections import namedtuple
from urllib.request import urlopen, urlcleanup
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support.script_helper import (assert_python_ok,
                                        assert_python_failure, spawn_python)
from test.support import threading_helper
from test.support import (reap_children, captured_stdout,
                          captured_stderr, is_emscripten, is_wasi,
                          requires_docstrings, MISSING_C_DOCSTRINGS)
from test.support.os_helper import (TESTFN, rmtree, unlink)
from test.test_pydoc import pydoc_mod
from test.test_pydoc import pydocfodder


class nonascii:
    'Це не латиниця'
    pass

if test.support.HAVE_DOCSTRINGS:
    expected_data_docstrings = (
        'dictionary for instance variables',
        'list of weak references to the object',
        ) * 2
else:
    expected_data_docstrings = ('', '', '', '')

expected_text_pattern = """
NAME
    test.test_pydoc.pydoc_mod - This is a test module for test_pydoc
%s
CLASSES
    builtins.object
        A
        B
        C

    class A(builtins.object)
     |  Hello and goodbye
     |
     |  Methods defined here:
     |
     |  __init__()
     |      Wow, I have no function!
     |
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |
     |  __dict__%s
     |
     |  __weakref__%s

    class B(builtins.object)
     |  Data descriptors defined here:
     |
     |  __dict__%s
     |
     |  __weakref__%s
     |
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |
     |  NO_MEANING = 'eggs'

    class C(builtins.object)
     |  Methods defined here:
     |
     |  get_answer(self)
     |      Return say_no()
     |
     |  is_it_true(self)
     |      Return self.get_answer()
     |
     |  say_no(self)
     |
     |  ----------------------------------------------------------------------
     |  Class methods defined here:
     |
     |  __class_getitem__(item)
     |
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |
     |  __dict__
     |      dictionary for instance variables
     |
     |  __weakref__
     |      list of weak references to the object

FUNCTIONS
    doc_func()
        This function solves all of the world's problems:
        hunger
        lack of Python
        war

    nodoc_func()

DATA
    __xyz__ = 'X, Y and Z'
    c_alias = test.test_pydoc.pydoc_mod.C[int]
    list_alias1 = typing.List[int]
    list_alias2 = list[int]
    type_union1 = int | str
    type_union2 = int | str

VERSION
    1.2.3.4

AUTHOR
    Benjamin Peterson

CREDITS
    Nobody

FILE
    %s
""".strip()

expected_text_data_docstrings = tuple('\n     |      ' + s if s else ''
                                      for s in expected_data_docstrings)

html2text_of_expected = """
test.test_pydoc.pydoc_mod (version 1.2.3.4)
This is a test module for test_pydoc

Modules
    types
    typing

Classes
    builtins.object
    A
    B
    C

class A(builtins.object)
    Hello and goodbye

    Methods defined here:
        __init__()
            Wow, I have no function!
    ----------------------------------------------------------------------
    Data descriptors defined here:
        __dict__
            dictionary for instance variables
        __weakref__
            list of weak references to the object

class B(builtins.object)
    Data descriptors defined here:
        __dict__
            dictionary for instance variables
        __weakref__
            list of weak references to the object
    ----------------------------------------------------------------------
    Data and other attributes defined here:
        NO_MEANING = 'eggs'


class C(builtins.object)
    Methods defined here:
        get_answer(self)
            Return say_no()
        is_it_true(self)
            Return self.get_answer()
        say_no(self)
    ----------------------------------------------------------------------
    Class methods defined here:
        __class_getitem__(item)
    ----------------------------------------------------------------------
    Data descriptors defined here:
        __dict__
            dictionary for instance variables
        __weakref__
             list of weak references to the object

Functions
    doc_func()
        This function solves all of the world's problems:
        hunger
        lack of Python
        war
    nodoc_func()

Data
    __xyz__ = 'X, Y and Z'
    c_alias = test.test_pydoc.pydoc_mod.C[int]
    list_alias1 = typing.List[int]
    list_alias2 = list[int]
    type_union1 = int | str
    type_union2 = int | str

Author
    Benjamin Peterson

Credits
    Nobody
"""

expected_html_data_docstrings = tuple(s.replace(' ', '&nbsp;')
                                      for s in expected_data_docstrings)

# output pattern for missing module
missing_pattern = '''\
No Python documentation found for %r.
Use help() to get the interactive help utility.
Use help(str) for help on the str class.'''.replace('\n', os.linesep)

# output pattern for module with bad imports
badimport_pattern = "problem in %s - ModuleNotFoundError: No module named %r"

expected_dynamicattribute_pattern = """
Help on class DA in module %s:

class DA(builtins.object)
 |  Data descriptors defined here:
 |
 |  __dict__%s
 |
 |  __weakref__%s
 |
 |  ham
 |
 |  ----------------------------------------------------------------------
 |  Data and other attributes inherited from Meta:
 |
 |  ham = 'spam'
""".strip()

expected_virtualattribute_pattern1 = """
Help on class Class in module %s:

class Class(builtins.object)
 |  Data and other attributes inherited from Meta:
 |
 |  LIFE = 42
""".strip()

expected_virtualattribute_pattern2 = """
Help on class Class1 in module %s:

class Class1(builtins.object)
 |  Data and other attributes inherited from Meta1:
 |
 |  one = 1
""".strip()

expected_virtualattribute_pattern3 = """
Help on class Class2 in module %s:

class Class2(Class1)
 |  Method resolution order:
 |      Class2
 |      Class1
 |      builtins.object
 |
 |  Data and other attributes inherited from Meta1:
 |
 |  one = 1
 |
 |  ----------------------------------------------------------------------
 |  Data and other attributes inherited from Meta3:
 |
 |  three = 3
 |
 |  ----------------------------------------------------------------------
 |  Data and other attributes inherited from Meta2:
 |
 |  two = 2
""".strip()

expected_missingattribute_pattern = """
Help on class C in module %s:

class C(builtins.object)
 |  Data and other attributes defined here:
 |
 |  here = 'present!'
""".strip()

def run_pydoc(module_name, *args, **env):
    """
    Runs pydoc on the specified module. Returns the stripped
    output of pydoc.
    """
    args = args + (module_name,)
    # do not write bytecode files to avoid caching errors
    rc, out, err = assert_python_ok('-B', pydoc.__file__, *args, **env)
    return out.strip()

def run_pydoc_fail(module_name, *args, **env):
    """
    Runs pydoc on the specified module expecting a failure.
    """
    args = args + (module_name,)
    rc, out, err = assert_python_failure('-B', pydoc.__file__, *args, **env)
    return out.strip()

def get_pydoc_html(module):
    "Returns pydoc generated output as html"
    doc = pydoc.HTMLDoc()
    output = doc.docmodule(module)
    loc = doc.getdocloc(pydoc_mod) or ""
    if loc:
        loc = "<br><a href=\"" + loc + "\">Module Docs</a>"
    return output.strip(), loc

def clean_text(doc):
    # clean up the extra text formatting that pydoc performs
    return re.sub('\b.', '', doc)

def get_pydoc_link(module):
    "Returns a documentation web link of a module"
    abspath = os.path.abspath
    dirname = os.path.dirname
    basedir = dirname(dirname(dirname(abspath(__file__))))
    doc = pydoc.TextDoc()
    loc = doc.getdocloc(module, basedir=basedir)
    return loc

def get_pydoc_text(module):
    "Returns pydoc generated output as text"
    doc = pydoc.TextDoc()
    loc = doc.getdocloc(pydoc_mod) or ""
    if loc:
        loc = "\nMODULE DOCS\n    " + loc + "\n"

    output = doc.docmodule(module)
    output = clean_text(output)
    return output.strip(), loc

def get_html_title(text):
    # Bit of hack, but good enough for test purposes
    header, _, _ = text.partition("</head>")
    _, _, title = header.partition("<title>")
    title, _, _ = title.partition("</title>")
    return title


def html2text(html):
    """A quick and dirty implementation of html2text.

    Tailored for pydoc tests only.
    """
    html = html.replace("<dd>", "\n")
    html = html.replace("<hr>", "-"*70)
    html = re.sub("<.*?>", "", html)
    html = pydoc.replace(html, "&nbsp;", " ", "&gt;", ">", "&lt;", "<")
    return html


class PydocBaseTest(unittest.TestCase):
    def tearDown(self):
        # Self-testing. Mocking only works if sys.modules['pydoc'] and pydoc
        # are the same. But some pydoc functions reload the module and change
        # sys.modules, so check that it was restored.
        self.assertIs(sys.modules['pydoc'], pydoc)

    def _restricted_walk_packages(self, walk_packages, path=None):
        """
        A version of pkgutil.walk_packages() that will restrict itself to
        a given path.
        """
        default_path = path or [os.path.dirname(__file__)]
        def wrapper(path=None, prefix='', onerror=None):
            return walk_packages(path or default_path, prefix, onerror)
        return wrapper

    @contextlib.contextmanager
    def restrict_walk_packages(self, path=None):
        walk_packages = pkgutil.walk_packages
        pkgutil.walk_packages = self._restricted_walk_packages(walk_packages,
                                                               path)
        try:
            yield
        finally:
            pkgutil.walk_packages = walk_packages

    def call_url_handler(self, url, expected_title):
        text = pydoc._url_handler(url, "text/html")
        result = get_html_title(text)
        # Check the title to ensure an unexpected error page was not returned
        self.assertEqual(result, expected_title, text)
        return text


class PydocDocTest(unittest.TestCase):
    maxDiff = None
    def tearDown(self):
        self.assertIs(sys.modules['pydoc'], pydoc)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_html_doc(self):
        result, doc_loc = get_pydoc_html(pydoc_mod)
        text_result = html2text(result)
        text_lines = [line.strip() for line in text_result.splitlines()]
        text_lines = [line for line in text_lines if line]
        del text_lines[1]
        expected_lines = html2text_of_expected.splitlines()
        expected_lines = [line.strip() for line in expected_lines if line]
        self.assertEqual(text_lines, expected_lines)
        mod_file = inspect.getabsfile(pydoc_mod)
        mod_url = urllib.parse.quote(mod_file)
        self.assertIn(mod_url, result)
        self.assertIn(mod_file, result)
        self.assertIn(doc_loc, result)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_text_doc(self):
        result, doc_loc = get_pydoc_text(pydoc_mod)
        expected_text = expected_text_pattern % (
                        (doc_loc,) +
                        expected_text_data_docstrings +
                        (inspect.getabsfile(pydoc_mod),))
        self.assertEqual(expected_text, result)

    def test_text_enum_member_with_value_zero(self):
        # Test issue #20654 to ensure enum member with value 0 can be
        # displayed. It used to throw KeyError: 'zero'.
        import enum
        class BinaryInteger(enum.IntEnum):
            zero = 0
            one = 1
        doc = pydoc.render_doc(BinaryInteger)
        self.assertIn('BinaryInteger.zero', doc)

    def test_slotted_dataclass_with_field_docs(self):
        import dataclasses
        @dataclasses.dataclass(slots=True)
        class My:
            x: int = dataclasses.field(doc='Docstring for x')
        doc = pydoc.render_doc(My)
        self.assertIn('Docstring for x', doc)

    def test_mixed_case_module_names_are_lower_cased(self):
        # issue16484
        doc_link = get_pydoc_link(xml.etree.ElementTree)
        self.assertIn('xml.etree.elementtree', doc_link)

    def test_issue8225(self):
        # Test issue8225 to ensure no doc link appears for xml.etree
        result, doc_loc = get_pydoc_text(xml.etree)
        self.assertEqual(doc_loc, "", "MODULE DOCS incorrectly includes a link")

    def test_getpager_with_stdin_none(self):
        previous_stdin = sys.stdin
        try:
            sys.stdin = None
            pydoc.getpager() # Shouldn't fail.
        finally:
            sys.stdin = previous_stdin

    def test_non_str_name(self):
        # issue14638
        # Treat illegal (non-str) name like no name

        class A:
            __name__ = 42
        class B:
            pass
        adoc = pydoc.render_doc(A())
        bdoc = pydoc.render_doc(B())
        self.assertEqual(adoc.replace("A", "B"), bdoc)

    def test_not_here(self):
        missing_module = "test.i_am_not_here"
        result = str(run_pydoc_fail(missing_module), 'ascii')
        expected = missing_pattern % missing_module
        self.assertEqual(expected, result,
            "documentation for missing module found")

    @requires_docstrings
    def test_not_ascii(self):
        result = run_pydoc('test.test_pydoc.test_pydoc.nonascii', PYTHONIOENCODING='ascii')
        encoded = nonascii.__doc__.encode('ascii', 'backslashreplace')
        self.assertIn(encoded, result)

    def test_input_strip(self):
        missing_module = " test.i_am_not_here "
        result = str(run_pydoc_fail(missing_module), 'ascii')
        expected = missing_pattern % missing_module.strip()
        self.assertEqual(expected, result)

    def test_stripid(self):
        # test with strings, other implementations might have different repr()
        stripid = pydoc.stripid
        # strip the id
        self.assertEqual(stripid('<function stripid at 0x88dcee4>'),
                         '<function stripid>')
        self.assertEqual(stripid('<function stripid at 0x01F65390>'),
                         '<function stripid>')
        # nothing to strip, return the same text
        self.assertEqual(stripid('42'), '42')
        self.assertEqual(stripid("<type 'exceptions.Exception'>"),
                         "<type 'exceptions.Exception'>")

    def test_builtin_with_more_than_four_children(self):
        """Tests help on builtin object which have more than four child classes.

        When running help() on a builtin class which has child classes, it
        should contain a "Built-in subclasses" section and only 4 classes
        should be displayed with a hint on how many more subclasses are present.
        For example:

        >>> help(object)
        Help on class object in module builtins:

        class object
         |  The most base type
         |
         |  Built-in subclasses:
         |      async_generator
         |      BaseException
         |      builtin_function_or_method
         |      bytearray
         |      ... and 82 other subclasses
        """
        doc = pydoc.TextDoc()
        try:
            # Make sure HeapType, which has no __module__ attribute, is one
            # of the known subclasses of object. (doc.docclass() used to
            # fail if HeapType was imported before running this test, like
            # when running tests sequentially.)
            from _testcapi import HeapType  # noqa: F401
        except ImportError:
            pass
        text = doc.docclass(object)
        snip = (" |  Built-in subclasses:\n"
                " |      async_generator\n"
                " |      BaseException\n"
                " |      builtin_function_or_method\n"
                " |      bytearray\n"
                " |      ... and \\d+ other subclasses")
        self.assertRegex(text, snip)

    def test_builtin_with_child(self):
        """Tests help on builtin object which have only child classes.

        When running help() on a builtin class which has child classes, it
        should contain a "Built-in subclasses" section. For example:

        >>> help(ArithmeticError)
        Help on class ArithmeticError in module builtins:

        class ArithmeticError(Exception)
         |  Base class for arithmetic errors.
         |
         ...
         |
         |  Built-in subclasses:
         |      FloatingPointError
         |      OverflowError
         |      ZeroDivisionError
        """
        doc = pydoc.TextDoc()
        text = doc.docclass(ArithmeticError)
        snip = (" |  Built-in subclasses:\n"
                " |      FloatingPointError\n"
                " |      OverflowError\n"
                " |      ZeroDivisionError")
        self.assertIn(snip, text)

    def test_builtin_with_grandchild(self):
        """Tests help on builtin classes which have grandchild classes.

        When running help() on a builtin class which has child classes, it
        should contain a "Built-in subclasses" section. However, if it also has
        grandchildren, these should not show up on the subclasses section.
        For example:

        >>> help(Exception)
        Help on class Exception in module builtins:

        class Exception(BaseException)
         |  Common base class for all non-exit exceptions.
         |
         ...
         |
         |  Built-in subclasses:
         |      ArithmeticError
         |      AssertionError
         |      AttributeError
         ...
        """
        doc = pydoc.TextDoc()
        text = doc.docclass(Exception)
        snip = (" |  Built-in subclasses:\n"
                " |      ArithmeticError\n"
                " |      AssertionError\n"
                " |      AttributeError")
        self.assertIn(snip, text)
        # Testing that the grandchild ZeroDivisionError does not show up
        self.assertNotIn('ZeroDivisionError', text)

    def test_builtin_no_child(self):
        """Tests help on builtin object which have no child classes.

        When running help() on a builtin class which has no child classes, it
        should not contain any "Built-in subclasses" section. For example:

        >>> help(ZeroDivisionError)

        Help on class ZeroDivisionError in module builtins:

        class ZeroDivisionError(ArithmeticError)
         |  Second argument to a division or modulo operation was zero.
         |
         |  Method resolution order:
         |      ZeroDivisionError
         |      ArithmeticError
         |      Exception
         |      BaseException
         |      object
         |
         |  Methods defined here:
         ...
        """
        doc = pydoc.TextDoc()
        text = doc.docclass(ZeroDivisionError)
        # Testing that the subclasses section does not appear
        self.assertNotIn('Built-in subclasses', text)

    def test_builtin_on_metaclasses(self):
        """Tests help on metaclasses.

        When running help() on a metaclasses such as type, it
        should not contain any "Built-in subclasses" section.
        """
        doc = pydoc.TextDoc()
        text = doc.docclass(type)
        # Testing that the subclasses section does not appear
        self.assertNotIn('Built-in subclasses', text)

    def test_fail_help_cli(self):
        elines = (missing_pattern % 'abd').splitlines()
        with spawn_python("-c" "help()") as proc:
            out, _ = proc.communicate(b"abd")
            olines = out.decode().splitlines()[-9:-6]
            olines[0] = olines[0].removeprefix('help> ')
            self.assertEqual(elines, olines)

    def test_fail_help_output_redirect(self):
        with StringIO() as buf:
            helper = pydoc.Helper(output=buf)
            helper.help("abd")
            expected = missing_pattern % "abd"
            self.assertEqual(expected, buf.getvalue().strip().replace('\n', os.linesep))

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @unittest.mock.patch('pydoc.pager')
    @requires_docstrings
    def test_help_output_redirect(self, pager_mock):
        # issue 940286, if output is set in Helper, then all output from
        # Helper.help should be redirected
        self.maxDiff = None

        unused, doc_loc = get_pydoc_text(pydoc_mod)
        module = "test.test_pydoc.pydoc_mod"
        help_header = """
        Help on module test.test_pydoc.pydoc_mod in test.test_pydoc:

        """.lstrip()
        help_header = textwrap.dedent(help_header)
        expected_help_pattern = help_header + expected_text_pattern

        with captured_stdout() as output, captured_stderr() as err:
            buf = StringIO()
            helper = pydoc.Helper(output=buf)
            helper.help(module)
            result = buf.getvalue().strip()
            expected_text = expected_help_pattern % (
                            (doc_loc,) +
                            expected_text_data_docstrings +
                            (inspect.getabsfile(pydoc_mod),))
            self.assertEqual('', output.getvalue())
            self.assertEqual('', err.getvalue())
            self.assertEqual(expected_text, result)

        pager_mock.assert_not_called()

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    @unittest.mock.patch('pydoc.pager')
    def test_help_output_redirect_various_requests(self, pager_mock):
        # issue 940286, if output is set in Helper, then all output from
        # Helper.help should be redirected

        def run_pydoc_for_request(request, expected_text_part):
            """Helper function to run pydoc with its output redirected"""
            with captured_stdout() as output, captured_stderr() as err:
                buf = StringIO()
                helper = pydoc.Helper(output=buf)
                helper.help(request)
                result = buf.getvalue().strip()
                self.assertEqual('', output.getvalue(), msg=f'failed on request "{request}"')
                self.assertEqual('', err.getvalue(), msg=f'failed on request "{request}"')
                self.assertIn(expected_text_part, result, msg=f'failed on request "{request}"')
                pager_mock.assert_not_called()

        self.maxDiff = None

        # test for "keywords"
        run_pydoc_for_request('keywords', 'Here is a list of the Python keywords.')
        # test for "symbols"
        run_pydoc_for_request('symbols', 'Here is a list of the punctuation symbols')
        # test for "topics"
        run_pydoc_for_request('topics', 'Here is a list of available topics.')
        # test for "modules" skipped, see test_modules()
        # test for symbol "%"
        run_pydoc_for_request('%', 'The power operator')
        # test for special True, False, None keywords
        run_pydoc_for_request('True', 'class bool(int)')
        run_pydoc_for_request('False', 'class bool(int)')
        run_pydoc_for_request('None', 'class NoneType(object)')
        # test for keyword "assert"
        run_pydoc_for_request('assert', 'The "assert" statement')
        # test for topic "TYPES"
        run_pydoc_for_request('TYPES', 'The standard type hierarchy')
        # test for "pydoc.Helper.help"
        run_pydoc_for_request('pydoc.Helper.help', 'Help on function help in pydoc.Helper:')
        # test for pydoc.Helper.help
        run_pydoc_for_request(pydoc.Helper.help, 'Help on function help in module pydoc:')
        # test for pydoc.Helper() instance skipped because it is always meant to be interactive

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_help_output_pager(self):
        def run_pydoc_pager(request, what, expected_first_line):
            with (captured_stdout() as output,
                  captured_stderr() as err,
                  unittest.mock.patch('pydoc.pager') as pager_mock,
                  self.subTest(repr(request))):
                helper = pydoc.Helper()
                helper.help(request)
                self.assertEqual('', err.getvalue())
                self.assertEqual('\n', output.getvalue())
                pager_mock.assert_called_once()
                result = clean_text(pager_mock.call_args.args[0])
                self.assertEqual(result.splitlines()[0], expected_first_line)
                self.assertEqual(pager_mock.call_args.args[1], f'Help on {what}')

        run_pydoc_pager('%', 'EXPRESSIONS', 'Operator precedence')
        run_pydoc_pager('True', 'bool object', 'Help on bool object:')
        run_pydoc_pager(True, 'bool object', 'Help on bool object:')
        run_pydoc_pager('assert', 'assert', 'The "assert" statement')
        run_pydoc_pager('TYPES', 'TYPES', 'The standard type hierarchy')
        run_pydoc_pager('pydoc.Helper.help', 'pydoc.Helper.help',
                        'Help on function help in pydoc.Helper:')
        run_pydoc_pager(pydoc.Helper.help, 'Helper.help',
                        'Help on function help in module pydoc:')
        run_pydoc_pager('str', 'str', 'Help on class str in module builtins:')
        run_pydoc_pager(str, 'str', 'Help on class str in module builtins:')
        run_pydoc_pager('str.upper', 'str.upper',
                        'Help on method descriptor upper in str:')
        run_pydoc_pager(str.upper, 'str.upper',
                        'Help on method descriptor upper:')
        run_pydoc_pager(''.upper, 'str.upper',
                        'Help on built-in function upper:')
        run_pydoc_pager(str.__add__,
                        'str.__add__', 'Help on method descriptor __add__:')
        run_pydoc_pager(''.__add__,
                        'str.__add__', 'Help on method wrapper __add__:')
        run_pydoc_pager(int.numerator, 'int.numerator',
                        'Help on getset descriptor builtins.int.numerator:')
        run_pydoc_pager(list[int], 'list',
                        'Help on GenericAlias in module builtins:')
        run_pydoc_pager('sys', 'sys', 'Help on built-in module sys:')
        run_pydoc_pager(sys, 'sys', 'Help on built-in module sys:')

    def test_showtopic(self):
        with captured_stdout() as showtopic_io:
            helper = pydoc.Helper()
            helper.showtopic('with')
        helptext = showtopic_io.getvalue()
        self.assertIn('The "with" statement', helptext)

    def test_fail_showtopic(self):
        with captured_stdout() as showtopic_io:
            helper = pydoc.Helper()
            helper.showtopic('abd')
            expected = "no documentation found for 'abd'"
            self.assertEqual(expected, showtopic_io.getvalue().strip())

    @unittest.mock.patch('pydoc.pager')
    def test_fail_showtopic_output_redirect(self, pager_mock):
        with StringIO() as buf:
            helper = pydoc.Helper(output=buf)
            helper.showtopic("abd")
            expected = "no documentation found for 'abd'"
            self.assertEqual(expected, buf.getvalue().strip())

        pager_mock.assert_not_called()

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    @unittest.mock.patch('pydoc.pager')
    def test_showtopic_output_redirect(self, pager_mock):
        # issue 940286, if output is set in Helper, then all output from
        # Helper.showtopic should be redirected
        self.maxDiff = None

        with captured_stdout() as output, captured_stderr() as err:
            buf = StringIO()
            helper = pydoc.Helper(output=buf)
            helper.showtopic('with')
            result = buf.getvalue().strip()
            self.assertEqual('', output.getvalue())
            self.assertEqual('', err.getvalue())
            self.assertIn('The "with" statement', result)

        pager_mock.assert_not_called()

    def test_lambda_with_return_annotation(self):
        func = lambda a, b, c: 1
        func.__annotations__ = {"return": int}
        with captured_stdout() as help_io:
            pydoc.help(func)
        helptext = help_io.getvalue()
        self.assertIn("lambda (a, b, c) -> int", helptext)

    def test_lambda_without_return_annotation(self):
        func = lambda a, b, c: 1
        func.__annotations__ = {"a": int, "b": int, "c": int}
        with captured_stdout() as help_io:
            pydoc.help(func)
        helptext = help_io.getvalue()
        self.assertIn("lambda (a: int, b: int, c: int)", helptext)

    def test_lambda_with_return_and_params_annotation(self):
        func = lambda a, b, c: 1
        func.__annotations__ = {"a": int, "b": int, "c": int, "return": int}
        with captured_stdout() as help_io:
            pydoc.help(func)
        helptext = help_io.getvalue()
        self.assertIn("lambda (a: int, b: int, c: int) -> int", helptext)

    def test_namedtuple_fields(self):
        Person = namedtuple('Person', ['nickname', 'firstname'])
        with captured_stdout() as help_io:
            pydoc.help(Person)
        helptext = help_io.getvalue()
        self.assertIn("nickname", helptext)
        self.assertIn("firstname", helptext)
        self.assertIn("Alias for field number 0", helptext)
        self.assertIn("Alias for field number 1", helptext)

    def test_namedtuple_public_underscore(self):
        NT = namedtuple('NT', ['abc', 'def'], rename=True)
        with captured_stdout() as help_io:
            pydoc.help(NT)
        helptext = help_io.getvalue()
        self.assertIn('_1', helptext)
        self.assertIn('_replace', helptext)
        self.assertIn('_asdict', helptext)

    def test_synopsis(self):
        self.addCleanup(unlink, TESTFN)
        for encoding in ('ISO-8859-1', 'UTF-8'):
            with open(TESTFN, 'w', encoding=encoding) as script:
                if encoding != 'UTF-8':
                    print('#coding: {}'.format(encoding), file=script)
                print('"""line 1: h\xe9', file=script)
                print('line 2: hi"""', file=script)
            synopsis = pydoc.synopsis(TESTFN, {})
            self.assertEqual(synopsis, 'line 1: h\xe9')

    def test_source_synopsis(self):
        def check(source, expected, encoding=None):
            if isinstance(source, str):
                source_file = StringIO(source)
            else:
                source_file = io.TextIOWrapper(io.BytesIO(source), encoding=encoding)
            with source_file:
                result = pydoc.source_synopsis(source_file)
                self.assertEqual(result, expected)

        check('"""Single line docstring."""',
              'Single line docstring.')
        check('"""First line of docstring.\nSecond line.\nThird line."""',
              'First line of docstring.')
        check('"""First line of docstring.\\nSecond line.\\nThird line."""',
              'First line of docstring.')
        check('"""  Whitespace around docstring.  """',
              'Whitespace around docstring.')
        check('import sys\n"""No docstring"""',
              None)
        check('  \n"""Docstring after empty line."""',
              'Docstring after empty line.')
        check('# Comment\n"""Docstring after comment."""',
              'Docstring after comment.')
        check('  # Indented comment\n"""Docstring after comment."""',
              'Docstring after comment.')
        check('""""""', # Empty docstring
              '')
        check('', # Empty file
              None)
        check('"""Embedded\0null byte"""',
              None)
        check('"""Embedded null byte"""\0',
              None)
        check('"""Café and résumé."""',
              'Café and résumé.')
        check("'''Triple single quotes'''",
              'Triple single quotes')
        check('"Single double quotes"',
              'Single double quotes')
        check("'Single single quotes'",
              'Single single quotes')
        check('"""split\\\nline"""',
              'splitline')
        check('"""Unrecognized escape \\sequence"""',
              'Unrecognized escape \\sequence')
        check('"""Invalid escape seq\\uence"""',
              None)
        check('r"""Raw \\stri\\ng"""',
              'Raw \\stri\\ng')
        check('b"""Bytes literal"""',
              None)
        check('f"""f-string"""',
              None)
        check('"""Concatenated""" \\\n"string" \'literals\'',
              'Concatenatedstringliterals')
        check('"""String""" + """expression"""',
              None)
        check('("""In parentheses""")',
              'In parentheses')
        check('("""Multiple lines """\n"""in parentheses""")',
              'Multiple lines in parentheses')
        check('()', # tuple
              None)
        check(b'# coding: iso-8859-15\n"""\xa4uro sign"""',
              '€uro sign', encoding='iso-8859-15')
        check(b'"""\xa4"""', # Decoding error
              None, encoding='utf-8')

        with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as temp_file:
            temp_file.write('"""Real file test."""\n')
            temp_file.flush()
            temp_file.seek(0)
            result = pydoc.source_synopsis(temp_file)
            self.assertEqual(result, "Real file test.")

    @requires_docstrings
    def test_synopsis_sourceless(self):
        os = import_helper.import_fresh_module('os')
        expected = os.__doc__.splitlines()[0]
        filename = os.__spec__.cached
        synopsis = pydoc.synopsis(filename)

        self.assertEqual(synopsis, expected)

    def test_synopsis_sourceless_empty_doc(self):
        with os_helper.temp_cwd() as test_dir:
            init_path = os.path.join(test_dir, 'foomod42.py')
            cached_path = importlib.util.cache_from_source(init_path)
            with open(init_path, 'w') as fobj:
                fobj.write("foo = 1")
            py_compile.compile(init_path)
            synopsis = pydoc.synopsis(init_path, {})
            self.assertIsNone(synopsis)
            synopsis_cached = pydoc.synopsis(cached_path, {})
            self.assertIsNone(synopsis_cached)

    def test_splitdoc_with_description(self):
        example_string = "I Am A Doc\n\n\nHere is my description"
        self.assertEqual(pydoc.splitdoc(example_string),
                         ('I Am A Doc', '\nHere is my description'))

    def test_is_package_when_not_package(self):
        with os_helper.temp_cwd() as test_dir:
            with self.assertWarns(DeprecationWarning) as cm:
                self.assertFalse(pydoc.ispackage(test_dir))
            self.assertEqual(cm.filename, __file__)

    def test_is_package_when_is_package(self):
        with os_helper.temp_cwd() as test_dir:
            init_path = os.path.join(test_dir, '__init__.py')
            open(init_path, 'w').close()
            with self.assertWarns(DeprecationWarning) as cm:
                self.assertTrue(pydoc.ispackage(test_dir))
            os.remove(init_path)
            self.assertEqual(cm.filename, __file__)

    def test_allmethods(self):
        # issue 17476: allmethods was no longer returning unbound methods.
        # This test is a bit fragile in the face of changes to object and type,
        # but I can't think of a better way to do it without duplicating the
        # logic of the function under test.

        class TestClass(object):
            def method_returning_true(self):
                return True

        # What we expect to get back: everything on object...
        expected = dict(vars(object))
        # ...plus our unbound method...
        expected['method_returning_true'] = TestClass.method_returning_true
        # ...but not the non-methods on object.
        del expected['__doc__']
        del expected['__class__']
        # inspect resolves descriptors on type into methods, but vars doesn't,
        # so we need to update __subclasshook__ and __init_subclass__.
        expected['__subclasshook__'] = TestClass.__subclasshook__
        expected['__init_subclass__'] = TestClass.__init_subclass__

        methods = pydoc.allmethods(TestClass)
        self.assertDictEqual(methods, expected)

    @requires_docstrings
    def test_method_aliases(self):
        class A:
            def tkraise(self, aboveThis=None):
                """Raise this widget in the stacking order."""
            lift = tkraise
            def a_size(self):
                """Return size"""
        class B(A):
            def itemconfigure(self, tagOrId, cnf=None, **kw):
                """Configure resources of an item TAGORID."""
            itemconfig = itemconfigure
            b_size = A.a_size

        doc = pydoc.render_doc(B)
        doc = clean_text(doc)
        self.assertEqual(doc, '''\
Python Library Documentation: class B in module %s

class B(A)
 |  Method resolution order:
 |      B
 |      A
 |      builtins.object
 |
 |  Methods defined here:
 |
 |  b_size = a_size(self)
 |
 |  itemconfig = itemconfigure(self, tagOrId, cnf=None, **kw)
 |
 |  itemconfigure(self, tagOrId, cnf=None, **kw)
 |      Configure resources of an item TAGORID.
 |
 |  ----------------------------------------------------------------------
 |  Methods inherited from A:
 |
 |  a_size(self)
 |      Return size
 |
 |  lift = tkraise(self, aboveThis=None)
 |
 |  tkraise(self, aboveThis=None)
 |      Raise this widget in the stacking order.
 |
 |  ----------------------------------------------------------------------
 |  Data descriptors inherited from A:
 |
 |  __dict__
 |      dictionary for instance variables
 |
 |  __weakref__
 |      list of weak references to the object
''' % __name__)

        doc = pydoc.render_doc(B, renderer=pydoc.HTMLDoc())
        expected_text = f"""
Python Library Documentation

class B in module {__name__}
class B(A)
    Method resolution order:
        B
        A
        builtins.object

    Methods defined here:
        b_size = a_size(self)
        itemconfig = itemconfigure(self, tagOrId, cnf=None, **kw)
        itemconfigure(self, tagOrId, cnf=None, **kw)
            Configure resources of an item TAGORID.

    Methods inherited from A:
        a_size(self)
            Return size
        lift = tkraise(self, aboveThis=None)
        tkraise(self, aboveThis=None)
            Raise this widget in the stacking order.

    Data descriptors inherited from A:
        __dict__
            dictionary for instance variables
        __weakref__
            list of weak references to the object
"""
        as_text = html2text(doc)
        expected_lines = [line.strip() for line in expected_text.split("\n") if line]
        for expected_line in expected_lines:
            self.assertIn(expected_line, as_text)

    def test_long_signatures(self):
        from collections.abc import Callable
        from typing import Literal, Annotated

        class A:
            def __init__(self,
                         arg1: Callable[[int, int, int], str],
                         arg2: Literal['some value', 'other value'],
                         arg3: Annotated[int, 'some docs about this type'],
                         ) -> None:
                ...

        doc = pydoc.render_doc(A)
        doc = clean_text(doc)
        self.assertEqual(doc, '''Python Library Documentation: class A in module %s

class A(builtins.object)
 |  A(
 |      arg1: Callable[[int, int, int], str],
 |      arg2: Literal['some value', 'other value'],
 |      arg3: Annotated[int, 'some docs about this type']
 |  ) -> None
 |
 |  Methods defined here:
 |
 |  __init__(
 |      self,
 |      arg1: Callable[[int, int, int], str],
 |      arg2: Literal['some value', 'other value'],
 |      arg3: Annotated[int, 'some docs about this type']
 |  ) -> None
 |
 |  ----------------------------------------------------------------------
 |  Data descriptors defined here:
 |
 |  __dict__%s
 |
 |  __weakref__%s
''' % (__name__,
       '' if MISSING_C_DOCSTRINGS else '\n |      dictionary for instance variables',
       '' if MISSING_C_DOCSTRINGS else '\n |      list of weak references to the object',
      ))

        def func(
            arg1: Callable[[Annotated[int, 'Some doc']], str],
            arg2: Literal[1, 2, 3, 4, 5, 6, 7, 8],
        ) -> Annotated[int, 'Some other']:
            ...

        doc = pydoc.render_doc(func)
        doc = clean_text(doc)
        self.assertEqual(doc, '''Python Library Documentation: function func in module %s

func(
    arg1: Callable[[Annotated[int, 'Some doc']], str],
    arg2: Literal[1, 2, 3, 4, 5, 6, 7, 8]
) -> Annotated[int, 'Some other']
''' % __name__)

        def function_with_really_long_name_so_annotations_can_be_rather_small(
            arg1: int,
            arg2: str,
        ):
            ...

        doc = pydoc.render_doc(function_with_really_long_name_so_annotations_can_be_rather_small)
        doc = clean_text(doc)
        self.assertEqual(doc, '''Python Library Documentation: function function_with_really_long_name_so_annotations_can_be_rather_small in module %s

function_with_really_long_name_so_annotations_can_be_rather_small(
    arg1: int,
    arg2: str
)
''' % __name__)

        does_not_have_name = lambda \
            very_long_parameter_name_that_should_not_fit_into_a_single_line, \
            second_very_long_parameter_name: ...

        doc = pydoc.render_doc(does_not_have_name)
        doc = clean_text(doc)
        self.assertEqual(doc, '''Python Library Documentation: function <lambda> in module %s

<lambda> lambda very_long_parameter_name_that_should_not_fit_into_a_single_line, second_very_long_parameter_name
''' % __name__)

    def test__future__imports(self):
        # __future__ features are excluded from module help,
        # except when it's the __future__ module itself
        import __future__
        future_text, _ = get_pydoc_text(__future__)
        future_html, _ = get_pydoc_html(__future__)
        pydoc_mod_text, _ = get_pydoc_text(pydoc_mod)
        pydoc_mod_html, _ = get_pydoc_html(pydoc_mod)

        for feature in __future__.all_feature_names:
            txt = f"{feature} = _Feature"
            html = f"<strong>{feature}</strong> = _Feature"
            self.assertIn(txt, future_text)
            self.assertIn(html, future_html)
            self.assertNotIn(txt, pydoc_mod_text)
            self.assertNotIn(html, pydoc_mod_html)


class PydocImportTest(PydocBaseTest):

    def setUp(self):
        self.test_dir = os.mkdir(TESTFN)
        self.addCleanup(rmtree, TESTFN)
        importlib.invalidate_caches()

    def test_badimport(self):
        # This tests the fix for issue 5230, where if pydoc found the module
        # but the module had an internal import error pydoc would report no doc
        # found.
        modname = 'testmod_xyzzy'
        testpairs = (
            ('i_am_not_here', 'i_am_not_here'),
            ('test.i_am_not_here_either', 'test.i_am_not_here_either'),
            ('test.i_am_not_here.neither_am_i', 'test.i_am_not_here'),
            ('i_am_not_here.{}'.format(modname), 'i_am_not_here'),
            ('test.{}'.format(modname), 'test.{}'.format(modname)),
            )

        sourcefn = os.path.join(TESTFN, modname) + os.extsep + "py"
        for importstring, expectedinmsg in testpairs:
            with open(sourcefn, 'w') as f:
                f.write("import {}\n".format(importstring))
            result = run_pydoc_fail(modname, PYTHONPATH=TESTFN).decode("ascii")
            expected = badimport_pattern % (modname, expectedinmsg)
            self.assertEqual(expected, result)

    def test_apropos_with_bad_package(self):
        # Issue 7425 - pydoc -k failed when bad package on path
        pkgdir = os.path.join(TESTFN, "syntaxerr")
        os.mkdir(pkgdir)
        badsyntax = os.path.join(pkgdir, "__init__") + os.extsep + "py"
        with open(badsyntax, 'w') as f:
            f.write("invalid python syntax = $1\n")
        with self.restrict_walk_packages(path=[TESTFN]):
            with captured_stdout() as out:
                with captured_stderr() as err:
                    pydoc.apropos('xyzzy')
            # No result, no error
            self.assertEqual(out.getvalue(), '')
            self.assertEqual(err.getvalue(), '')
            # The package name is still matched
            with captured_stdout() as out:
                with captured_stderr() as err:
                    pydoc.apropos('syntaxerr')
            self.assertEqual(out.getvalue().strip(), 'syntaxerr')
            self.assertEqual(err.getvalue(), '')

    def test_apropos_with_unreadable_dir(self):
        # Issue 7367 - pydoc -k failed when unreadable dir on path
        self.unreadable_dir = os.path.join(TESTFN, "unreadable")
        os.mkdir(self.unreadable_dir, 0)
        self.addCleanup(os.rmdir, self.unreadable_dir)
        # Note, on Windows the directory appears to be still
        #   readable so this is not really testing the issue there
        with self.restrict_walk_packages(path=[TESTFN]):
            with captured_stdout() as out:
                with captured_stderr() as err:
                    pydoc.apropos('SOMEKEY')
        # No result, no error
        self.assertEqual(out.getvalue(), '')
        self.assertEqual(err.getvalue(), '')

    @os_helper.skip_unless_working_chmod
    def test_apropos_empty_doc(self):
        pkgdir = os.path.join(TESTFN, 'walkpkg')
        os.mkdir(pkgdir)
        self.addCleanup(rmtree, pkgdir)
        init_path = os.path.join(pkgdir, '__init__.py')
        with open(init_path, 'w') as fobj:
            fobj.write("foo = 1")
        current_mode = stat.S_IMODE(os.stat(pkgdir).st_mode)
        try:
            os.chmod(pkgdir, current_mode & ~stat.S_IEXEC)
            with self.restrict_walk_packages(path=[TESTFN]), captured_stdout() as stdout:
                pydoc.apropos('')
            self.assertIn('walkpkg', stdout.getvalue())
        finally:
            os.chmod(pkgdir, current_mode)

    def test_url_search_package_error(self):
        # URL handler search should cope with packages that raise exceptions
        pkgdir = os.path.join(TESTFN, "test_error_package")
        os.mkdir(pkgdir)
        init = os.path.join(pkgdir, "__init__.py")
        with open(init, "wt", encoding="ascii") as f:
            f.write("""raise ValueError("ouch")\n""")
        with self.restrict_walk_packages(path=[TESTFN]):
            # Package has to be importable for the error to have any effect
            saved_paths = tuple(sys.path)
            sys.path.insert(0, TESTFN)
            try:
                with self.assertRaisesRegex(ValueError, "ouch"):
                    # Sanity check
                    import test_error_package  # noqa: F401

                text = self.call_url_handler("search?key=test_error_package",
                    "Pydoc: Search Results")
                found = ('<a href="test_error_package.html">'
                    'test_error_package</a>')
                self.assertIn(found, text)
            finally:
                sys.path[:] = saved_paths

    @unittest.skip('causes undesirable side-effects (#20128)')
    def test_modules(self):
        # See Helper.listmodules().
        num_header_lines = 2
        num_module_lines_min = 5  # Playing it safe.
        num_footer_lines = 3
        expected = num_header_lines + num_module_lines_min + num_footer_lines

        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper('modules')
        result = output.getvalue().strip()
        num_lines = len(result.splitlines())

        self.assertGreaterEqual(num_lines, expected)

    @unittest.skip('causes undesirable side-effects (#20128)')
    def test_modules_search(self):
        # See Helper.listmodules().
        expected = 'pydoc - '

        output = StringIO()
        helper = pydoc.Helper(output=output)
        with captured_stdout() as help_io:
            helper('modules pydoc')
        result = help_io.getvalue()

        self.assertIn(expected, result)

    @unittest.skip('some buildbots are not cooperating (#20128)')
    def test_modules_search_builtin(self):
        expected = 'gc - '

        output = StringIO()
        helper = pydoc.Helper(output=output)
        with captured_stdout() as help_io:
            helper('modules garbage')
        result = help_io.getvalue()

        self.assertStartsWith(result, expected)

    def test_importfile(self):
        try:
            loaded_pydoc = pydoc.importfile(pydoc.__file__)

            self.assertIsNot(loaded_pydoc, pydoc)
            self.assertEqual(loaded_pydoc.__name__, 'pydoc')
            self.assertEqual(loaded_pydoc.__file__, pydoc.__file__)
            self.assertEqual(loaded_pydoc.__spec__, pydoc.__spec__)
        finally:
            sys.modules['pydoc'] = pydoc


class Rect:
    @property
    def area(self):
        '''Area of the rect'''
        return self.w * self.h


class Square(Rect):
    area = property(lambda self: self.side**2)


class TestDescriptions(unittest.TestCase):
    def tearDown(self):
        self.assertIs(sys.modules['pydoc'], pydoc)

    def test_module(self):
        # Check that pydocfodder module can be described
        doc = pydoc.render_doc(pydocfodder)
        self.assertIn("pydocfodder", doc)

    def test_class(self):
        class C: "New-style class"
        c = C()

        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'C')
        expected = 'C in module %s object' % __name__
        self.assertIn(expected, pydoc.render_doc(c))

    def test_generic_alias(self):
        self.assertEqual(pydoc.describe(typing.List[int]), '_GenericAlias')
        doc = pydoc.render_doc(typing.List[int], renderer=pydoc.plaintext)
        self.assertIn('_GenericAlias in module typing', doc)
        self.assertIn('List = class list(object)', doc)
        if not MISSING_C_DOCSTRINGS:
            self.assertIn(list.__doc__.strip().splitlines()[0], doc)

        self.assertEqual(pydoc.describe(list[int]), 'GenericAlias')
        doc = pydoc.render_doc(list[int], renderer=pydoc.plaintext)
        self.assertIn('GenericAlias in module builtins', doc)
        self.assertIn('\nclass list(object)', doc)
        if not MISSING_C_DOCSTRINGS:
            self.assertIn(list.__doc__.strip().splitlines()[0], doc)

    def test_union_type(self):
        self.assertEqual(pydoc.describe(typing.Union[int, str]), 'Union')
        doc = pydoc.render_doc(typing.Union[int, str], renderer=pydoc.plaintext)
        self.assertIn('Union in module typing', doc)
        self.assertIn('class Union(builtins.object)', doc)
        if typing.Union.__doc__:
            self.assertIn(typing.Union.__doc__.strip().splitlines()[0], doc)

        self.assertEqual(pydoc.describe(int | str), 'Union')
        doc = pydoc.render_doc(int | str, renderer=pydoc.plaintext)
        self.assertIn('Union in module typing', doc)
        self.assertIn('class Union(builtins.object)', doc)
        if not MISSING_C_DOCSTRINGS:
            self.assertIn(types.UnionType.__doc__.strip().splitlines()[0], doc)

    def test_special_form(self):
        self.assertEqual(pydoc.describe(typing.NoReturn), '_SpecialForm')
        doc = pydoc.render_doc(typing.NoReturn, renderer=pydoc.plaintext)
        self.assertIn('_SpecialForm in module typing', doc)
        if typing.NoReturn.__doc__:
            self.assertIn('NoReturn = typing.NoReturn', doc)
            self.assertIn(typing.NoReturn.__doc__.strip().splitlines()[0], doc)
        else:
            self.assertIn('NoReturn = class _SpecialForm(_Final)', doc)

    def test_typing_pydoc(self):
        def foo(data: typing.List[typing.Any],
                x: int) -> typing.Iterator[typing.Tuple[int, typing.Any]]:
            ...
        T = typing.TypeVar('T')
        class C(typing.Generic[T], typing.Mapping[int, str]): ...
        self.assertEqual(pydoc.render_doc(foo).splitlines()[-1],
                         'f\x08fo\x08oo\x08o(data: typing.List[typing.Any], x: int)'
                         ' -> typing.Iterator[typing.Tuple[int, typing.Any]]')
        self.assertEqual(pydoc.render_doc(C).splitlines()[2],
                         'class C\x08C(collections.abc.Mapping, typing.Generic)')

    def test_builtin(self):
        for name in ('str', 'str.translate', 'builtins.str',
                     'builtins.str.translate'):
            # test low-level function
            self.assertIsNotNone(pydoc.locate(name))
            # test high-level function
            try:
                pydoc.render_doc(name)
            except ImportError:
                self.fail('finding the doc of {!r} failed'.format(name))

        for name in ('notbuiltins', 'strrr', 'strr.translate',
                     'str.trrrranslate', 'builtins.strrr',
                     'builtins.str.trrranslate'):
            self.assertIsNone(pydoc.locate(name))
            self.assertRaises(ImportError, pydoc.render_doc, name)

    @staticmethod
    def _get_summary_line(o):
        text = pydoc.plain(pydoc.render_doc(o))
        lines = text.split('\n')
        assert len(lines) >= 2
        return lines[2]

    @staticmethod
    def _get_summary_lines(o):
        text = pydoc.plain(pydoc.render_doc(o))
        lines = text.split('\n')
        return '\n'.join(lines[2:])

    # these should include "self"
    def test_unbound_python_method(self):
        self.assertEqual(self._get_summary_line(textwrap.TextWrapper.wrap),
            "wrap(self, text)")

    @requires_docstrings
    def test_unbound_builtin_method(self):
        self.assertEqual(self._get_summary_line(_pickle.Pickler.dump),
            "dump(self, obj, /) unbound _pickle.Pickler method")

    # these no longer include "self"
    def test_bound_python_method(self):
        t = textwrap.TextWrapper()
        self.assertEqual(self._get_summary_line(t.wrap),
            "wrap(text) method of textwrap.TextWrapper instance")
    def test_field_order_for_named_tuples(self):
        Person = namedtuple('Person', ['nickname', 'firstname', 'agegroup'])
        s = pydoc.render_doc(Person)
        self.assertLess(s.index('nickname'), s.index('firstname'))
        self.assertLess(s.index('firstname'), s.index('agegroup'))

        class NonIterableFields:
            _fields = None

        class NonHashableFields:
            _fields = [[]]

        # Make sure these doesn't fail
        pydoc.render_doc(NonIterableFields)
        pydoc.render_doc(NonHashableFields)

    @requires_docstrings
    def test_bound_builtin_method(self):
        s = StringIO()
        p = _pickle.Pickler(s)
        self.assertEqual(self._get_summary_line(p.dump),
            "dump(obj, /) method of _pickle.Pickler instance")

    # this should *never* include self!
    @requires_docstrings
    def test_module_level_callable(self):
        self.assertEqual(self._get_summary_line(os.stat),
            "stat(path, *, dir_fd=None, follow_symlinks=True)")

    def test_module_level_callable_noargs(self):
        self.assertEqual(self._get_summary_line(time.time),
            "time()")

    def test_module_level_callable_o(self):
        try:
            import _stat
        except ImportError:
            # stat.S_IMODE() and _stat.S_IMODE() have a different signature
            self.skipTest('_stat extension is missing')

        self.assertEqual(self._get_summary_line(_stat.S_IMODE),
            "S_IMODE(object, /)")

    def test_unbound_builtin_method_noargs(self):
        self.assertEqual(self._get_summary_line(str.lower),
            "lower(self, /) unbound builtins.str method")

    def test_bound_builtin_method_noargs(self):
        self.assertEqual(self._get_summary_line(''.lower),
            "lower() method of builtins.str instance")

    def test_unbound_builtin_method_o(self):
        self.assertEqual(self._get_summary_line(set.add),
            "add(self, object, /) unbound builtins.set method")

    def test_bound_builtin_method_o(self):
        self.assertEqual(self._get_summary_line(set().add),
            "add(object, /) method of builtins.set instance")

    def test_unbound_builtin_method_coexist_o(self):
        self.assertEqual(self._get_summary_line(set.__contains__),
            "__contains__(self, object, /) unbound builtins.set method")

    def test_bound_builtin_method_coexist_o(self):
        self.assertEqual(self._get_summary_line(set().__contains__),
            "__contains__(object, /) method of builtins.set instance")

    def test_unbound_builtin_classmethod_noargs(self):
        self.assertEqual(self._get_summary_line(datetime.datetime.__dict__['utcnow']),
            "utcnow(type, /) unbound datetime.datetime method")

    def test_bound_builtin_classmethod_noargs(self):
        self.assertEqual(self._get_summary_line(datetime.datetime.utcnow),
            "utcnow() class method of datetime.datetime")

    def test_unbound_builtin_classmethod_o(self):
        self.assertEqual(self._get_summary_line(dict.__dict__['__class_getitem__']),
            "__class_getitem__(type, object, /) unbound builtins.dict method")

    def test_bound_builtin_classmethod_o(self):
        self.assertEqual(self._get_summary_line(dict.__class_getitem__),
            "__class_getitem__(object, /) class method of builtins.dict")

    @support.cpython_only
    @requires_docstrings
    def test_module_level_callable_unrepresentable_default(self):
        _testcapi = import_helper.import_module("_testcapi")
        builtin = _testcapi.func_with_unrepresentable_signature
        self.assertEqual(self._get_summary_line(builtin),
            "func_with_unrepresentable_signature(a, b=<x>)")

    @support.cpython_only
    @requires_docstrings
    def test_builtin_staticmethod_unrepresentable_default(self):
        self.assertEqual(self._get_summary_line(str.maketrans),
            "maketrans(x, y=<unrepresentable>, z=<unrepresentable>, /)")
        _testcapi = import_helper.import_module("_testcapi")
        cls = _testcapi.DocStringUnrepresentableSignatureTest
        self.assertEqual(self._get_summary_line(cls.staticmeth),
            "staticmeth(a, b=<x>)")

    @support.cpython_only
    @requires_docstrings
    def test_unbound_builtin_method_unrepresentable_default(self):
        self.assertEqual(self._get_summary_line(dict.pop),
            "pop(self, key, default=<unrepresentable>, /) "
            "unbound builtins.dict method")
        _testcapi = import_helper.import_module("_testcapi")
        cls = _testcapi.DocStringUnrepresentableSignatureTest
        self.assertEqual(self._get_summary_line(cls.meth),
            "meth(self, /, a, b=<x>) unbound "
            "_testcapi.DocStringUnrepresentableSignatureTest method")

    @support.cpython_only
    @requires_docstrings
    def test_bound_builtin_method_unrepresentable_default(self):
        self.assertEqual(self._get_summary_line({}.pop),
            "pop(key, default=<unrepresentable>, /) "
            "method of builtins.dict instance")
        _testcapi = import_helper.import_module("_testcapi")
        obj = _testcapi.DocStringUnrepresentableSignatureTest()
        self.assertEqual(self._get_summary_line(obj.meth),
            "meth(a, b=<x>) "
            "method of _testcapi.DocStringUnrepresentableSignatureTest instance")

    @support.cpython_only
    @requires_docstrings
    def test_unbound_builtin_classmethod_unrepresentable_default(self):
        _testcapi = import_helper.import_module("_testcapi")
        cls = _testcapi.DocStringUnrepresentableSignatureTest
        descr = cls.__dict__['classmeth']
        self.assertEqual(self._get_summary_line(descr),
            "classmeth(type, /, a, b=<x>) unbound "
            "_testcapi.DocStringUnrepresentableSignatureTest method")

    @support.cpython_only
    @requires_docstrings
    def test_bound_builtin_classmethod_unrepresentable_default(self):
        _testcapi = import_helper.import_module("_testcapi")
        cls = _testcapi.DocStringUnrepresentableSignatureTest
        self.assertEqual(self._get_summary_line(cls.classmeth),
            "classmeth(a, b=<x>) class method of "
            "_testcapi.DocStringUnrepresentableSignatureTest")

    def test_overridden_text_signature(self):
        class C:
            def meth(*args, **kwargs):
                pass
            @classmethod
            def cmeth(*args, **kwargs):
                pass
            @staticmethod
            def smeth(*args, **kwargs):
                pass
        for text_signature, unbound, bound in [
            ("($slf)", "(slf, /)", "()"),
            ("($slf, /)", "(slf, /)", "()"),
            ("($slf, /, arg)", "(slf, /, arg)", "(arg)"),
            ("($slf, /, arg=<x>)", "(slf, /, arg=<x>)", "(arg=<x>)"),
            ("($slf, arg, /)", "(slf, arg, /)", "(arg, /)"),
            ("($slf, arg=<x>, /)", "(slf, arg=<x>, /)", "(arg=<x>, /)"),
            ("(/, slf, arg)", "(/, slf, arg)", "(/, slf, arg)"),
            ("(/, slf, arg=<x>)", "(/, slf, arg=<x>)", "(/, slf, arg=<x>)"),
            ("(slf, /, arg)", "(slf, /, arg)", "(arg)"),
            ("(slf, /, arg=<x>)", "(slf, /, arg=<x>)", "(arg=<x>)"),
            ("(slf, arg, /)", "(slf, arg, /)", "(arg, /)"),
            ("(slf, arg=<x>, /)", "(slf, arg=<x>, /)", "(arg=<x>, /)"),
        ]:
            with self.subTest(text_signature):
                C.meth.__text_signature__ = text_signature
                self.assertEqual(self._get_summary_line(C.meth),
                        "meth" + unbound)
                self.assertEqual(self._get_summary_line(C().meth),
                        "meth" + bound + " method of test.test_pydoc.test_pydoc.C instance")
                C.cmeth.__func__.__text_signature__ = text_signature
                self.assertEqual(self._get_summary_line(C.cmeth),
                        "cmeth" + bound + " class method of test.test_pydoc.test_pydoc.C")
                C.smeth.__text_signature__ = text_signature
                self.assertEqual(self._get_summary_line(C.smeth),
                        "smeth" + unbound)

    @requires_docstrings
    def test_staticmethod(self):
        class X:
            @staticmethod
            def sm(x, y):
                '''A static method'''
                ...
        self.assertEqual(self._get_summary_lines(X.__dict__['sm']),
                         'sm(x, y)\n'
                         '    A static method\n')
        self.assertEqual(self._get_summary_lines(X.sm), """\
sm(x, y)
    A static method
""")
        self.assertIn("""
 |  Static methods defined here:
 |
 |  sm(x, y)
 |      A static method
""", pydoc.plain(pydoc.render_doc(X)))

    @requires_docstrings
    def test_classmethod(self):
        class X:
            @classmethod
            def cm(cls, x):
                '''A class method'''
                ...
        self.assertEqual(self._get_summary_lines(X.__dict__['cm']),
                         'cm(...)\n'
                         '    A class method\n')
        self.assertEqual(self._get_summary_lines(X.cm), """\
cm(x) class method of test.test_pydoc.test_pydoc.X
    A class method
""")
        self.assertIn("""
 |  Class methods defined here:
 |
 |  cm(x)
 |      A class method
""", pydoc.plain(pydoc.render_doc(X)))

    @requires_docstrings
    def test_getset_descriptor(self):
        # Currently these attributes are implemented as getset descriptors
        # in CPython.
        self.assertEqual(self._get_summary_line(int.numerator), "numerator")
        self.assertEqual(self._get_summary_line(float.real), "real")
        self.assertEqual(self._get_summary_line(Exception.args), "args")
        self.assertEqual(self._get_summary_line(memoryview.obj), "obj")

    @requires_docstrings
    def test_member_descriptor(self):
        # Currently these attributes are implemented as member descriptors
        # in CPython.
        self.assertEqual(self._get_summary_line(complex.real), "real")
        self.assertEqual(self._get_summary_line(range.start), "start")
        self.assertEqual(self._get_summary_line(slice.start), "start")
        self.assertEqual(self._get_summary_line(property.fget), "fget")
        self.assertEqual(self._get_summary_line(StopIteration.value), "value")

    @requires_docstrings
    def test_slot_descriptor(self):
        class Point:
            __slots__ = 'x', 'y'
        self.assertEqual(self._get_summary_line(Point.x), "x")

    @requires_docstrings
    def test_dict_attr_descriptor(self):
        class NS:
            pass
        self.assertEqual(self._get_summary_line(NS.__dict__['__dict__']),
                         "__dict__")

    @requires_docstrings
    def test_structseq_member_descriptor(self):
        self.assertEqual(self._get_summary_line(type(sys.hash_info).width),
                         "width")
        self.assertEqual(self._get_summary_line(type(sys.flags).debug),
                         "debug")
        self.assertEqual(self._get_summary_line(type(sys.version_info).major),
                         "major")
        self.assertEqual(self._get_summary_line(type(sys.float_info).max),
                         "max")

    @requires_docstrings
    def test_namedtuple_field_descriptor(self):
        Box = namedtuple('Box', ('width', 'height'))
        self.assertEqual(self._get_summary_lines(Box.width), """\
    Alias for field number 0
""")

    @requires_docstrings
    def test_property(self):
        self.assertEqual(self._get_summary_lines(Rect.area), """\
area
    Area of the rect
""")
        # inherits the docstring from Rect.area
        self.assertEqual(self._get_summary_lines(Square.area), """\
area
    Area of the rect
""")
        self.assertIn("""
 |  area
 |      Area of the rect
""", pydoc.plain(pydoc.render_doc(Rect)))

    @requires_docstrings
    def test_custom_non_data_descriptor(self):
        class Descr:
            def __get__(self, obj, cls):
                if obj is None:
                    return self
                return 42
        class X:
            attr = Descr()

        self.assertEqual(self._get_summary_lines(X.attr), f"""\
<{__name__}.TestDescriptions.test_custom_non_data_descriptor.<locals>.Descr object>""")

        X.attr.__doc__ = 'Custom descriptor'
        self.assertEqual(self._get_summary_lines(X.attr), f"""\
<{__name__}.TestDescriptions.test_custom_non_data_descriptor.<locals>.Descr object>
    Custom descriptor
""")

        X.attr.__name__ = 'foo'
        self.assertEqual(self._get_summary_lines(X.attr), """\
foo(...)
    Custom descriptor
""")

    @requires_docstrings
    def test_custom_data_descriptor(self):
        class Descr:
            def __get__(self, obj, cls):
                if obj is None:
                    return self
                return 42
            def __set__(self, obj, cls):
                1/0
        class X:
            attr = Descr()

        self.assertEqual(self._get_summary_lines(X.attr), "")

        X.attr.__doc__ = 'Custom descriptor'
        self.assertEqual(self._get_summary_lines(X.attr), """\
    Custom descriptor
""")

        X.attr.__name__ = 'foo'
        self.assertEqual(self._get_summary_lines(X.attr), """\
foo
    Custom descriptor
""")

    def test_async_annotation(self):
        async def coro_function(ign) -> int:
            return 1

        text = pydoc.plain(pydoc.plaintext.document(coro_function))
        self.assertIn('async coro_function', text)

        html = pydoc.HTMLDoc().document(coro_function)
        self.assertIn(
            'async <a name="-coro_function"><strong>coro_function',
            html)

    def test_async_generator_annotation(self):
        async def an_async_generator():
            yield 1

        text = pydoc.plain(pydoc.plaintext.document(an_async_generator))
        self.assertIn('async an_async_generator', text)

        html = pydoc.HTMLDoc().document(an_async_generator)
        self.assertIn(
            'async <a name="-an_async_generator"><strong>an_async_generator',
            html)

    @requires_docstrings
    def test_html_for_https_links(self):
        def a_fn_with_https_link():
            """a link https://localhost/"""
            pass

        html = pydoc.HTMLDoc().document(a_fn_with_https_link)
        self.assertIn(
            '<a href="https://localhost/">https://localhost/</a>',
            html
        )

    def test_module_none(self):
        # Issue #128772
        from test.test_pydoc import module_none
        pydoc.render_doc(module_none)


class PydocFodderTest(unittest.TestCase):
    def tearDown(self):
        self.assertIs(sys.modules['pydoc'], pydoc)

    def getsection(self, text, beginline, endline):
        lines = text.splitlines()
        beginindex, endindex = 0, None
        if beginline is not None:
            beginindex = lines.index(beginline)
        if endline is not None:
            endindex = lines.index(endline, beginindex)
        return lines[beginindex:endindex]

    def test_text_doc_routines_in_class(self, cls=pydocfodder.B):
        doc = pydoc.TextDoc()
        result = doc.docclass(cls)
        result = clean_text(result)
        where = 'defined here' if cls is pydocfodder.B else 'inherited from B'
        lines = self.getsection(result, f' |  Methods {where}:', ' |  ' + '-'*70)
        self.assertIn(' |  A_method_alias = A_method(self)', lines)
        self.assertIn(' |  B_method_alias = B_method(self)', lines)
        self.assertIn(' |  A_staticmethod(x, y) from test.test_pydoc.pydocfodder.A', lines)
        self.assertIn(' |  A_staticmethod_alias = A_staticmethod(x, y)', lines)
        self.assertIn(' |  global_func(x, y) from test.test_pydoc.pydocfodder', lines)
        self.assertIn(' |  global_func_alias = global_func(x, y)', lines)
        self.assertIn(' |  global_func2_alias = global_func2(x, y) from test.test_pydoc.pydocfodder', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn(' |  count(self, value, /) from builtins.list', lines)
            self.assertIn(' |  list_count = count(self, value, /)', lines)
            self.assertIn(' |  __repr__(self, /) from builtins.object', lines)
            self.assertIn(' |  object_repr = __repr__(self, /)', lines)
        else:
            self.assertIn(' |  count(self, object, /) from builtins.list', lines)
            self.assertIn(' |  list_count = count(self, object, /)', lines)
            self.assertIn(' |  __repr__(...) from builtins.object', lines)
            self.assertIn(' |  object_repr = __repr__(...)', lines)

        lines = self.getsection(result, f' |  Static methods {where}:', ' |  ' + '-'*70)
        self.assertIn(' |  A_classmethod_ref = A_classmethod(x) class method of test.test_pydoc.pydocfodder.A', lines)
        note = '' if cls is pydocfodder.B else ' class method of test.test_pydoc.pydocfodder.B'
        self.assertIn(' |  B_classmethod_ref = B_classmethod(x)' + note, lines)
        self.assertIn(' |  A_method_ref = A_method() method of test.test_pydoc.pydocfodder.A instance', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn(' |  get(key, default=None, /) method of builtins.dict instance', lines)
            self.assertIn(' |  dict_get = get(key, default=None, /) method of builtins.dict instance', lines)
        else:
            self.assertIn(' |  get(...) method of builtins.dict instance', lines)
            self.assertIn(' |  dict_get = get(...) method of builtins.dict instance', lines)

        lines = self.getsection(result, f' |  Class methods {where}:', ' |  ' + '-'*70)
        self.assertIn(' |  B_classmethod(x)', lines)
        self.assertIn(' |  B_classmethod_alias = B_classmethod(x)', lines)

    def test_html_doc_routines_in_class(self, cls=pydocfodder.B):
        doc = pydoc.HTMLDoc()
        result = doc.docclass(cls)
        result = html2text(result)
        where = 'defined here' if cls is pydocfodder.B else 'inherited from B'
        lines = self.getsection(result, f'Methods {where}:', '-'*70)
        self.assertIn('A_method_alias = A_method(self)', lines)
        self.assertIn('B_method_alias = B_method(self)', lines)
        self.assertIn('A_staticmethod(x, y) from test.test_pydoc.pydocfodder.A', lines)
        self.assertIn('A_staticmethod_alias = A_staticmethod(x, y)', lines)
        self.assertIn('global_func(x, y) from test.test_pydoc.pydocfodder', lines)
        self.assertIn('global_func_alias = global_func(x, y)', lines)
        self.assertIn('global_func2_alias = global_func2(x, y) from test.test_pydoc.pydocfodder', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn('count(self, value, /) from builtins.list', lines)
            self.assertIn('list_count = count(self, value, /)', lines)
            self.assertIn('__repr__(self, /) from builtins.object', lines)
            self.assertIn('object_repr = __repr__(self, /)', lines)
        else:
            self.assertIn('count(self, object, /) from builtins.list', lines)
            self.assertIn('list_count = count(self, object, /)', lines)
            self.assertIn('__repr__(...) from builtins.object', lines)
            self.assertIn('object_repr = __repr__(...)', lines)

        lines = self.getsection(result, f'Static methods {where}:', '-'*70)
        self.assertIn('A_classmethod_ref = A_classmethod(x) class method of test.test_pydoc.pydocfodder.A', lines)
        note = '' if cls is pydocfodder.B else ' class method of test.test_pydoc.pydocfodder.B'
        self.assertIn('B_classmethod_ref = B_classmethod(x)' + note, lines)
        self.assertIn('A_method_ref = A_method() method of test.test_pydoc.pydocfodder.A instance', lines)

        lines = self.getsection(result, f'Class methods {where}:', '-'*70)
        self.assertIn('B_classmethod(x)', lines)
        self.assertIn('B_classmethod_alias = B_classmethod(x)', lines)

    def test_text_doc_inherited_routines_in_class(self):
        self.test_text_doc_routines_in_class(pydocfodder.D)

    def test_html_doc_inherited_routines_in_class(self):
        self.test_html_doc_routines_in_class(pydocfodder.D)

    def test_text_doc_routines_in_module(self):
        doc = pydoc.TextDoc()
        result = doc.docmodule(pydocfodder)
        result = clean_text(result)
        lines = self.getsection(result, 'FUNCTIONS', 'FILE')
        # function alias
        self.assertIn('    global_func_alias = global_func(x, y)', lines)
        self.assertIn('    A_staticmethod(x, y)', lines)
        self.assertIn('    A_staticmethod_alias = A_staticmethod(x, y)', lines)
        # bound class methods
        self.assertIn('    A_classmethod(x) class method of A', lines)
        self.assertIn('    A_classmethod2 = A_classmethod(x) class method of A', lines)
        self.assertIn('    A_classmethod3 = A_classmethod(x) class method of B', lines)
        # bound methods
        self.assertIn('    A_method() method of A instance', lines)
        self.assertIn('    A_method2 = A_method() method of A instance', lines)
        self.assertIn('    A_method3 = A_method() method of B instance', lines)
        self.assertIn('    A_staticmethod_ref = A_staticmethod(x, y)', lines)
        self.assertIn('    A_staticmethod_ref2 = A_staticmethod(y) method of B instance', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn('    get(key, default=None, /) method of builtins.dict instance', lines)
            self.assertIn('    dict_get = get(key, default=None, /) method of builtins.dict instance', lines)
        else:
            self.assertIn('    get(...) method of builtins.dict instance', lines)
            self.assertIn('    dict_get = get(...) method of builtins.dict instance', lines)

        # unbound methods
        self.assertIn('    B_method(self)', lines)
        self.assertIn('    B_method2 = B_method(self)', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn('    count(self, value, /) unbound builtins.list method', lines)
            self.assertIn('    list_count = count(self, value, /) unbound builtins.list method', lines)
            self.assertIn('    __repr__(self, /) unbound builtins.object method', lines)
            self.assertIn('    object_repr = __repr__(self, /) unbound builtins.object method', lines)
        else:
            self.assertIn('    count(self, object, /) unbound builtins.list method', lines)
            self.assertIn('    list_count = count(self, object, /) unbound builtins.list method', lines)
            self.assertIn('    __repr__(...) unbound builtins.object method', lines)
            self.assertIn('    object_repr = __repr__(...) unbound builtins.object method', lines)


    def test_html_doc_routines_in_module(self):
        doc = pydoc.HTMLDoc()
        result = doc.docmodule(pydocfodder)
        result = html2text(result)
        lines = self.getsection(result, ' Functions', None)
        # function alias
        self.assertIn(' global_func_alias = global_func(x, y)', lines)
        self.assertIn(' A_staticmethod(x, y)', lines)
        self.assertIn(' A_staticmethod_alias = A_staticmethod(x, y)', lines)
        # bound class methods
        self.assertIn('A_classmethod(x) class method of A', lines)
        self.assertIn(' A_classmethod2 = A_classmethod(x) class method of A', lines)
        self.assertIn(' A_classmethod3 = A_classmethod(x) class method of B', lines)
        # bound methods
        self.assertIn(' A_method() method of A instance', lines)
        self.assertIn(' A_method2 = A_method() method of A instance', lines)
        self.assertIn(' A_method3 = A_method() method of B instance', lines)
        self.assertIn(' A_staticmethod_ref = A_staticmethod(x, y)', lines)
        self.assertIn(' A_staticmethod_ref2 = A_staticmethod(y) method of B instance', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn(' get(key, default=None, /) method of builtins.dict instance', lines)
            self.assertIn(' dict_get = get(key, default=None, /) method of builtins.dict instance', lines)
        else:
            self.assertIn(' get(...) method of builtins.dict instance', lines)
            self.assertIn(' dict_get = get(...) method of builtins.dict instance', lines)
        # unbound methods
        self.assertIn(' B_method(self)', lines)
        self.assertIn(' B_method2 = B_method(self)', lines)
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn(' count(self, value, /) unbound builtins.list method', lines)
            self.assertIn(' list_count = count(self, value, /) unbound builtins.list method', lines)
            self.assertIn(' __repr__(self, /) unbound builtins.object method', lines)
            self.assertIn(' object_repr = __repr__(self, /) unbound builtins.object method', lines)
        else:
            self.assertIn(' count(self, object, /) unbound builtins.list method', lines)
            self.assertIn(' list_count = count(self, object, /) unbound builtins.list method', lines)
            self.assertIn(' __repr__(...) unbound builtins.object method', lines)
            self.assertIn(' object_repr = __repr__(...) unbound builtins.object method', lines)


@unittest.skipIf(
    is_emscripten or is_wasi,
    "Socket server not available on Emscripten/WASI."
)
class PydocServerTest(unittest.TestCase):
    """Tests for pydoc._start_server"""
    def tearDown(self):
        self.assertIs(sys.modules['pydoc'], pydoc)

    def test_server(self):
        # Minimal test that starts the server, checks that it works, then stops
        # it and checks its cleanup.
        def my_url_handler(url, content_type):
            text = 'the URL sent was: (%s, %s)' % (url, content_type)
            return text

        serverthread = pydoc._start_server(
            my_url_handler,
            hostname='localhost',
            port=0,
            )
        self.assertEqual(serverthread.error, None)
        self.assertTrue(serverthread.serving)
        self.addCleanup(
            lambda: serverthread.stop() if serverthread.serving else None
            )
        self.assertIn('localhost', serverthread.url)

        self.addCleanup(urlcleanup)
        self.assertEqual(
            b'the URL sent was: (/test, text/html)',
            urlopen(urllib.parse.urljoin(serverthread.url, '/test')).read(),
            )
        self.assertEqual(
            b'the URL sent was: (/test.css, text/css)',
            urlopen(urllib.parse.urljoin(serverthread.url, '/test.css')).read(),
            )

        serverthread.stop()
        self.assertFalse(serverthread.serving)
        self.assertIsNone(serverthread.docserver)
        self.assertIsNone(serverthread.url)


class PydocUrlHandlerTest(PydocBaseTest):
    """Tests for pydoc._url_handler"""

    def test_content_type_err(self):
        f = pydoc._url_handler
        self.assertRaises(TypeError, f, 'A', '')
        self.assertRaises(TypeError, f, 'B', 'foobar')

    def test_url_requests(self):
        # Test for the correct title in the html pages returned.
        # This tests the different parts of the URL handler without
        # getting too picky about the exact html.
        requests = [
            ("", "Pydoc: Index of Modules"),
            ("get?key=", "Pydoc: Index of Modules"),
            ("index", "Pydoc: Index of Modules"),
            ("topics", "Pydoc: Topics"),
            ("keywords", "Pydoc: Keywords"),
            ("pydoc", "Pydoc: module pydoc"),
            ("get?key=pydoc", "Pydoc: module pydoc"),
            ("search?key=pydoc", "Pydoc: Search Results"),
            ("topic?key=def", "Pydoc: KEYWORD def"),
            ("topic?key=STRINGS", "Pydoc: TOPIC STRINGS"),
            ("foobar", "Pydoc: Error - foobar"),
            ]

        self.assertIs(sys.modules['pydoc'], pydoc)
        try:
            with self.restrict_walk_packages():
                for url, title in requests:
                    self.call_url_handler(url, title)
        finally:
            # Some requests reload the module and change sys.modules.
            sys.modules['pydoc'] = pydoc


class TestHelper(unittest.TestCase):
    def test_keywords(self):
        self.assertEqual(sorted(pydoc.Helper.keywords),
                         sorted(keyword.kwlist))


class PydocWithMetaClasses(unittest.TestCase):
    def tearDown(self):
        self.assertIs(sys.modules['pydoc'], pydoc)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_DynamicClassAttribute(self):
        class Meta(type):
            def __getattr__(self, name):
                if name == 'ham':
                    return 'spam'
                return super().__getattr__(name)
        class DA(metaclass=Meta):
            @types.DynamicClassAttribute
            def ham(self):
                return 'eggs'
        expected_text_data_docstrings = tuple('\n |      ' + s if s else ''
                                      for s in expected_data_docstrings)
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(DA)
        expected_text = expected_dynamicattribute_pattern % (
                (__name__,) + expected_text_data_docstrings[:2])
        result = output.getvalue().strip()
        self.assertEqual(expected_text, result)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_virtualClassAttributeWithOneMeta(self):
        class Meta(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'LIFE']
            def __getattr__(self, name):
                if name =='LIFE':
                    return 42
                return super().__getattr(name)
        class Class(metaclass=Meta):
            pass
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(Class)
        expected_text = expected_virtualattribute_pattern1 % __name__
        result = output.getvalue().strip()
        self.assertEqual(expected_text, result)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_virtualClassAttributeWithTwoMeta(self):
        class Meta1(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'one']
            def __getattr__(self, name):
                if name =='one':
                    return 1
                return super().__getattr__(name)
        class Meta2(type):
            def __dir__(cls):
                return ['__class__', '__module__', '__name__', 'two']
            def __getattr__(self, name):
                if name =='two':
                    return 2
                return super().__getattr__(name)
        class Meta3(Meta1, Meta2):
            def __dir__(cls):
                return list(sorted(set(
                    ['__class__', '__module__', '__name__', 'three'] +
                    Meta1.__dir__(cls) + Meta2.__dir__(cls))))
            def __getattr__(self, name):
                if name =='three':
                    return 3
                return super().__getattr__(name)
        class Class1(metaclass=Meta1):
            pass
        class Class2(Class1, metaclass=Meta3):
            pass
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(Class1)
        expected_text1 = expected_virtualattribute_pattern2 % __name__
        result1 = output.getvalue().strip()
        self.assertEqual(expected_text1, result1)
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(Class2)
        expected_text2 = expected_virtualattribute_pattern3 % __name__
        result2 = output.getvalue().strip()
        self.assertEqual(expected_text2, result2)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    @requires_docstrings
    def test_buggy_dir(self):
        class M(type):
            def __dir__(cls):
                return ['__class__', '__name__', 'missing', 'here']
        class C(metaclass=M):
            here = 'present!'
        output = StringIO()
        helper = pydoc.Helper(output=output)
        helper(C)
        expected_text = expected_missingattribute_pattern % __name__
        result = output.getvalue().strip()
        self.assertEqual(expected_text, result)

    def test_resolve_false(self):
        # Issue #23008: pydoc enum.{,Int}Enum failed
        # because bool(enum.Enum) is False.
        with captured_stdout() as help_io:
            pydoc.help('enum.Enum')
        helptext = help_io.getvalue()
        self.assertIn('class Enum', helptext)


class TestInternalUtilities(unittest.TestCase):

    def setUp(self):
        tmpdir = tempfile.TemporaryDirectory()
        self.argv0dir = tmpdir.name
        self.argv0 = os.path.join(tmpdir.name, "nonexistent")
        self.addCleanup(tmpdir.cleanup)
        self.abs_curdir = abs_curdir = os.getcwd()
        self.curdir_spellings = ["", os.curdir, abs_curdir]

    def _get_revised_path(self, given_path, argv0=None):
        # Checking that pydoc.cli() actually calls pydoc._get_revised_path()
        # is handled via code review (at least for now).
        if argv0 is None:
            argv0 = self.argv0
        return pydoc._get_revised_path(given_path, argv0)

    def _get_starting_path(self):
        # Get a copy of sys.path without the current directory.
        clean_path = sys.path.copy()
        for spelling in self.curdir_spellings:
            for __ in range(clean_path.count(spelling)):
                clean_path.remove(spelling)
        return clean_path

    def test_sys_path_adjustment_adds_missing_curdir(self):
        clean_path = self._get_starting_path()
        expected_path = [self.abs_curdir] + clean_path
        self.assertEqual(self._get_revised_path(clean_path), expected_path)

    def test_sys_path_adjustment_removes_argv0_dir(self):
        clean_path = self._get_starting_path()
        expected_path = [self.abs_curdir] + clean_path
        leading_argv0dir = [self.argv0dir] + clean_path
        self.assertEqual(self._get_revised_path(leading_argv0dir), expected_path)
        trailing_argv0dir = clean_path + [self.argv0dir]
        self.assertEqual(self._get_revised_path(trailing_argv0dir), expected_path)

    def test_sys_path_adjustment_protects_pydoc_dir(self):
        def _get_revised_path(given_path):
            return self._get_revised_path(given_path, argv0=pydoc.__file__)
        clean_path = self._get_starting_path()
        leading_argv0dir = [self.argv0dir] + clean_path
        expected_path = [self.abs_curdir] + leading_argv0dir
        self.assertEqual(_get_revised_path(leading_argv0dir), expected_path)
        trailing_argv0dir = clean_path + [self.argv0dir]
        expected_path = [self.abs_curdir] + trailing_argv0dir
        self.assertEqual(_get_revised_path(trailing_argv0dir), expected_path)

    def test_sys_path_adjustment_when_curdir_already_included(self):
        clean_path = self._get_starting_path()
        for spelling in self.curdir_spellings:
            with self.subTest(curdir_spelling=spelling):
                # If curdir is already present, no alterations are made at all
                leading_curdir = [spelling] + clean_path
                self.assertIsNone(self._get_revised_path(leading_curdir))
                trailing_curdir = clean_path + [spelling]
                self.assertIsNone(self._get_revised_path(trailing_curdir))
                leading_argv0dir = [self.argv0dir] + leading_curdir
                self.assertIsNone(self._get_revised_path(leading_argv0dir))
                trailing_argv0dir = trailing_curdir + [self.argv0dir]
                self.assertIsNone(self._get_revised_path(trailing_argv0dir))


def setUpModule():
    thread_info = threading_helper.threading_setup()
    unittest.addModuleCleanup(threading_helper.threading_cleanup, *thread_info)
    unittest.addModuleCleanup(reap_children)


if __name__ == "__main__":
    unittest.main()
