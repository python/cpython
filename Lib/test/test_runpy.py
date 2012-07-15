# Test the runpy module
import unittest
import os
import os.path
import sys
import re
import tempfile
import importlib
import py_compile
from test.support import (
    forget, make_legacy_pyc, run_unittest, unload, verbose, no_tracing,
    create_empty_file)
from test.script_helper import (
    make_pkg, make_script, make_zip_pkg, make_zip_script, temp_dir)


import runpy
from runpy import _run_code, _run_module_code, run_module, run_path
# Note: This module can't safely test _run_module_as_main as it
# runs its tests in the current process, which would mess with the
# real __main__ module (usually test.regrtest)
# See test_cmd_line_script for a test that executes that code path


# Set up the test code and expected results
example_source = """\
# Check basic code execution
result = ['Top level assignment']
def f():
    result.append('Lower level reference')
f()
del f
# Check the sys module
import sys
run_argv0 = sys.argv[0]
run_name_in_sys_modules = __name__ in sys.modules
module_in_sys_modules = (run_name_in_sys_modules and
                         globals() is sys.modules[__name__].__dict__)
# Check nested operation
import runpy
nested = runpy._run_module_code('x=1\\n', mod_name='<run>')
"""

implicit_namespace = {
    "__name__": None,
    "__file__": None,
    "__cached__": None,
    "__package__": None,
    "__doc__": None,
}
example_namespace =  {
    "sys": sys,
    "runpy": runpy,
    "result": ["Top level assignment", "Lower level reference"],
    "run_argv0": sys.argv[0],
    "run_name_in_sys_modules": False,
    "module_in_sys_modules": False,
    "nested": dict(implicit_namespace,
                   x=1, __name__="<run>", __loader__=None),
}
example_namespace.update(implicit_namespace)

class CodeExecutionMixin:
    # Issue #15230 (run_path not handling run_name correctly) highlighted a
    # problem with the way arguments were being passed from higher level APIs
    # down to lower level code. This mixin makes it easier to ensure full
    # testing occurs at those upper layers as well, not just at the utility
    # layer

    def assertNamespaceMatches(self, result_ns, expected_ns):
        """Check two namespaces match.

           Ignores any unspecified interpreter created names
        """
        # Impls are permitted to add extra names, so filter them out
        for k in list(result_ns):
            if k.startswith("__") and k.endswith("__"):
                if k not in expected_ns:
                    result_ns.pop(k)
                if k not in expected_ns["nested"]:
                    result_ns["nested"].pop(k)
        # Don't use direct dict comparison - the diffs are too hard to debug
        self.assertEqual(set(result_ns), set(expected_ns))
        for k in result_ns:
            actual = (k, result_ns[k])
            expected = (k, expected_ns[k])
            self.assertEqual(actual, expected)

    def check_code_execution(self, create_namespace, expected_namespace):
        """Check that an interface runs the example code correctly

           First argument is a callable accepting the initial globals and
           using them to create the actual namespace
           Second argument is the expected result
        """
        sentinel = object()
        expected_ns = expected_namespace.copy()
        run_name = expected_ns["__name__"]
        saved_argv0 = sys.argv[0]
        saved_mod = sys.modules.get(run_name, sentinel)
        # Check without initial globals
        result_ns = create_namespace(None)
        self.assertNamespaceMatches(result_ns, expected_ns)
        self.assertIs(sys.argv[0], saved_argv0)
        self.assertIs(sys.modules.get(run_name, sentinel), saved_mod)
        # And then with initial globals
        initial_ns = {"sentinel": sentinel}
        expected_ns["sentinel"] = sentinel
        result_ns = create_namespace(initial_ns)
        self.assertIsNot(result_ns, initial_ns)
        self.assertNamespaceMatches(result_ns, expected_ns)
        self.assertIs(sys.argv[0], saved_argv0)
        self.assertIs(sys.modules.get(run_name, sentinel), saved_mod)


class ExecutionLayerTestCase(unittest.TestCase, CodeExecutionMixin):
    """Unit tests for runpy._run_code and runpy._run_module_code"""

    def test_run_code(self):
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__loader__": None,
        })
        def create_ns(init_globals):
            return _run_code(example_source, {}, init_globals)
        self.check_code_execution(create_ns, expected_ns)

    def test_run_module_code(self):
        mod_name = "<Nonsense>"
        mod_fname = "Some other nonsense"
        mod_loader = "Now you're just being silly"
        mod_package = '' # Treat as a top level module
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__loader__": mod_loader,
            "__package__": mod_package,
            "run_argv0": mod_fname,
            "run_name_in_sys_modules": True,
            "module_in_sys_modules": True,
        })
        def create_ns(init_globals):
            return _run_module_code(example_source,
                                    init_globals,
                                    mod_name,
                                    mod_fname,
                                    mod_loader,
                                    mod_package)
        self.check_code_execution(create_ns, expected_ns)

# TODO: Use self.addCleanup to get rid of a lot of try-finally blocks
class RunModuleTestCase(unittest.TestCase, CodeExecutionMixin):
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
        self.assertEqual(run_module("runpy")["__name__"], "runpy")

    def _add_pkg_dir(self, pkg_dir):
        os.mkdir(pkg_dir)
        pkg_fname = os.path.join(pkg_dir, "__init__.py")
        create_empty_file(pkg_fname)
        return pkg_fname

    def _make_pkg(self, source, depth, mod_base="runpy_test"):
        pkg_name = "__runpy_pkg__"
        test_fname = mod_base+os.extsep+"py"
        pkg_dir = sub_dir = tempfile.mkdtemp()
        if verbose > 1: print("  Package tree in:", sub_dir)
        sys.path.insert(0, pkg_dir)
        if verbose > 1: print("  Updated sys.path:", sys.path[0])
        for i in range(depth):
            sub_dir = os.path.join(sub_dir, pkg_name)
            pkg_fname = self._add_pkg_dir(sub_dir)
            if verbose > 1: print("  Next level in:", sub_dir)
            if verbose > 1: print("  Created:", pkg_fname)
        mod_fname = os.path.join(sub_dir, test_fname)
        mod_file = open(mod_fname, "w")
        mod_file.write(source)
        mod_file.close()
        if verbose > 1: print("  Created:", mod_fname)
        mod_name = (pkg_name+".")*depth + mod_base
        return pkg_dir, mod_fname, mod_name

    def _del_pkg(self, top, depth, mod_name):
        for entry in list(sys.modules):
            if entry.startswith("__runpy_pkg__"):
                del sys.modules[entry]
        if verbose > 1: print("  Removed sys.modules entries")
        del sys.path[0]
        if verbose > 1: print("  Removed sys.path entry")
        for root, dirs, files in os.walk(top, topdown=False):
            for name in files:
                try:
                    os.remove(os.path.join(root, name))
                except OSError as ex:
                    if verbose > 1: print(ex) # Persist with cleaning up
            for name in dirs:
                fullname = os.path.join(root, name)
                try:
                    os.rmdir(fullname)
                except OSError as ex:
                    if verbose > 1: print(ex) # Persist with cleaning up
        try:
            os.rmdir(top)
            if verbose > 1: print("  Removed package tree")
        except OSError as ex:
            if verbose > 1: print(ex) # Persist with cleaning up

    def _fix_ns_for_legacy_pyc(self, ns, alter_sys):
        char_to_add = "c" if __debug__ else "o"
        ns["__file__"] += char_to_add
        if alter_sys:
            ns["run_argv0"] += char_to_add


    def _check_module(self, depth, alter_sys=False):
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(example_source, depth))
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__package__": mod_name.rpartition(".")[0],
        })
        if alter_sys:
            expected_ns.update({
                "run_argv0": mod_fname,
                "run_name_in_sys_modules": True,
                "module_in_sys_modules": True,
            })
        def create_ns(init_globals):
            return run_module(mod_name, init_globals, alter_sys=alter_sys)
        try:
            if verbose > 1: print("Running from source:", mod_name)
            self.check_code_execution(create_ns, expected_ns)
            importlib.invalidate_caches()
            __import__(mod_name)
            os.remove(mod_fname)
            make_legacy_pyc(mod_fname)
            unload(mod_name)  # In case loader caches paths
            importlib.invalidate_caches()
            if verbose > 1: print("Running from compiled:", mod_name)
            self._fix_ns_for_legacy_pyc(expected_ns, alter_sys)
            self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose > 1: print("Module executed successfully")

    def _check_package(self, depth, alter_sys=False):
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(example_source, depth, "__main__"))
        pkg_name = mod_name.rpartition(".")[0]
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__package__": pkg_name,
        })
        if alter_sys:
            expected_ns.update({
                "run_argv0": mod_fname,
                "run_name_in_sys_modules": True,
                "module_in_sys_modules": True,
            })
        def create_ns(init_globals):
            return run_module(pkg_name, init_globals, alter_sys=alter_sys)
        try:
            if verbose > 1: print("Running from source:", pkg_name)
            self.check_code_execution(create_ns, expected_ns)
            importlib.invalidate_caches()
            __import__(mod_name)
            os.remove(mod_fname)
            make_legacy_pyc(mod_fname)
            unload(mod_name)  # In case loader caches paths
            if verbose > 1: print("Running from compiled:", pkg_name)
            importlib.invalidate_caches()
            self._fix_ns_for_legacy_pyc(expected_ns, alter_sys)
            self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir, depth, pkg_name)
        if verbose > 1: print("Package executed successfully")

    def _add_relative_modules(self, base_dir, source, depth):
        if depth <= 1:
            raise ValueError("Relative module test needs depth > 1")
        pkg_name = "__runpy_pkg__"
        module_dir = base_dir
        for i in range(depth):
            parent_dir = module_dir
            module_dir = os.path.join(module_dir, pkg_name)
        # Add sibling module
        sibling_fname = os.path.join(module_dir, "sibling.py")
        create_empty_file(sibling_fname)
        if verbose > 1: print("  Added sibling module:", sibling_fname)
        # Add nephew module
        uncle_dir = os.path.join(parent_dir, "uncle")
        self._add_pkg_dir(uncle_dir)
        if verbose > 1: print("  Added uncle package:", uncle_dir)
        cousin_dir = os.path.join(uncle_dir, "cousin")
        self._add_pkg_dir(cousin_dir)
        if verbose > 1: print("  Added cousin package:", cousin_dir)
        nephew_fname = os.path.join(cousin_dir, "nephew.py")
        create_empty_file(nephew_fname)
        if verbose > 1: print("  Added nephew module:", nephew_fname)

    def _check_relative_imports(self, depth, run_name=None):
        contents = r"""\
from __future__ import absolute_import
from . import sibling
from ..uncle.cousin import nephew
"""
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(contents, depth))
        if run_name is None:
            expected_name = mod_name
        else:
            expected_name = run_name
        try:
            self._add_relative_modules(pkg_dir, contents, depth)
            pkg_name = mod_name.rpartition('.')[0]
            if verbose > 1: print("Running from source:", mod_name)
            d1 = run_module(mod_name, run_name=run_name) # Read from source
            self.assertEqual(d1["__name__"], expected_name)
            self.assertEqual(d1["__package__"], pkg_name)
            self.assertIn("sibling", d1)
            self.assertIn("nephew", d1)
            del d1 # Ensure __loader__ entry doesn't keep file open
            importlib.invalidate_caches()
            __import__(mod_name)
            os.remove(mod_fname)
            make_legacy_pyc(mod_fname)
            unload(mod_name)  # In case the loader caches paths
            if verbose > 1: print("Running from compiled:", mod_name)
            importlib.invalidate_caches()
            d2 = run_module(mod_name, run_name=run_name) # Read from bytecode
            self.assertEqual(d2["__name__"], expected_name)
            self.assertEqual(d2["__package__"], pkg_name)
            self.assertIn("sibling", d2)
            self.assertIn("nephew", d2)
            del d2 # Ensure __loader__ entry doesn't keep file open
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)
        if verbose > 1: print("Module executed successfully")

    def test_run_module(self):
        for depth in range(4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_module(depth)

    def test_run_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth)

    def test_run_module_alter_sys(self):
        for depth in range(4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_module(depth, alter_sys=True)

    def test_run_package_alter_sys(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth, alter_sys=True)

    def test_explicit_relative_import(self):
        for depth in range(2, 5):
            if verbose > 1: print("Testing relative imports at depth:", depth)
            self._check_relative_imports(depth)

    def test_main_relative_import(self):
        for depth in range(2, 5):
            if verbose > 1: print("Testing main relative imports at depth:", depth)
            self._check_relative_imports(depth, "__main__")

    def test_run_name(self):
        depth = 1
        run_name = "And now for something completely different"
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg(example_source, depth))
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": run_name,
            "__file__": mod_fname,
            "__package__": mod_name.rpartition(".")[0],
        })
        def create_ns(init_globals):
            return run_module(mod_name, init_globals, run_name)
        try:
            self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir, depth, mod_name)

    def test_pkgutil_walk_packages(self):
        # This is a dodgy hack to use the test_runpy infrastructure to test
        # issue #15343. Issue #15348 declares this is indeed a dodgy hack ;)
        import pkgutil
        max_depth = 4
        base_name = "__runpy_pkg__"
        package_suffixes = ["uncle", "uncle.cousin"]
        module_suffixes = ["uncle.cousin.nephew", base_name + ".sibling"]
        expected_packages = set()
        expected_modules = set()
        for depth in range(1, max_depth):
            pkg_name = ".".join([base_name] * depth)
            expected_packages.add(pkg_name)
            for name in package_suffixes:
                expected_packages.add(pkg_name + "." + name)
            for name in module_suffixes:
                expected_modules.add(pkg_name + "." + name)
        pkg_name = ".".join([base_name] * max_depth)
        expected_packages.add(pkg_name)
        expected_modules.add(pkg_name + ".runpy_test")
        pkg_dir, mod_fname, mod_name = (
               self._make_pkg("", max_depth))
        self.addCleanup(self._del_pkg, pkg_dir, max_depth, mod_name)
        for depth in range(2, max_depth+1):
            self._add_relative_modules(pkg_dir, "", depth)
        for finder, mod_name, ispkg in pkgutil.walk_packages([pkg_dir]):
            self.assertIsInstance(finder,
                                  importlib.machinery.FileFinder)
            if ispkg:
                expected_packages.remove(mod_name)
            else:
                expected_modules.remove(mod_name)
        self.assertEqual(len(expected_packages), 0, expected_packages)
        self.assertEqual(len(expected_modules), 0, expected_modules)

class RunPathTestCase(unittest.TestCase, CodeExecutionMixin):
    """Unit tests for runpy.run_path"""

    def _make_test_script(self, script_dir, script_basename, source=None):
        if source is None:
            source = example_source
        return make_script(script_dir, script_basename, source)

    def _check_script(self, script_name, expected_name, expected_file,
                            expected_argv0):
        # First check is without run_name
        def create_ns(init_globals):
            return run_path(script_name, init_globals)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": expected_name,
            "__file__": expected_file,
            "__package__": "",
            "run_argv0": expected_argv0,
            "run_name_in_sys_modules": True,
            "module_in_sys_modules": True,
        })
        self.check_code_execution(create_ns, expected_ns)
        # Second check makes sure run_name works in all cases
        run_name = "prove.issue15230.is.fixed"
        def create_ns(init_globals):
            return run_path(script_name, init_globals, run_name)
        expected_ns["__name__"] = run_name
        expected_ns["__package__"] = run_name.rpartition(".")[0]
        self.check_code_execution(create_ns, expected_ns)

    def _check_import_error(self, script_name, msg):
        msg = re.escape(msg)
        self.assertRaisesRegex(ImportError, msg, run_path, script_name)

    def test_basic_script(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(script_name, "<run_path>", script_name,
                               script_name)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            self._check_script(compiled_name, "<run_path>", compiled_name,
                               compiled_name)

    def test_directory(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(script_dir, "<run_path>", script_name,
                               script_dir)

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            legacy_pyc = make_legacy_pyc(script_name)
            self._check_script(script_dir, "<run_path>", legacy_pyc,
                               script_dir)

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
            self._check_script(zip_name, "<run_path>", fname, zip_name)

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            zip_name, fname = make_zip_script(script_dir, 'test_zip',
                                              compiled_name)
            self._check_script(zip_name, "<run_path>", fname, zip_name)

    def test_zipfile_error(self):
        with temp_dir() as script_dir:
            mod_name = 'not_main'
            script_name = self._make_test_script(script_dir, mod_name)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "can't find '__main__' module in %r" % zip_name
            self._check_import_error(zip_name, msg)

    @no_tracing
    def test_main_recursion_error(self):
        with temp_dir() as script_dir, temp_dir() as dummy_dir:
            mod_name = '__main__'
            source = ("import runpy\n"
                      "runpy.run_path(%r)\n") % dummy_dir
            script_name = self._make_test_script(script_dir, mod_name, source)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "recursion depth exceeded"
            self.assertRaisesRegex(RuntimeError, msg, run_path, zip_name)

    def test_encoding(self):
        with temp_dir() as script_dir:
            filename = os.path.join(script_dir, 'script.py')
            with open(filename, 'w', encoding='latin1') as f:
                f.write("""
#coding:latin1
"non-ASCII: h\xe9"
""")
            result = run_path(filename)
            self.assertEqual(result['__doc__'], "non-ASCII: h\xe9")


def test_main():
    run_unittest(
                 ExecutionLayerTestCase,
                 RunModuleTestCase,
                 RunPathTestCase
                 )

if __name__ == "__main__":
    test_main()
