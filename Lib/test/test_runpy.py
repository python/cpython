# Test the runpy module
import unittest
import os
import os.path
import sys
import tempfile
from test.test_support import verbose, run_unittest
from runpy import _run_module_code, run_module

# Set up the test code and expected results

class RunModuleCodeTest(unittest.TestCase):

    expected_result = ["Top level assignment", "Lower level reference"]
    test_source = (
        "# Check basic code execution\n"
        "result = ['Top level assignment']\n"
        "def f():\n"
        "    result.append('Lower level reference')\n"
        "f()\n"
        "# Check the sys module\n"
        "import sys\n"
        "run_argv0 = sys.argv[0]\n"
        "run_name_in_sys_modules = __name__ in sys.modules\n"
        "if run_name_in_sys_modules:\n"
        "   module_in_sys_modules = globals() is sys.modules[__name__].__dict__\n"
        "# Check nested operation\n"
        "import runpy\n"
        "nested = runpy._run_module_code('x=1\\n', mod_name='<run>',\n"
        "                                          alter_sys=True)\n"
    )


    def test_run_module_code(self):
        initial = object()
        name = "<Nonsense>"
        file = "Some other nonsense"
        loader = "Now you're just being silly"
        d1 = dict(initial=initial)
        saved_argv0 = sys.argv[0]
        d2 = _run_module_code(self.test_source,
                              d1,
                              name,
                              file,
                              loader,
                              True)
        self.failUnless("result" not in d1)
        self.failUnless(d2["initial"] is initial)
        self.failUnless(d2["result"] == self.expected_result)
        self.failUnless(d2["nested"]["x"] == 1)
        self.failUnless(d2["__name__"] is name)
        self.failUnless(d2["run_name_in_sys_modules"])
        self.failUnless(d2["module_in_sys_modules"])
        self.failUnless(d2["__file__"] is file)
        self.failUnless(d2["run_argv0"] is file)
        self.failUnless(d2["__loader__"] is loader)
        self.failUnless(sys.argv[0] is saved_argv0)
        self.failUnless(name not in sys.modules)

    def test_run_module_code_defaults(self):
        saved_argv0 = sys.argv[0]
        d = _run_module_code(self.test_source)
        self.failUnless(d["result"] == self.expected_result)
        self.failUnless(d["__name__"] is None)
        self.failUnless(d["__file__"] is None)
        self.failUnless(d["__loader__"] is None)
        self.failUnless(d["run_argv0"] is saved_argv0)
        self.failUnless("run_name" not in d)
        self.failUnless(sys.argv[0] is saved_argv0)

class RunModuleTest(unittest.TestCase):

    def expect_import_error(self, mod_name):
        try:
            run_module(mod_name)
        except ImportError:
            pass
        else:
            self.fail("Expected import error for " + mod_name)

    def test_invalid_names(self):
        # Builtin module
        self.expect_import_error("sys")
        # Non-existent modules
        self.expect_import_error("sys.imp.eric")
        self.expect_import_error("os.path.half")
        self.expect_import_error("a.bee")
        self.expect_import_error(".howard")
        self.expect_import_error("..eaten")
        # Package
        self.expect_import_error("logging")

    def test_library_module(self):
        run_module("runpy")

    def _add_pkg_dir(self, pkg_dir):
        os.mkdir(pkg_dir)
        pkg_fname = os.path.join(pkg_dir, "__init__"+os.extsep+"py")
        pkg_file = open(pkg_fname, "w")
        pkg_file.close()
        return pkg_fname

    def _make_pkg(self, source, depth):
        pkg_name = "__runpy_pkg__"
        test_fname = "runpy_test"+os.extsep+"py"
        pkg_dir = sub_dir = tempfile.mkdtemp()
        if verbose: print "  Package tree in:", sub_dir
        sys.path.insert(0, pkg_dir)
        if verbose: print "  Updated sys.path:", sys.path[0]
        for i in range(depth):
            sub_dir = os.path.join(sub_dir, pkg_name)
            pkg_fname = self._add_pkg_dir(sub_dir)
            if verbose: print "  Next level in:", sub_dir
            if verbose: print "  Created:", pkg_fname
        mod_fname = os.path.join(sub_dir, test_fname)
        mod_file = open(mod_fname, "w")
        mod_file.write(source)
        mod_file.close()
        if verbose: print "  Created:", mod_fname
        mod_name = (pkg_name+".")*depth + "runpy_test"
        return pkg_dir, mod_fname, mod_name

    def _del_pkg(self, top, depth, mod_name):
        for entry in list(sys.modules):
            if entry.startswith("__runpy_pkg__"):
                del sys.modules[entry]
        if verbose: print "  Removed sys.modules entries"
        del sys.path[0]
        if verbose: print "  Removed sys.path entry"
        for root, dirs, files in os.walk(top, topdown=False):
            for name in files:
                try:
                    os.remove(os.path.join(root, name))
                except OSError, ex:
                    if verbose: print ex # Persist with cleaning up
            for name in dirs:
                fullname = os.path.join(root, name)
                try:
                    os.rmdir(fullname)
                except OSError, ex:
                    if verbose: print ex # Persist with cleaning up
        try:
            os.rmdir(top)
            if verbose: print "  Removed package tree"
        except OSError, ex:
            if verbose: print ex # Persist with cleaning up

    def _check_module(self, depth):
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg("x=1\n", depth))
        try:
            if verbose: print "Running from source:", mod_name
            d1 = run_module(mod_name) # Read from source
            self.failUnless("x" in d1)
            self.failUnless(d1["x"] == 1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            __import__(mod_name)
            os.remove(mod_fname)
            if verbose: print "Running from compiled:", mod_name
            d2 = run_module(mod_name) # Read from bytecode
            self.failUnless("x" in d2)
            self.failUnless(d2["x"] == 1)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose: print "Module executed successfully"

    def _add_relative_modules(self, base_dir, depth):
        if depth <= 1:
            raise ValueError("Relative module test needs depth > 1")
        pkg_name = "__runpy_pkg__"
        module_dir = base_dir
        for i in range(depth):
            parent_dir = module_dir
            module_dir = os.path.join(module_dir, pkg_name)
        # Add sibling module
        sibling_fname = os.path.join(module_dir, "sibling"+os.extsep+"py")
        sibling_file = open(sibling_fname, "w")
        sibling_file.close()
        if verbose: print "  Added sibling module:", sibling_fname
        # Add nephew module
        uncle_dir = os.path.join(parent_dir, "uncle")
        self._add_pkg_dir(uncle_dir)
        if verbose: print "  Added uncle package:", uncle_dir
        cousin_dir = os.path.join(uncle_dir, "cousin")
        self._add_pkg_dir(cousin_dir)
        if verbose: print "  Added cousin package:", cousin_dir
        nephew_fname = os.path.join(cousin_dir, "nephew"+os.extsep+"py")
        nephew_file = open(nephew_fname, "w")
        nephew_file.close()
        if verbose: print "  Added nephew module:", nephew_fname

    def _check_relative_imports(self, depth, run_name=None):
        contents = """\
from __future__ import absolute_import
from . import sibling
from ..uncle.cousin import nephew
"""
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(contents, depth))
        try:
            self._add_relative_modules(pkg_dir, depth)
            if verbose: print "Running from source:", mod_name
            d1 = run_module(mod_name) # Read from source
            self.failUnless("sibling" in d1)
            self.failUnless("nephew" in d1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            __import__(mod_name)
            os.remove(mod_fname)
            if verbose: print "Running from compiled:", mod_name
            d2 = run_module(mod_name) # Read from bytecode
            self.failUnless("sibling" in d2)
            self.failUnless("nephew" in d2)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose: print "Module executed successfully"

    def test_run_module(self):
        for depth in range(4):
            if verbose: print "Testing package depth:", depth
            self._check_module(depth)

    def test_explicit_relative_import(self):
        for depth in range(2, 5):
            if verbose: print "Testing relative imports at depth:", depth
            self._check_relative_imports(depth)


def test_main():
    run_unittest(RunModuleCodeTest)
    run_unittest(RunModuleTest)

if __name__ == "__main__":
    test_main()
