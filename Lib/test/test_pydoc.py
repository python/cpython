import os
import sys
import difflib
import inspect
import pydoc
import re
import string
import subprocess
import test.support
import time
import unittest
import xml.etree
import textwrap
from io import StringIO
from contextlib import contextmanager
from test.support import TESTFN, forget, rmtree, EnvironmentVarGuard, \
     reap_children, captured_output

from test import pydoc_mod

# Just in case sys.modules["test"] has the optional attribute __loader__.
if hasattr(pydoc_mod, "__loader__"):
    del pydoc_mod.__loader__

expected_text_pattern = """
NAME
    test.pydoc_mod - This is a test module for test_pydoc
%s
CLASSES
    builtins.object
        A
        B
\x20\x20\x20\x20
    class A(builtins.object)
     |  Hello and goodbye
     |\x20\x20
     |  Methods defined here:
     |\x20\x20
     |  __init__()
     |      Wow, I have no function!
     |\x20\x20
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |\x20\x20
     |  __dict__
     |      dictionary for instance variables (if defined)
     |\x20\x20
     |  __weakref__
     |      list of weak references to the object (if defined)
\x20\x20\x20\x20
    class B(builtins.object)
     |  Data descriptors defined here:
     |\x20\x20
     |  __dict__
     |      dictionary for instance variables (if defined)
     |\x20\x20
     |  __weakref__
     |      list of weak references to the object (if defined)
     |\x20\x20
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |\x20\x20
     |  NO_MEANING = 'eggs'

FUNCTIONS
    doc_func()
        This function solves all of the world's problems:
        hunger
        lack of Python
        war
\x20\x20\x20\x20
    nodoc_func()

DATA
    __xyz__ = 'X, Y and Z'

VERSION
    1.2.3.4

AUTHOR
    Benjamin Peterson

CREDITS
    Nobody

FILE
    %s
""".strip()

expected_html_pattern = """
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
<dt><font face="helvetica, arial"><a href="builtins.html#object">builtins.object</a>
</font></dt><dd>
<dl>
<dt><font face="helvetica, arial"><a href="test.pydoc_mod.html#A">A</a>
</font></dt><dt><font face="helvetica, arial"><a href="test.pydoc_mod.html#B">B</a>
</font></dt></dl>
</dd>
</dl>
 <p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ffc8d8">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#000000" face="helvetica, arial"><a name="A">class <strong>A</strong></a>(<a href="builtins.html#object">builtins.object</a>)</font></td></tr>
\x20\x20\x20\x20
<tr bgcolor="#ffc8d8"><td rowspan=2><tt>&nbsp;&nbsp;&nbsp;</tt></td>
<td colspan=2><tt>Hello&nbsp;and&nbsp;goodbye<br>&nbsp;</tt></td></tr>
<tr><td>&nbsp;</td>
<td width="100%%">Methods defined here:<br>
<dl><dt><a name="A-__init__"><strong>__init__</strong></a>()</dt><dd><tt>Wow,&nbsp;I&nbsp;have&nbsp;no&nbsp;function!</tt></dd></dl>

<hr>
Data descriptors defined here:<br>
<dl><dt><strong>__dict__</strong></dt>
<dd><tt>dictionary&nbsp;for&nbsp;instance&nbsp;variables&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
<dl><dt><strong>__weakref__</strong></dt>
<dd><tt>list&nbsp;of&nbsp;weak&nbsp;references&nbsp;to&nbsp;the&nbsp;object&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
</td></tr></table> <p>
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="section">
<tr bgcolor="#ffc8d8">
<td colspan=3 valign=bottom>&nbsp;<br>
<font color="#000000" face="helvetica, arial"><a name="B">class <strong>B</strong></a>(<a href="builtins.html#object">builtins.object</a>)</font></td></tr>
\x20\x20\x20\x20
<tr><td bgcolor="#ffc8d8"><tt>&nbsp;&nbsp;&nbsp;</tt></td><td>&nbsp;</td>
<td width="100%%">Data descriptors defined here:<br>
<dl><dt><strong>__dict__</strong></dt>
<dd><tt>dictionary&nbsp;for&nbsp;instance&nbsp;variables&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
<dl><dt><strong>__weakref__</strong></dt>
<dd><tt>list&nbsp;of&nbsp;weak&nbsp;references&nbsp;to&nbsp;the&nbsp;object&nbsp;(if&nbsp;defined)</tt></dd>
</dl>
<hr>
Data and other attributes defined here:<br>
<dl><dt><strong>NO_MEANING</strong> = 'eggs'</dl>

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
<td width="100%%"><strong>__xyz__</strong> = 'X, Y and Z'</td></tr></table><p>
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
""".strip() # ' <- emacs turd


# output pattern for missing module
missing_pattern = "no Python documentation found for '%s'"

# output pattern for module with bad imports
badimport_pattern = "problem in %s - ImportError: No module named %r"

def run_pydoc(module_name, *args):
    """
    Runs pydoc on the specified module. Returns the stripped
    output of pydoc.
    """
    cmd = [sys.executable, pydoc.__file__, " ".join(args), module_name]
    try:
        output = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        return output.strip()
    finally:
        reap_children()

def get_pydoc_html(module):
    "Returns pydoc generated output as html"
    doc = pydoc.HTMLDoc()
    output = doc.docmodule(module)
    loc = doc.getdocloc(pydoc_mod) or ""
    if loc:
        loc = "<br><a href=\"" + loc + "\">Module Docs</a>"
    return output.strip(), loc

def get_pydoc_text(module):
    "Returns pydoc generated output as text"
    doc = pydoc.TextDoc()
    loc = doc.getdocloc(pydoc_mod) or ""
    if loc:
        loc = "\nMODULE DOCS\n    " + loc + "\n"

    output = doc.docmodule(module)

    # clean up the extra text formatting that pydoc performs
    patt = re.compile('\b.')
    output = patt.sub('', output)
    return output.strip(), loc

def print_diffs(text1, text2):
    "Prints unified diffs for two texts"
    # XXX now obsolete, use unittest built-in support
    lines1 = text1.splitlines(True)
    lines2 = text2.splitlines(True)
    diffs = difflib.unified_diff(lines1, lines2, n=0, fromfile='expected',
                                 tofile='got')
    print('\n' + ''.join(diffs))

def get_html_title(text):
    # Bit of hack, but good enough for test purposes
    header, _, _ = text.partition("</head>")
    _, _, title = header.partition("<title>")
    title, _, _ = title.partition("</title>")
    return title


class PydocDocTest(unittest.TestCase):

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    def test_html_doc(self):
        result, doc_loc = get_pydoc_html(pydoc_mod)
        mod_file = inspect.getabsfile(pydoc_mod)
        if sys.platform == 'win32':
            import nturl2path
            mod_url = nturl2path.pathname2url(mod_file)
        else:
            mod_url = mod_file
        expected_html = expected_html_pattern % (mod_url, mod_file, doc_loc)
        if result != expected_html:
            print_diffs(expected_html, result)
            self.fail("outputs are not equal, see diff above")

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    def test_text_doc(self):
        result, doc_loc = get_pydoc_text(pydoc_mod)
        expected_text = expected_text_pattern % \
                        (doc_loc, inspect.getabsfile(pydoc_mod))
        if result != expected_text:
            print_diffs(expected_text, result)
            self.fail("outputs are not equal, see diff above")

    def test_issue8225(self):
        # Test issue8225 to ensure no doc link appears for xml.etree
        result, doc_loc = get_pydoc_text(xml.etree)
        self.assertEqual(doc_loc, "", "MODULE DOCS incorrectly includes a link")

    def test_not_here(self):
        missing_module = "test.i_am_not_here"
        result = str(run_pydoc(missing_module), 'ascii')
        expected = missing_pattern % missing_module
        self.assertEqual(expected, result,
            "documentation for missing module found")

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

        @contextmanager
        def newdirinpath(dir):
            os.mkdir(dir)
            sys.path.insert(0, dir)
            yield
            sys.path.pop(0)
            rmtree(dir)

        with newdirinpath(TESTFN), EnvironmentVarGuard() as env:
            env['PYTHONPATH'] = TESTFN
            fullmodname = os.path.join(TESTFN, modname)
            sourcefn = fullmodname + os.extsep + "py"
            for importstring, expectedinmsg in testpairs:
                with open(sourcefn, 'w') as f:
                    f.write("import {}\n".format(importstring))
                try:
                    result = run_pydoc(modname).decode("ascii")
                finally:
                    forget(modname)
                expected = badimport_pattern % (modname, expectedinmsg)
                self.assertEqual(expected, result)

    def test_input_strip(self):
        missing_module = " test.i_am_not_here "
        result = str(run_pydoc(missing_module), 'ascii')
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

    @unittest.skipIf(sys.flags.optimize >= 2,
                     'Docstrings are omitted with -O2 and above')
    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'trace function introduces __locals__ unexpectedly')
    def test_help_output_redirect(self):
        # issue 940286, if output is set in Helper, then all output from
        # Helper.help should be redirected
        old_pattern = expected_text_pattern
        getpager_old = pydoc.getpager
        getpager_new = lambda: (lambda x: x)
        self.maxDiff = None

        buf = StringIO()
        helper = pydoc.Helper(output=buf)
        unused, doc_loc = get_pydoc_text(pydoc_mod)
        module = "test.pydoc_mod"
        help_header = """
        Help on module test.pydoc_mod in test:

        """.lstrip()
        help_header = textwrap.dedent(help_header)
        expected_help_pattern = help_header + expected_text_pattern

        pydoc.getpager = getpager_new
        try:
            with captured_output('stdout') as output, \
                 captured_output('stderr') as err:
                helper.help(module)
                result = buf.getvalue().strip()
                expected_text = expected_help_pattern % \
                                (doc_loc, inspect.getabsfile(pydoc_mod))
                self.assertEqual('', output.getvalue())
                self.assertEqual('', err.getvalue())
                self.assertEqual(expected_text, result)
        finally:
            pydoc.getpager = getpager_old


class TestDescriptions(unittest.TestCase):

    def test_module(self):
        # Check that pydocfodder module can be described
        from test import pydocfodder
        doc = pydoc.render_doc(pydocfodder)
        self.assertIn("pydocfodder", doc)

    def test_class(self):
        class C: "New-style class"
        c = C()

        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'C')
        expected = 'C in module %s object' % __name__
        self.assertIn(expected, pydoc.render_doc(c))


class PydocServerTest(unittest.TestCase):
    """Tests for pydoc._start_server"""

    def test_server(self):

        # Minimal test that starts the server, then stops it.
        def my_url_handler(url, content_type):
            text = 'the URL sent was: (%s, %s)' % (url, content_type)
            return text

        serverthread = pydoc._start_server(my_url_handler, port=0)
        starttime = time.time()
        timeout = 1  #seconds

        while serverthread.serving:
            time.sleep(.01)
            if serverthread.serving and time.time() - starttime > timeout:
                serverthread.stop()
                break

        self.assertEqual(serverthread.error, None)


class PydocUrlHandlerTest(unittest.TestCase):
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
            ("getfile?key=foobar", "Pydoc: Error - getfile?key=foobar"),
            ]

        for url, title in requests:
            text = pydoc._url_handler(url, "text/html")
            result = get_html_title(text)
            self.assertEqual(result, title)

        path = string.__file__
        title = "Pydoc: getfile " + path
        url = "getfile?key=" + path
        text = pydoc._url_handler(url, "text/html")
        result = get_html_title(text)
        self.assertEqual(result, title)


def test_main():
    test.support.run_unittest(PydocDocTest,
                              TestDescriptions,
                              PydocServerTest,
                              PydocUrlHandlerTest,
                              )

if __name__ == "__main__":
    test_main()
