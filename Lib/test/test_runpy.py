# Test the runpy module
import unittest
import os
import os.path
import sys
import re
import tempfile
from test.test_support import verbose, run_unittest, forget
from test.script_helper import (temp_dir, make_script, compile_script,
                                make_pkg, make_zip_script, make_zip_pkg)


from runpy import _run_code, _run_module_code, run_module, run_path
# Note: This module can't safely test _run_module_as_main as it
# runs its tests in the current process, which would mess with the
# real __main__ module (usually test.regrtest)
# See test_cmd_line_script for a test that executes that code path

# Set up the test code and expected results

class RunModuleCodeTest(unittest.TestCase):
    """Unit tests for runpy._run_code and runpy._run_module_code"""

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
        "nested = runpy._run_module_code('x=1\\n', mod_name='<run>')\n"
    )

    def test_run_code(self):
        saved_argv0 = sys.argv[0]
        d = _run_code(self.test_source, {})
        self.assertEqual(d["result"], self.expected_result)
        self.assertIs(d["__name__"], None)
        self.assertIs(d["__file__"], None)
        self.assertIs(d["__loader__"], None)
        self.assertIs(d["__package__"], None)
        self.assertIs(d["run_argv0"], saved_argv0)
        self.assertNotIn("run_name", d)
        self.assertIs(sys.argv[0], saved_argv0)

    def test_run_module_code(self):
        initial = object()
        name = "<Nonsense>"
        file = "Some other nonsense"
        loader = "Now you're just being silly"
        package = '' # Treat as a top level module
        d1 = dict(initial=initial)
        saved_argv0 = sys.argv[0]
        d2 = _run_module_code(self.test_source,
                              d1,
                              name,
                              file,
                              loader,
                              package)
        self.assertNotIn("result", d1)
        self.assertIs(d2["initial"], initial)
        self.assertEqual(d2["result"], self.expected_result)
        self.assertEqual(d2["nested"]["x"], 1)
        self.assertIs(d2["__name__"], name)
        self.assertTrue(d2["run_name_in_sys_modules"])
        self.assertTrue(d2["module_in_sys_modules"])
        self.assertIs(d2["__file__"], file)
        self.assertIs(d2["run_argv0"], file)
        self.assertIs(d2["__loader__"], loader)
        self.assertIs(d2["__package__"], package)
        self.assertIs(sys.argv[0], saved_argv0)
        self.assertNotIn(name, sys.modules)


class RunModuleTest(unittest.TestCase):
    """Unit tests for runpy.run_module"""

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
        # Package without __main__.py
        self.expect_import_error("multiprocessing")

    def test_library_module(self):
        run_module("runpy")

    def _add_pkg_dir(self, pkg_dir):
        os.mkdir(pkg_dir)
        pkg_fname = os.path.join(pkg_dir, "__init__"+os.extsep+"py")
        pkg_file = open(pkg_fname, "w")
        pkg_file.close()
        return pkg_fname

    def _make_pkg(self, source, depth, mod_base="runpy_test"):
        pkg_name = "__runpy_pkg__"
        test_fname = mod_base+os.extsep+"py"
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
        mod_name = (pkg_name+".")*depth + mod_base
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
        forget(mod_name)
        try:
            if verbose: print "Running from source:", mod_name
            d1 = run_module(mod_name) # Read from source
            self.assertIn("x", d1)
            self.assertTrue(d1["x"] == 1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            __import__(mod_name)
            os.remove(mod_fname)
            if verbose: print "Running from compiled:", mod_name
            d2 = run_module(mod_name) # Read from bytecode
            self.assertIn("x", d2)
            self.assertTrue(d2["x"] == 1)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose: print "Module executed successfully"

    def _check_package(self, depth):
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg("x=1\n", depth, "__main__"))
        pkg_name, _, _ = mod_name.rpartition(".")
        forget(mod_name)
        try:
            if verbose: print "Running from source:", pkg_name
            d1 = run_module(pkg_name) # Read from source
            self.assertIn("x", d1)
            self.assertTrue(d1["x"] == 1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            __import__(mod_name)
            os.remove(mod_fname)
            if verbose: print "Running from compiled:", pkg_name
            d2 = run_module(pkg_name) # Read from bytecode
            self.assertIn("x", d2)
            self.assertTrue(d2["x"] == 1)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, pkg_name)
        if verbose: print "Package executed successfully"

    def _add_relative_modules(self, base_dir, source, depth):
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
        contents = r"""\
from __future__ import absolute_import
from . import sibling
from ..uncle.cousin import nephew
"""
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(contents, depth))
        try:
            self._add_relative_modules(pkg_dir, contents, depth)
            pkg_name = mod_name.rpartition('.')[0]
            if verbose: print "Running from source:", mod_name
            d1 = run_module(mod_name, run_name=run_name) # Read from source
            self.assertIn("__package__", d1)
            self.assertTrue(d1["__package__"] == pkg_name)
            self.assertIn("sibling", d1)
            self.assertIn("nephew", d1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            __import__(mod_name)
            os.remove(mod_fname)
            if verbose: print "Running from compiled:", mod_name
            d2 = run_module(mod_name, run_name=run_name) # Read from bytecode
            self.assertIn("__package__", d2)
            self.assertTrue(d2["__package__"] == pkg_name)
            self.assertIn("sibling", d2)
            self.assertIn("nephew", d2)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose: print "Module executed successfully"

    def test_run_module(self):
        for depth in range(4):
            if verbose: print "Testing package depth:", depth
            self._check_module(depth)

    def test_run_package(self):
        for depth in range(1, 4):
            if verbose: print "Testing package depth:", depth
            self._check_package(depth)

    def test_explicit_relative_import(self):
        for depth in range(2, 5):
            if verbose: print "Testing relative imports at depth:", depth
            self._check_relative_imports(depth)

    def test_main_relative_import(self):
        for depth in range(2, 5):
            if verbose: print "Testing main relative imports at depth:", depth
            self._check_relative_imports(depth, "__main__")


class RunPathTest(unittest.TestCase):
    """Unit tests for runpy.run_path"""
    # Based on corresponding tests in test_cmd_line_script

    test_source = """\
# Script may be run with optimisation enabled, so don't rely on assert
# statements being executed
def assertEqual(lhs, rhs):
    if lhs != rhs:
        raise AssertionError('%r != %r' % (lhs, rhs))
def assertIs(lhs, rhs):
    if lhs is not rhs:
        raise AssertionError('%r is not %r' % (lhs, rhs))
# Check basic code execution
result = ['Top level assignment']
def f():
    result.append('Lower level reference')
f()
assertEqual(result, ['Top level assignment', 'Lower level reference'])
# Check the sys module
import sys
assertIs(globals(), sys.modules[__name__].__dict__)
argv0 = sys.argv[0]
"""

    def _make_test_script(self, script_dir, script_basename, source=None):
        if source is None:
            source = self.test_source
        return make_script(script_dir, script_basename, source)

    def _check_script(self, script_name, expected_name, expected_file,
                            expected_argv0, expected_package):
        result = run_path(script_name)
        self.assertEqual(result["__name__"], expected_name)
        self.assertEqual(result["__file__"], expected_file)
        self.assertIn("argv0", result)
        self.assertEqual(result["argv0"], expected_argv0)
        self.assertEqual(result["__package__"], expected_package)

    def _check_import_error(self, script_name, msg):
        msg = re.escape(msg)
        self.assertRaisesRegexp(ImportError, msg, run_path, script_name)

    def test_basic_script(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(script_name, "<run_path>", script_name,
                               script_name, None)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = compile_script(script_name)
            os.remove(script_name)
            self._check_script(compiled_name, "<run_path>", compiled_name,
                               compiled_name, None)

    def test_directory(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(script_dir, "<run_path>", script_name,
                               script_dir, '')

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = compile_script(script_name)
            os.remove(script_name)
            self._check_script(script_dir, "<run_path>", compiled_name,
                               script_dir, '')

    def test_directory_error(self):
        with temp_dir() as script_dir:
            mod_name = 'not_main'
            script_name = self._make_test_script(script_dir, mod_name)
            msg = "can't find '__main__' module in %r" % script_dir
            self._check_import_error(script_dir, msg)

    def test_zipfile(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            self._check_script(zip_name, "<run_path>", fname, zip_name, '')

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = compile_script(script_name)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, "<run_path>", fname, zip_name, '')

    def test_zipfile_error(self):
        with temp_dir() as script_dir:
            mod_name = 'not_main'
            script_name = self._make_test_script(script_dir, mod_name)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "can't find '__main__' module in %r" % zip_name
            self._check_import_error(zip_name, msg)

    def test_main_recursion_error(self):
        with temp_dir() as script_dir, temp_dir() as dummy_dir:
            mod_name = '__main__'
            source = ("import runpy\n"
                      "runpy.run_path(%r)\n") % dummy_dir
            script_name = self._make_test_script(script_dir, mod_name, source)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "recursion depth exceeded"
            self.assertRaisesRegexp(RuntimeError, msg, run_path, zip_name)



def test_main():
    run_unittest(RunModuleCodeTest, RunModuleTest, RunPathTest)

if __name__ == "__main__":
    test_main()
