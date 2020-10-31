'''Test Tools/scripts/fixcid.py.'''

from io import StringIO
import os, os.path
import runpy
import sys
from test import support
from test.support import os_helper
from test.test_tools import skip_if_missing, scriptsdir
import unittest

skip_if_missing()

class Test(unittest.TestCase):
    def test_parse_strings(self):
        old1 = 'int xx = "xx\\"xx"[xx];\n'
        old2 = "int xx = 'x\\'xx' + xx;\n"
        output = self.run_script(old1 + old2)
        new1 = 'int yy = "xx\\"xx"[yy];\n'
        new2 = "int yy = 'x\\'xx' + yy;\n"
        self.assertMultiLineEqual(output,
            "1\n"
            "< {old1}"
            "> {new1}"
            "{new1}"
            "2\n"
            "< {old2}"
            "> {new2}"
            "{new2}".format(old1=old1, old2=old2, new1=new1, new2=new2)
        )

    def test_alter_comments(self):
        output = self.run_script(
            substfile=
                "xx yy\n"
                "*aa bb\n",
            args=("-c", "-",),
            input=
                "/* xx altered */\n"
                "int xx;\n"
                "/* aa unaltered */\n"
                "int aa;\n",
        )
        self.assertMultiLineEqual(output,
            "1\n"
            "< /* xx altered */\n"
            "> /* yy altered */\n"
            "/* yy altered */\n"
            "2\n"
            "< int xx;\n"
            "> int yy;\n"
            "int yy;\n"
            "/* aa unaltered */\n"
            "4\n"
            "< int aa;\n"
            "> int bb;\n"
            "int bb;\n"
        )

    def test_directory(self):
        os.mkdir(os_helper.TESTFN)
        self.addCleanup(os_helper.rmtree, os_helper.TESTFN)
        c_filename = os.path.join(os_helper.TESTFN, "file.c")
        with open(c_filename, "w") as file:
            file.write("int xx;\n")
        with open(os.path.join(os_helper.TESTFN, "file.py"), "w") as file:
            file.write("xx = 'unaltered'\n")
        script = os.path.join(scriptsdir, "fixcid.py")
        output = self.run_script(args=(os_helper.TESTFN,))
        self.assertMultiLineEqual(output,
            "{}:\n"
            "1\n"
            '< int xx;\n'
            '> int yy;\n'.format(c_filename)
        )

    def run_script(self, input="", *, args=("-",), substfile="xx yy\n"):
        substfilename = os_helper.TESTFN + ".subst"
        with open(substfilename, "w") as file:
            file.write(substfile)
        self.addCleanup(os_helper.unlink, substfilename)

        argv = ["fixcid.py", "-s", substfilename] + list(args)
        script = os.path.join(scriptsdir, "fixcid.py")
        with support.swap_attr(sys, "argv", argv), \
                support.swap_attr(sys, "stdin", StringIO(input)), \
                support.captured_stdout() as output, \
                support.captured_stderr():
            try:
                runpy.run_path(script, run_name="__main__")
            except SystemExit as exit:
                self.assertEqual(exit.code, 0)
        return output.getvalue()
