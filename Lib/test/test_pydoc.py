import os
import sys
import difflib
import __builtin__
import re
import py_compile
import pydoc
import contextlib
import inspect
import keyword
import pkgutil
import unittest
import xml.etree
import types
import test.test_support
import xml.etree.ElementTree
from collections import namedtuple
from test.script_helper import assert_python_ok
from test.test_support import (TESTFN, rmtree, reap_children, captured_stdout,
                               captured_stderr, requires_docstrings)

from test import pydoc_mod

if test.test_support.HAVE_DOCSTRINGS:
    expected_data_docstrings = (
        'dictionary for instance variables (if defined)',
        'list of weak references to the object (if defined)',
        )
else:
    expected_data_docstrings = ('', '')

expected_text_pattern = \
"""
NAME
    test.pydoc_mod - This is a test module for test_pydoc

FILE
    %s
%s
CLASSES
    __builtin__.object
        B
        C
    A
\x20\x20\x20\x20
    class A
     |  Hello and goodbye
     |\x20\x20
     |  Methods defined here:
     |\x20\x20
     |  __init__()
     |      Wow, I have no function!
\x20\x20\x20\x20
    class B(__builtin__.object)
     |  Data descriptors defined here:
     |\x20\x20
     |  __dict__%s
     |\x20\x20
     |  __weakref__%s
     |\x20\x20
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |\x20\x20
     |  NO_MEANING = 'eggs'
\x20\x20\x20\x20
    class C(__builtin__.object)
     |  Methods defined here:
     |\x20\x20
     |  get_answer(self)
     |      Return say_no()
     |\x20\x20
     |  is_it_true(self)
     |      Return self.get_answer()
     |\x20\x20
     |  say_no(self)
     |\x20\x20
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |\x20\x20
     |  __dict__
     |      dictionary for instance variables (if defined)
     |\x20\x20
     |  __weakref__
     |      list of weak references to the object (if defined)

FUNCTIONS
    doc_func()
        This function solves all of the world's problems:
        hunger
        lack of Python
        war
\x20\x20\x20\x20
    nodoc_func()

DATA
    __author__ = 'Benjamin Peterson'
    __credits__ = 'Nobody'
    __version__ = '1.2.3.4'

VERSION
    1.2.3.4

AUTHOR
    Benjamin Peterson

CREDITS
    Nobody
""".strip()

expected_text_data_docstrings = tuple('\n     |      ' + s if s else ''
                                      for s in expected_data_docstrings)

expected_html_pattern = \
"""
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="heading">
<tr bgcolor="#7799ee">
<td valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial">&nbsp;<br><big><big><strong><a href="test.html"><font color="#ffffff">test</font></a>.pydoc_mod</strong></big></big> (version 1.2.3.4)</font></td
><td align=right valign=bottom
><font color="#ffffff" face="helvetica, arial"><a href=".">index</a><br><a href="file:%s">%s</a>%s</font></td></tr></table>
    <p><tt>This&nbsp;is&nbsp;a&nbsp;test&nbsp;module&nbsp;for&nbsp;test_pydoc</tt></p>
<p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ee77aa">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial"><big><strong>Classes</strong></big></font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#ee77aa"><tt>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%"><dl>
<dt><font face="helvetica, arial"><a href="__builtin__.html#object">__builtin__.object</a>
</font></dt><dd>
<dl>
<dt><font face="helvetica, arial"><a href="test.pydoc_mod.html#B">B</a>
</font></dt><dt><font face="helvetica, arial"><a href="test.pydoc_mod.html#C">C</a>
</font></dt></dl>
</dd>
<dt><font face="helvetica, arial"><a href="test.pydoc_mod.html#A">A</a>
</font></dt></dl>
 <p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ffc8d8">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#000000" face="helvetica, arial"><a name="A">class <strong>A</strong></a></font></td></tr>
\x20\x20\x20\x20
<tr bgcolor="#ffc8d8"><td rowspan=2><tt>&nbsp;&nbsp;&nbsp;</tt></td>
<td colspan=2><tt>Hello&nbsp;and&nbsp;goodbye<br>&nbsp;</tt></td></tr>
<tr><td>&nbsp;</td>
<td width="100%%">Methods defined here:<br>
<dl><dt><a name="A-__init__"><strong>__init__</strong></a>()</dt><dd><tt>Wow,&nbsp;I&nbsp;have&nbsp;no&nbsp;function!</tt></dd></dl>

</td></tr></table> <p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ffc8d8">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#000000" face="helvetica, arial"><a name="B">class <strong>B</strong></a>(<a href="__builtin__.html#object">__builtin__.object</a>)</font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#ffc8d8"><tt>&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%">Data descriptors defined here:<br>
<dl><dt><strong>__dict__</strong></dt>
<dd><tt>%s</tt></dd>
</dl>
<dl><dt><strong>__weakref__</strong></dt>
<dd><tt>%s</tt></dd>
</dl>
<hr>
Data and other attributes defined here:<br>
<dl><dt><strong>NO_MEANING</strong> = 'eggs'</dl>

</td></tr></table> <p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ffc8d8">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#000000" face="helvetica, arial"><a name="C">class <strong>C</strong></a>(<a href="__builtin__.html#object">__builtin__.object</a>)</font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#ffc8d8"><tt>&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%">Methods defined here:<br>
<dl><dt><a name="C-get_answer"><strong>get_answer</strong></a>(self)</dt><dd><tt>Return&nbsp;<a href="#C-say_no">say_no</a>()</tt></dd></dl>

<dl><dt><a name="C-is_it_true"><strong>is_it_true</strong></a>(self)</dt><dd><tt>Return&nbsp;self.<a href="#C-get_answer">get_answer</a>()</tt></dd></dl>

<dl><dt><a name="C-say_no"><strong>say_no</strong></a>(self)</dt></dl>

<hr>
Data descriptors defined here:<br>
<dl><dt><strong>__dict__</strong></dt>
<dd><tt>dictionary&nbsp;for&nbsp;instance&nbsp;variables&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
<dl><dt><strong>__weakref__</strong></dt>
<dd><tt>list&nbsp;of&nbsp;weak&nbsp;references&nbsp;to&nbsp;the&nbsp;object&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
</td></tr></table></td></tr></table><p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#eeaa77">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial"><big><strong>Functions</strong></big></font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#eeaa77"><tt>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%"><dl><dt><a name="-doc_func"><strong>doc_func</strong></a>()</dt><dd><tt>This&nbsp;function&nbsp;solves&nbsp;all&nbsp;of&nbsp;the&nbsp;world's&nbsp;problems:<br>
hunger<br>
lack&nbsp;of&nbsp;Python<br>
war</tt></dd></dl>
 <dl><dt><a name="-nodoc_func"><strong>nodoc_func</strong></a>()</dt></dl>
</td></tr></table><p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#55aa55">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial"><big><strong>Data</strong></big></font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#55aa55"><tt>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%"><strong>__author__</strong> = 'Benjamin Peterson'<br>
<strong>__credits__</strong> = 'Nobody'<br>
<strong>__version__</strong> = '1.2.3.4'</td></tr></table><p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#7799ee">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial"><big><strong>Author</strong></big></font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#7799ee"><tt>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%">Benjamin&nbsp;Peterson</td></tr></table><p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#7799ee">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial"><big><strong>Credits</strong></big></font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#7799ee"><tt>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%">Nobody</td></tr></table>
""".strip()

expected_html_data_docstrings = tuple(s.replace(' ', '&nbsp;')
                                      for s in expected_data_docstrings)

# output pattern for missing module
missing_pattern = "no Python documentation found for '%s'"

# output pattern for module with bad imports
badimport_pattern = "problem in %s - <type 'exceptions.ImportError'>: No module named %s"

def run_pydoc(module_name, *args, **env):
    """
    Runs pydoc on the specified module. Returns the stripped
    output of pydoc.
    """
    args = args + (module_name,)
    # do not write bytecode files to avoid caching errors
    rc, out, err = assert_python_ok('-B', pydoc.__file__, *args, **env)
    return out.strip()

def get_pydoc_html(module):
    "Returns pydoc generated output as html"
    doc = pydoc.HTMLDoc()
    output = doc.docmodule(module)
    loc = doc.getdocloc(pydoc_mod) or ""
    if loc:
        loc = "<br><a href=\"" + loc + "\">Module Docs</a>"
    return output.strip(), loc

def get_pydoc_link(module):
    "Returns a documentation web link of a module"
    dirname = os.path.dirname
    basedir = dirname(dirname(os.path.realpath(__file__)))
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

    # cleanup the extra text formatting that pydoc preforms
    patt = re.compile('\b.')
    output = patt.sub('', output)
    return output.strip(), loc

def print_diffs(text1, text2):
    "Prints unified diffs for two texts"
    lines1 = text1.splitlines(True)
    lines2 = text2.splitlines(True)
    diffs = difflib.unified_diff(lines1, lines2, n=0, fromfile='expected',
                                 tofile='got')
    print '\n' + ''.join(diffs)


class PydocBaseTest(unittest.TestCase):

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


class PydocDocTest(unittest.TestCase):

    @requires_docstrings
    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_html_doc(self):
        result, doc_loc = get_pydoc_html(pydoc_mod)
        mod_file = inspect.getabsfile(pydoc_mod)
        if sys.platform == 'win32':
            import nturl2path
            mod_url = nturl2path.pathname2url(mod_file)
        else:
            mod_url = mod_file
        expected_html = expected_html_pattern % (
                        (mod_url, mod_file, doc_loc) +
                        expected_html_data_docstrings)
        if result != expected_html:
            print_diffs(expected_html, result)
            self.fail("outputs are not equal, see diff above")

    @requires_docstrings
    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_text_doc(self):
        result, doc_loc = get_pydoc_text(pydoc_mod)
        expected_text = expected_text_pattern % (
                        (inspect.getabsfile(pydoc_mod), doc_loc) +
                        expected_text_data_docstrings)
        if result != expected_text:
            print_diffs(expected_text, result)
            self.fail("outputs are not equal, see diff above")

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
        result = run_pydoc(missing_module)
        expected = missing_pattern % missing_module
        self.assertEqual(expected, result,
            "documentation for missing module found")

    def test_input_strip(self):
        missing_module = " test.i_am_not_here "
        result = run_pydoc(missing_module)
        expected = missing_pattern % missing_module.strip()
        self.assertEqual(expected, result,
            "white space was not stripped from module name "
            "or other error output mismatch")

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

    def test_synopsis(self):
        with test.test_support.temp_cwd() as test_dir:
            init_path = os.path.join(test_dir, 'dt.py')
            with open(init_path, 'w') as fobj:
                fobj.write('''\
"""
my doc

second line
"""
foo = 1
''')
            py_compile.compile(init_path)
            synopsis = pydoc.synopsis(init_path, {})
            self.assertEqual(synopsis, 'my doc')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     'Docstrings are omitted with -OO and above')
    def test_synopsis_sourceless_empty_doc(self):
        with test.test_support.temp_cwd() as test_dir:
            init_path = os.path.join(test_dir, 'foomod42.py')
            cached_path = os.path.join(test_dir, 'foomod42.pyc')
            with open(init_path, 'w') as fobj:
                fobj.write("foo = 1")
            py_compile.compile(init_path)
            synopsis = pydoc.synopsis(init_path, {})
            self.assertIsNone(synopsis)
            synopsis_cached = pydoc.synopsis(cached_path, {})
            self.assertIsNone(synopsis_cached)


class PydocImportTest(PydocBaseTest):

    def setUp(self):
        self.test_dir = os.mkdir(TESTFN)
        self.addCleanup(rmtree, TESTFN)

    def test_badimport(self):
        # This tests the fix for issue 5230, where if pydoc found the module
        # but the module had an internal import error pydoc would report no doc
        # found.
        modname = 'testmod_xyzzy'
        testpairs = (
            ('i_am_not_here', 'i_am_not_here'),
            ('test.i_am_not_here_either', 'i_am_not_here_either'),
            ('test.i_am_not_here.neither_am_i', 'i_am_not_here.neither_am_i'),
            ('i_am_not_here.{}'.format(modname),
             'i_am_not_here.{}'.format(modname)),
            ('test.{}'.format(modname), modname),
            )

        sourcefn = os.path.join(TESTFN, modname) + os.extsep + "py"
        for importstring, expectedinmsg in testpairs:
            with open(sourcefn, 'w') as f:
                f.write("import {}\n".format(importstring))
            result = run_pydoc(modname, PYTHONPATH=TESTFN)
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


class TestDescriptions(unittest.TestCase):

    def test_module(self):
        # Check that pydocfodder module can be described
        from test import pydocfodder
        doc = pydoc.render_doc(pydocfodder)
        self.assertIn("pydocfodder", doc)

    def test_classic_class(self):
        class C: "Classic class"
        c = C()
        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'instance of C')
        expected = 'instance of C in module %s' % __name__
        self.assertIn(expected, pydoc.render_doc(c))

    def test_class(self):
        class C(object): "New-style class"
        c = C()

        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'C')
        expected = 'C in module %s object' % __name__
        self.assertIn(expected, pydoc.render_doc(c))

    def test_namedtuple_public_underscore(self):
        NT = namedtuple('NT', ['abc', 'def'], rename=True)
        with captured_stdout() as help_io:
            pydoc.help(NT)
        helptext = help_io.getvalue()
        self.assertIn('_1', helptext)
        self.assertIn('_replace', helptext)
        self.assertIn('_asdict', helptext)


@unittest.skipUnless(test.test_support.have_unicode,
                     "test requires unicode support")
class TestUnicode(unittest.TestCase):

    def setUp(self):
        # Better not to use unicode escapes in literals, lest the
        # parser choke on it if Python has been built without
        # unicode support.
        self.Q  = types.ModuleType(
            'Q', 'Rational numbers: \xe2\x84\x9a'.decode('utf8'))
        self.Q.__version__ = '\xe2\x84\x9a'.decode('utf8')
        self.Q.__date__ = '\xe2\x84\x9a'.decode('utf8')
        self.Q.__author__ = '\xe2\x84\x9a'.decode('utf8')
        self.Q.__credits__ = '\xe2\x84\x9a'.decode('utf8')

        self.assertIsInstance(self.Q.__doc__, unicode)

    def test_render_doc(self):
        # render_doc is robust against unicode in docstrings
        doc = pydoc.render_doc(self.Q)
        self.assertIsInstance(doc, str)

    def test_encode(self):
        # _encode is robust against characters out the specified encoding
        self.assertEqual(pydoc._encode(self.Q.__doc__, 'ascii'), 'Rational numbers: &#8474;')

    def test_pipepager(self):
        # pipepager does not choke on unicode
        doc = pydoc.render_doc(self.Q)

        saved, os.popen = os.popen, open
        try:
            with test.test_support.temp_cwd():
                pydoc.pipepager(doc, 'pipe')
                self.assertEqual(open('pipe').read(), pydoc._encode(doc))
        finally:
            os.popen = saved

    def test_tempfilepager(self):
        # tempfilepager does not choke on unicode
        doc = pydoc.render_doc(self.Q)

        output = {}
        def mock_system(cmd):
            filename = cmd.strip()[1:-1]
            self.assertEqual('"' + filename + '"', cmd.strip())
            output['content'] = open(filename).read()
        saved, os.system = os.system, mock_system
        try:
            pydoc.tempfilepager(doc, '')
            self.assertEqual(output['content'], pydoc._encode(doc))
        finally:
            os.system = saved

    def test_plainpager(self):
        # plainpager does not choke on unicode
        doc = pydoc.render_doc(self.Q)

        # Note: captured_stdout is too permissive when it comes to
        # unicode, and using it here would make the test always
        # pass.
        with test.test_support.temp_cwd():
            with open('output', 'w') as f:
                saved, sys.stdout = sys.stdout, f
                try:
                    pydoc.plainpager(doc)
                finally:
                    sys.stdout = saved
            self.assertIn('Rational numbers:', open('output').read())

    def test_ttypager(self):
        # ttypager does not choke on unicode
        doc = pydoc.render_doc(self.Q)
        # Test ttypager
        with test.test_support.temp_cwd(), test.test_support.captured_stdin():
            with open('output', 'w') as f:
                saved, sys.stdout = sys.stdout, f
                try:
                    pydoc.ttypager(doc)
                finally:
                    sys.stdout = saved
            self.assertIn('Rational numbers:', open('output').read())

    def test_htmlpage(self):
        # html.page does not choke on unicode
        with test.test_support.temp_cwd():
            with captured_stdout() as output:
                pydoc.writedoc(self.Q)
        self.assertEqual(output.getvalue(), 'wrote Q.html\n')

class TestHelper(unittest.TestCase):
    def test_keywords(self):
        self.assertEqual(sorted(pydoc.Helper.keywords),
                         sorted(keyword.kwlist))

    def test_builtin(self):
        for name in ('str', 'str.translate', '__builtin__.str',
                     '__builtin__.str.translate'):
            # test low-level function
            self.assertIsNotNone(pydoc.locate(name))
            # test high-level function
            try:
                pydoc.render_doc(name)
            except ImportError:
                self.fail('finding the doc of {!r} failed'.format(name))

        for name in ('not__builtin__', 'strrr', 'strr.translate',
                     'str.trrrranslate', '__builtin__.strrr',
                     '__builtin__.str.trrranslate'):
            self.assertIsNone(pydoc.locate(name))
            self.assertRaises(ImportError, pydoc.render_doc, name)


def test_main():
    try:
        test.test_support.run_unittest(PydocDocTest,
                                       PydocImportTest,
                                       TestDescriptions,
                                       TestUnicode,
                                       TestHelper)
    finally:
        reap_children()

if __name__ == "__main__":
    test_main()
