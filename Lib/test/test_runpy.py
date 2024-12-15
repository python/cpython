# Test the runpy module
import contextlib
import importlib.machinery, importlib.util
import os.path
import pathlib
import py_compile
import re
import signal
import subprocess
import sys
import tempfile
import textwrap
import unittest
import warnings
from test.support import (infinite_recursion, no_tracing, verbose,
                          requires_subprocess, requires_resource)
from test.support.import_helper import forget, make_legacy_pyc, unload
from test.support.os_helper import create_empty_file, temp_dir, FakePath
from test.support.script_helper import make_script, make_zip_script


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
    "__spec__": None
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

    # Figuring out the loader details in advance is hard to do, so we skip
    # checking the full details of loader and loader_state
    CHECKED_SPEC_ATTRIBUTES = ["name", "parent", "origin", "cached",
                               "has_location", "submodule_search_locations"]

    def assertNamespaceMatches(self, result_ns, expected_ns):
        """Check two namespaces match.

           Ignores any unspecified interpreter created names
        """
        # Avoid side effects
        result_ns = result_ns.copy()
        expected_ns = expected_ns.copy()
        # Impls are permitted to add extra names, so filter them out
        for k in list(result_ns):
            if k.startswith("__") and k.endswith("__"):
                if k not in expected_ns:
                    result_ns.pop(k)
                if k not in expected_ns["nested"]:
                    result_ns["nested"].pop(k)
        # Spec equality includes the loader, so we take the spec out of the
        # result namespace and check that separately
        result_spec = result_ns.pop("__spec__")
        expected_spec = expected_ns.pop("__spec__")
        if expected_spec is None:
            self.assertIsNone(result_spec)
        else:
            # If an expected loader is set, we just check we got the right
            # type, rather than checking for full equality
            if expected_spec.loader is not None:
                self.assertEqual(type(result_spec.loader),
                                 type(expected_spec.loader))
            for attr in self.CHECKED_SPEC_ATTRIBUTES:
                k = "__spec__." + attr
                actual = (k, getattr(result_spec, attr))
                expected = (k, getattr(expected_spec, attr))
                self.assertEqual(actual, expected)
        # For the rest, we still don't use direct dict comparison on the
        # namespace, as the diffs are too hard to debug if anything breaks
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
        mod_spec = importlib.machinery.ModuleSpec(mod_name,
                                                  origin=mod_fname,
                                                  loader=mod_loader)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__loader__": mod_loader,
            "__package__": mod_package,
            "__spec__": mod_spec,
            "run_argv0": mod_fname,
            "run_name_in_sys_modules": True,
            "module_in_sys_modules": True,
        })
        def create_ns(init_globals):
            return _run_module_code(example_source,
                                    init_globals,
                                    mod_name,
                                    mod_spec)
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
        # Relative names not allowed
        self.expect_import_error(".howard")
        self.expect_import_error("..eaten")
        self.expect_import_error(".test_runpy")
        self.expect_import_error(".unittest")
        # Package without __main__.py
        self.expect_import_error("multiprocessing")

    def test_library_module(self):
        self.assertEqual(run_module("runpy")["__name__"], "runpy")

    def _add_pkg_dir(self, pkg_dir, namespace=False):
        os.mkdir(pkg_dir)
        if namespace:
            return None
        pkg_fname = os.path.join(pkg_dir, "__init__.py")
        create_empty_file(pkg_fname)
        return pkg_fname

    def _make_pkg(self, source, depth, mod_base="runpy_test",
                     *, namespace=False, parent_namespaces=False):
        # Enforce a couple of internal sanity checks on test cases
        if (namespace or parent_namespaces) and not depth:
            raise RuntimeError("Can't mark top level module as a "
                               "namespace package")
        pkg_name = "__runpy_pkg__"
        test_fname = mod_base+os.extsep+"py"
        pkg_dir = sub_dir = os.path.realpath(tempfile.mkdtemp())
        if verbose > 1: print("  Package tree in:", sub_dir)
        sys.path.insert(0, pkg_dir)
        if verbose > 1: print("  Updated sys.path:", sys.path[0])
        if depth:
            namespace_flags = [parent_namespaces] * depth
            namespace_flags[-1] = namespace
            for namespace_flag in namespace_flags:
                sub_dir = os.path.join(sub_dir, pkg_name)
                pkg_fname = self._add_pkg_dir(sub_dir, namespace_flag)
                if verbose > 1: print("  Next level in:", sub_dir)
                if verbose > 1: print("  Created:", pkg_fname)
        mod_fname = os.path.join(sub_dir, test_fname)
        with open(mod_fname, "w") as mod_file:
            mod_file.write(source)
        if verbose > 1: print("  Created:", mod_fname)
        mod_name = (pkg_name+".")*depth + mod_base
        mod_spec = importlib.util.spec_from_file_location(mod_name,
                                                          mod_fname)
        return pkg_dir, mod_fname, mod_name, mod_spec

    def _del_pkg(self, top):
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
        char_to_add = "c"
        ns["__file__"] += char_to_add
        ns["__cached__"] = ns["__file__"]
        spec = ns["__spec__"]
        new_spec = importlib.util.spec_from_file_location(spec.name,
                                                          ns["__file__"])
        ns["__spec__"] = new_spec
        if alter_sys:
            ns["run_argv0"] += char_to_add


    def _check_module(self, depth, alter_sys=False,
                         *, namespace=False, parent_namespaces=False):
        pkg_dir, mod_fname, mod_name, mod_spec = (
               self._make_pkg(example_source, depth,
                              namespace=namespace,
                              parent_namespaces=parent_namespaces))
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__cached__": mod_spec.cached,
            "__package__": mod_name.rpartition(".")[0],
            "__spec__": mod_spec,
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
            if not sys.dont_write_bytecode:
                make_legacy_pyc(mod_fname)
                unload(mod_name)  # In case loader caches paths
                importlib.invalidate_caches()
                if verbose > 1: print("Running from compiled:", mod_name)
                self._fix_ns_for_legacy_pyc(expected_ns, alter_sys)
                self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir)
        if verbose > 1: print("Module executed successfully")

    def _check_package(self, depth, alter_sys=False,
                          *, namespace=False, parent_namespaces=False):
        pkg_dir, mod_fname, mod_name, mod_spec = (
               self._make_pkg(example_source, depth, "__main__",
                              namespace=namespace,
                              parent_namespaces=parent_namespaces))
        pkg_name = mod_name.rpartition(".")[0]
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": mod_name,
            "__file__": mod_fname,
            "__cached__": importlib.util.cache_from_source(mod_fname),
            "__package__": pkg_name,
            "__spec__": mod_spec,
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
            if not sys.dont_write_bytecode:
                make_legacy_pyc(mod_fname)
                unload(mod_name)  # In case loader caches paths
                if verbose > 1: print("Running from compiled:", pkg_name)
                importlib.invalidate_caches()
                self._fix_ns_for_legacy_pyc(expected_ns, alter_sys)
                self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir)
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
        pkg_dir, mod_fname, mod_name, mod_spec = (
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
            if not sys.dont_write_bytecode:
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
            self._del_pkg(pkg_dir)
        if verbose > 1: print("Module executed successfully")

    def test_run_module(self):
        for depth in range(4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_module(depth)

    def test_run_module_in_namespace_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_module(depth, namespace=True, parent_namespaces=True)

    def test_run_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth)

    def test_run_package_init_exceptions(self):
        # These were previously wrapped in an ImportError; see Issue 14285
        result = self._make_pkg("", 1, "__main__")
        pkg_dir, _, mod_name, _ = result
        mod_name = mod_name.replace(".__main__", "")
        self.addCleanup(self._del_pkg, pkg_dir)
        init = os.path.join(pkg_dir, "__runpy_pkg__", "__init__.py")

        exceptions = (ImportError, AttributeError, TypeError, ValueError)
        for exception in exceptions:
            name = exception.__name__
            with self.subTest(name):
                source = "raise {0}('{0} in __init__.py.')".format(name)
                with open(init, "wt", encoding="ascii") as mod_file:
                    mod_file.write(source)
                try:
                    run_module(mod_name)
                except exception as err:
                    self.assertNotIn("finding spec", format(err))
                else:
                    self.fail("Nothing raised; expected {}".format(name))
                try:
                    run_module(mod_name + ".submodule")
                except exception as err:
                    self.assertNotIn("finding spec", format(err))
                else:
                    self.fail("Nothing raised; expected {}".format(name))

    def test_submodule_imported_warning(self):
        pkg_dir, _, mod_name, _ = self._make_pkg("", 1)
        try:
            __import__(mod_name)
            with self.assertWarnsRegex(RuntimeWarning,
                    r"found in sys\.modules"):
                run_module(mod_name)
        finally:
            self._del_pkg(pkg_dir)

    def test_package_imported_no_warning(self):
        pkg_dir, _, mod_name, _ = self._make_pkg("", 1, "__main__")
        self.addCleanup(self._del_pkg, pkg_dir)
        package = mod_name.replace(".__main__", "")
        # No warning should occur if we only imported the parent package
        __import__(package)
        self.assertIn(package, sys.modules)
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            run_module(package)
        # But the warning should occur if we imported the __main__ submodule
        __import__(mod_name)
        with self.assertWarnsRegex(RuntimeWarning, r"found in sys\.modules"):
            run_module(package)

    def test_run_package_in_namespace_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth, parent_namespaces=True)

    def test_run_namespace_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth, namespace=True)

    def test_run_namespace_package_in_namespace_package(self):
        for depth in range(1, 4):
            if verbose > 1: print("Testing package depth:", depth)
            self._check_package(depth, namespace=True, parent_namespaces=True)

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
        pkg_dir, mod_fname, mod_name, mod_spec = (
               self._make_pkg(example_source, depth))
        forget(mod_name)
        expected_ns = example_namespace.copy()
        expected_ns.update({
            "__name__": run_name,
            "__file__": mod_fname,
            "__cached__": importlib.util.cache_from_source(mod_fname),
            "__package__": mod_name.rpartition(".")[0],
            "__spec__": mod_spec,
        })
        def create_ns(init_globals):
            return run_module(mod_name, init_globals, run_name)
        try:
            self.check_code_execution(create_ns, expected_ns)
        finally:
            self._del_pkg(pkg_dir)

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
        pkg_dir, mod_fname, mod_name, mod_spec = (
               self._make_pkg("", max_depth))
        self.addCleanup(self._del_pkg, pkg_dir)
        for depth in range(2, max_depth+1):
            self._add_relative_modules(pkg_dir, "", depth)
        for moduleinfo in pkgutil.walk_packages([pkg_dir]):
            self.assertIsInstance(moduleinfo, pkgutil.ModuleInfo)
            self.assertIsInstance(moduleinfo.module_finder,
                                  importlib.machinery.FileFinder)
            if moduleinfo.ispkg:
                expected_packages.remove(moduleinfo.name)
            else:
                expected_modules.remove(moduleinfo.name)
        self.assertEqual(len(expected_packages), 0, expected_packages)
        self.assertEqual(len(expected_modules), 0, expected_modules)

class RunPathTestCase(unittest.TestCase, CodeExecutionMixin):
    """Unit tests for runpy.run_path"""

    def _make_test_script(self, script_dir, script_basename,
                          source=None, omit_suffix=False):
        if source is None:
            source = example_source
        return make_script(script_dir, script_basename,
                           source, omit_suffix)

    def _check_script(self, script_name, expected_name, expected_file,
                            expected_argv0, mod_name=None,
                            expect_spec=True, check_loader=True):
        # First check is without run_name
        def create_ns(init_globals):
            return run_path(script_name, init_globals)
        expected_ns = example_namespace.copy()
        if mod_name is None:
            spec_name = expected_name
        else:
            spec_name = mod_name
        if expect_spec:
            mod_spec = importlib.util.spec_from_file_location(spec_name,
                                                              expected_file)
            mod_cached = mod_spec.cached
            if not check_loader:
                mod_spec.loader = None
        else:
            mod_spec = mod_cached = None

        expected_ns.update({
            "__name__": expected_name,
            "__file__": expected_file,
            "__cached__": mod_cached,
            "__package__": "",
            "__spec__": mod_spec,
            "run_argv0": expected_argv0,
            "run_name_in_sys_modules": True,
            "module_in_sys_modules": True,
        })
        self.check_code_execution(create_ns, expected_ns)
        # Second check makes sure run_name works in all cases
        run_name = "prove.issue15230.is.fixed"
        def create_ns(init_globals):
            return run_path(script_name, init_globals, run_name)
        if expect_spec and mod_name is None:
            mod_spec = importlib.util.spec_from_file_location(run_name,
                                                              expected_file)
            if not check_loader:
                mod_spec.loader = None
            expected_ns["__spec__"] = mod_spec
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
                               script_name, expect_spec=False)

    def test_basic_script_with_pathlike_object(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(FakePath(script_name), "<run_path>",
                               script_name,
                               script_name,
                               expect_spec=False)

    def test_basic_script_no_suffix(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name,
                                                 omit_suffix=True)
            self._check_script(script_name, "<run_path>", script_name,
                               script_name, expect_spec=False)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            mod_name = 'script'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            self._check_script(compiled_name, "<run_path>", compiled_name,
                               compiled_name, expect_spec=False)

    def test_directory(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            self._check_script(script_dir, "<run_path>", script_name,
                               script_dir, mod_name=mod_name)

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            if not sys.dont_write_bytecode:
                legacy_pyc = make_legacy_pyc(script_name)
                self._check_script(script_dir, "<run_path>", legacy_pyc,
                                   script_dir, mod_name=mod_name)

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
            self._check_script(zip_name, "<run_path>", fname, zip_name,
                               mod_name=mod_name, check_loader=False)

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            mod_name = '__main__'
            script_name = self._make_test_script(script_dir, mod_name)
            compiled_name = py_compile.compile(script_name, doraise=True)
            zip_name, fname = make_zip_script(script_dir, 'test_zip',
                                              compiled_name)
            self._check_script(zip_name, "<run_path>", fname, zip_name,
                               mod_name=mod_name, check_loader=False)

    def test_zipfile_error(self):
        with temp_dir() as script_dir:
            mod_name = 'not_main'
            script_name = self._make_test_script(script_dir, mod_name)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "can't find '__main__' module in %r" % zip_name
            self._check_import_error(zip_name, msg)

    @no_tracing
    @requires_resource('cpu')
    def test_main_recursion_error(self):
        with temp_dir() as script_dir, temp_dir() as dummy_dir:
            mod_name = '__main__'
            source = ("import runpy\n"
                      "runpy.run_path(%r)\n") % dummy_dir
            script_name = self._make_test_script(script_dir, mod_name, source)
            zip_name, fname = make_zip_script(script_dir, 'test_zip', script_name)
            with infinite_recursion(25):
                self.assertRaises(RecursionError, run_path, zip_name)

    def test_encoding(self):
        with temp_dir() as script_dir:
            filename = os.path.join(script_dir, 'script.py')
            with open(filename, 'w', encoding='latin1') as f:
                f.write("""
#coding:latin1
s = "non-ASCII: h\xe9"
""")
            result = run_path(filename)
            self.assertEqual(result['s'], "non-ASCII: h\xe9")


class TestExit(unittest.TestCase):
    STATUS_CONTROL_C_EXIT = 0xC000013A
    EXPECTED_CODE = (
        STATUS_CONTROL_C_EXIT
        if sys.platform == "win32"
        else -signal.SIGINT
    )
    @staticmethod
    @contextlib.contextmanager
    def tmp_path(*args, **kwargs):
        with temp_dir() as tmp_fn:
            yield pathlib.Path(tmp_fn)


    def run(self, *args, **kwargs):
        with self.tmp_path() as tmp:
            self.ham = ham = tmp / "ham.py"
            ham.write_text(
                textwrap.dedent(
                    """\
                    raise KeyboardInterrupt
                    """
                )
            )
            super().run(*args, **kwargs)

    @requires_subprocess()
    def assertSigInt(self, cmd, *args, **kwargs):
        # Use -E to ignore PYTHONSAFEPATH
        cmd = [sys.executable, '-E', *cmd]
        proc = subprocess.run(cmd, *args, **kwargs, text=True, stderr=subprocess.PIPE)
        self.assertTrue(proc.stderr.endswith("\nKeyboardInterrupt\n"), proc.stderr)
        self.assertEqual(proc.returncode, self.EXPECTED_CODE)

    def test_pymain_run_file(self):
        self.assertSigInt([self.ham])

    def test_pymain_run_file_runpy_run_module(self):
        tmp = self.ham.parent
        run_module = tmp / "run_module.py"
        run_module.write_text(
            textwrap.dedent(
                """\
                import runpy
                runpy.run_module("ham")
                """
            )
        )
        self.assertSigInt([run_module], cwd=tmp)

    def test_pymain_run_file_runpy_run_module_as_main(self):
        tmp = self.ham.parent
        run_module_as_main = tmp / "run_module_as_main.py"
        run_module_as_main.write_text(
            textwrap.dedent(
                """\
                import runpy
                runpy._run_module_as_main("ham")
                """
            )
        )
        self.assertSigInt([run_module_as_main], cwd=tmp)

    def test_pymain_run_command_run_module(self):
        self.assertSigInt(
            ["-c", "import runpy; runpy.run_module('ham')"],
            cwd=self.ham.parent,
        )

    def test_pymain_run_command(self):
        self.assertSigInt(["-c", "import ham"], cwd=self.ham.parent)

    def test_pymain_run_stdin(self):
        self.assertSigInt([], input="import ham", cwd=self.ham.parent)

    def test_pymain_run_module(self):
        ham = self.ham
        self.assertSigInt(["-m", ham.stem], cwd=ham.parent)


if __name__ == "__main__":
    unittest.main()
