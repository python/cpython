import sys
import difflib
import subprocess
import re
import pydoc
import unittest
import test.test_support

from test import pydoc_mod

expected_text_pattern = \
"""
NAME
    test.pydoc_mod - This is a test module for test_pydoc

FILE
    %s

CLASSES
    __builtin__.object
        B
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
    __author__ = 'Benjamin Peterson'
    __credits__ = 'Nobody'
    __package__ = None
    __version__ = '1.2.3.4'

VERSION
    1.2.3.4

AUTHOR
    Benjamin Peterson

CREDITS
    Nobody
""".strip()

expected_html_pattern = \
"""
<table width="100%%" cellspacing=0 cellpadding=2 border=0 summary="heading">
<tr bgcolor="#7799ee">
<td valign=bottom>&nbsp;<br>
<font color="#ffffff" face="helvetica, arial">&nbsp;<br><big><big><strong><a href="test.html"><font color="#ffffff">test</font></a>.pydoc_mod</strong></big></big> (version 1.2.3.4)</font></td
><td align=right valign=bottom
><font color="#ffffff" face="helvetica, arial"><a href=".">index</a><br><a href="file:%s">%s</a></font></td></tr></table>
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
<td width="100%%"><strong>__author__</strong> = 'Benjamin Peterson'<br>
<strong>__credits__</strong> = 'Nobody'<br>
<strong>__package__</strong> = None<br>
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

# output pattern for missing module
missing_pattern = "no Python documentation found for '%s'"

def run_pydoc(module_name, *args):
    """
    Runs pydoc on the specified module. Returns the stripped
    output of pydoc.
    """
    cmd = [sys.executable, pydoc.__file__, " ".join(args), module_name]
    output = subprocess.Popen(cmd, stdout=subprocess.PIPE).stdout.read()
    return output.strip()

def get_pydoc_html(module):
    "Returns pydoc generated output as html"
    output = pydoc.HTMLDoc().docmodule(module)
    return output.strip()

def get_pydoc_text(module):
    "Returns pydoc generated output as text"
    output = pydoc.TextDoc().docmodule(module)

    # cleanup the extra text formatting that pydoc preforms
    patt = re.compile('\b.')
    output = patt.sub('', output)
    return output.strip()

def print_diffs(text1, text2):
    "Prints unified diffs for two texts"
    lines1 = text1.splitlines(True)
    lines2 = text2.splitlines(True)
    diffs = difflib.unified_diff(lines1, lines2, n=0, fromfile='expected',
                                 tofile='got')
    print '\n' + ''.join(diffs)

def get_mod_file(module):
    """Returns the name of the .py file if the module is compiled"""
    mod_file = module.__file__
    return mod_file if mod_file.endswith("py") else mod_file[:-3] + "py"


class PyDocDocTest(unittest.TestCase):

    def test_html_doc(self):
        result = get_pydoc_html(pydoc_mod)
        mod_file = get_mod_file(pydoc_mod)
        expected_html = expected_html_pattern % (mod_file, mod_file)
        if result != expected_html:
            print_diffs(expected_html, result)
            self.fail("outputs are not equal, see diff above")

    def test_text_doc(self):
        result = get_pydoc_text(pydoc_mod)
        expected_text = expected_text_pattern % get_mod_file(pydoc_mod)
        if result != expected_text:
            print_diffs(expected_text, result)
            self.fail("outputs are not equal, see diff above")

    def test_not_here(self):
        missing_module = "test.i_am_not_here"
        result = run_pydoc(missing_module)
        expected = missing_pattern % missing_module
        self.assertEqual(expected, result,
            "documentation for missing module found")


class TestDescriptions(unittest.TestCase):

    def test_module(self):
        # Check that pydocfodder module can be described
        from test import pydocfodder
        doc = pydoc.render_doc(pydocfodder)
        self.assert_("pydocfodder" in doc)

    def test_classic_class(self):
        class C: "Classic class"
        c = C()
        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'instance of C')
        self.assert_('instance of C in module test.test_pydoc'
                        in pydoc.render_doc(c))

    def test_class(self):
        class C(object): "New-style class"
        c = C()

        self.assertEqual(pydoc.describe(C), 'class C')
        self.assertEqual(pydoc.describe(c), 'C')
        self.assert_('C in module test.test_pydoc object'
                        in pydoc.render_doc(c))


def test_main():
    test.test_support.run_unittest(PyDocDocTest,
                                   TestDescriptions)

if __name__ == "__main__":
    test_main()
