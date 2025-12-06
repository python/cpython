"""Tests for PEP 810 lazy imports."""

import sys
import types
import unittest
import threading
import textwrap
import subprocess
from test.support import import_helper


class LazyImportTests(unittest.TestCase):
    """Tests for basic lazy import functionality."""

    def tearDown(self):
        """Clean up any test modules from sys.modules."""
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_basic_unused(self):
        """Lazy imported module should not be loaded if never accessed."""
        import test.test_import.data.lazy_imports.basic_unused
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_basic_unused_use_externally(self):
        """Lazy import should load module when accessed from outside."""
        from test.test_import.data.lazy_imports import basic_unused

        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)
        x = basic_unused.test.test_import.data.lazy_imports.basic2
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_basic_from_unused_use_externally(self):
        """Lazy 'from' import should load when accessed from outside."""
        from test.test_import.data.lazy_imports import basic_from_unused

        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)
        x = basic_from_unused.basic2
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_basic_unused_dir(self):
        """dir() on module should not trigger lazy import reification."""
        import test.test_import.data.lazy_imports.basic_unused

        x = dir(test.test_import.data.lazy_imports.basic_unused)
        self.assertIn("test", x)
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_basic_dir(self):
        """dir() at module scope should not trigger lazy import reification."""
        from test.test_import.data.lazy_imports import basic_dir

        self.assertIn("test", basic_dir.x)
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_basic_used(self):
        """Lazy import should load when accessed within the module."""
        import test.test_import.data.lazy_imports.basic_used
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)


class GlobalLazyImportModeTests(unittest.TestCase):
    """Tests for sys.set_lazy_imports() global mode control."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_global_off(self):
        """Mode 'none' should disable lazy imports entirely."""
        import test.test_import.data.lazy_imports.global_off
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_global_on(self):
        """Mode 'all' should make regular imports lazy."""
        import test.test_import.data.lazy_imports.global_on
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_global_filter(self):
        """Filter returning False should prevent lazy loading."""
        import test.test_import.data.lazy_imports.global_filter
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_global_filter_true(self):
        """Filter returning True should allow lazy loading."""
        import test.test_import.data.lazy_imports.global_filter_true
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_global_filter_from(self):
        """Filter should work with 'from' imports."""
        import test.test_import.data.lazy_imports.global_filter
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_global_filter_from_true(self):
        """Filter returning True should allow lazy 'from' imports."""
        import test.test_import.data.lazy_imports.global_filter_true
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)


class CompatibilityModeTests(unittest.TestCase):
    """Tests for __lazy_modules__ compatibility mode."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_compatibility_mode(self):
        """__lazy_modules__ should enable lazy imports for listed modules."""
        import test.test_import.data.lazy_imports.basic_compatibility_mode
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_compatibility_mode_used(self):
        """Using a lazy import from __lazy_modules__ should load the module."""
        import test.test_import.data.lazy_imports.basic_compatibility_mode_used
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_compatibility_mode_func(self):
        """Imports inside functions should be eager even in compatibility mode."""
        import test.test_import.data.lazy_imports.compatibility_mode_func
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_compatibility_mode_try_except(self):
        """Imports in try/except should be eager even in compatibility mode."""
        import test.test_import.data.lazy_imports.compatibility_mode_try_except
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_compatibility_mode_relative(self):
        """__lazy_modules__ should work with relative imports."""
        import test.test_import.data.lazy_imports.basic_compatibility_mode_relative
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)


class ModuleIntrospectionTests(unittest.TestCase):
    """Tests for module dict and getattr behavior with lazy imports."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_modules_dict(self):
        """Accessing module.__dict__ should not trigger reification."""
        import test.test_import.data.lazy_imports.modules_dict
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_modules_getattr(self):
        """Module __getattr__ for lazy import name should trigger reification."""
        import test.test_import.data.lazy_imports.modules_getattr
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_modules_getattr_other(self):
        """Module __getattr__ for other names should not trigger reification."""
        import test.test_import.data.lazy_imports.modules_getattr_other
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)


class LazyImportTypeTests(unittest.TestCase):
    """Tests for the LazyImportType and its resolve() method."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_value_resolve(self):
        """resolve() method should force the lazy import to load."""
        import test.test_import.data.lazy_imports.lazy_get_value
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_lazy_import_type_exposed(self):
        """LazyImportType should be exposed in types module."""
        self.assertTrue(hasattr(types, 'LazyImportType'))
        self.assertEqual(types.LazyImportType.__name__, 'lazy_import')


class SyntaxRestrictionTests(unittest.TestCase):
    """Tests for syntax restrictions on lazy imports."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_try_except(self):
        """lazy import inside try/except should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_try_except

    def test_lazy_try_except_from(self):
        """lazy from import inside try/except should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_try_except_from

    def test_lazy_try_except_from_star(self):
        """lazy from import * should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_try_except_from_star

    def test_lazy_future_import(self):
        """lazy from __future__ import should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_future_import

    def test_lazy_import_func(self):
        """lazy import inside function should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_import_func


class EagerImportInLazyModeTests(unittest.TestCase):
    """Tests for imports that should remain eager even in lazy mode."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_try_except_eager(self):
        """Imports in try/except should be eager even with mode='all'."""
        sys.set_lazy_imports("all")
        import test.test_import.data.lazy_imports.try_except_eager
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_try_except_eager_from(self):
        """From imports in try/except should be eager even with mode='all'."""
        sys.set_lazy_imports("all")
        import test.test_import.data.lazy_imports.try_except_eager_from
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_eager_import_func(self):
        """Imports inside functions should return modules, not proxies."""
        sys.set_lazy_imports("all")
        import test.test_import.data.lazy_imports.eager_import_func

        f = test.test_import.data.lazy_imports.eager_import_func.f
        self.assertEqual(type(f()), type(sys))


class WithStatementTests(unittest.TestCase):
    """Tests for lazy imports in with statement context."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_with(self):
        """lazy import with 'with' statement should work."""
        import test.test_import.data.lazy_imports.lazy_with
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_lazy_with_from(self):
        """lazy from import with 'with' statement should work."""
        import test.test_import.data.lazy_imports.lazy_with_from
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)


class PackageTests(unittest.TestCase):
    """Tests for lazy imports with packages."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_import_pkg(self):
        """lazy import of package submodule should load the package."""
        import test.test_import.data.lazy_imports.lazy_import_pkg

        self.assertTrue("test.test_import.data.lazy_imports.pkg" in sys.modules)
        self.assertTrue("test.test_import.data.lazy_imports.pkg.bar" in sys.modules)

    def test_lazy_import_pkg_cross_import(self):
        """Cross-imports within package should preserve lazy imports."""
        import test.test_import.data.lazy_imports.pkg.c

        self.assertTrue("test.test_import.data.lazy_imports.pkg" in sys.modules)
        self.assertTrue("test.test_import.data.lazy_imports.pkg.c" in sys.modules)
        self.assertFalse("test.test_import.data.lazy_imports.pkg.b" in sys.modules)

        g = test.test_import.data.lazy_imports.pkg.c.get_globals()
        self.assertEqual(type(g["x"]), int)
        self.assertEqual(type(g["b"]), types.LazyImportType)


class DunderLazyImportTests(unittest.TestCase):
    """Tests for __lazy_import__ builtin function."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_dunder_lazy_import(self):
        """__lazy_import__ should create lazy import proxy."""
        import test.test_import.data.lazy_imports.dunder_lazy_import
        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_dunder_lazy_import_used(self):
        """Using __lazy_import__ result should trigger module load."""
        import test.test_import.data.lazy_imports.dunder_lazy_import_used
        self.assertTrue("test.test_import.data.lazy_imports.basic2" in sys.modules)

    def test_dunder_lazy_import_builtins(self):
        """__lazy_import__ should use module's __builtins__ for __import__."""
        from test.test_import.data.lazy_imports import dunder_lazy_import_builtins

        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)
        self.assertEqual(dunder_lazy_import_builtins.basic, 42)


class SysLazyImportsAPITests(unittest.TestCase):
    """Tests for sys lazy imports API functions."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_set_lazy_imports_requires_string(self):
        """set_lazy_imports should reject non-string arguments."""
        with self.assertRaises(TypeError):
            sys.set_lazy_imports(True)
        with self.assertRaises(TypeError):
            sys.set_lazy_imports(None)
        with self.assertRaises(TypeError):
            sys.set_lazy_imports(1)

    def test_set_lazy_imports_rejects_invalid_mode(self):
        """set_lazy_imports should reject invalid mode strings."""
        with self.assertRaises(ValueError):
            sys.set_lazy_imports("invalid")
        with self.assertRaises(ValueError):
            sys.set_lazy_imports("on")
        with self.assertRaises(ValueError):
            sys.set_lazy_imports("off")

    def test_get_lazy_imports_returns_string(self):
        """get_lazy_imports should return string modes."""
        sys.set_lazy_imports("normal")
        self.assertEqual(sys.get_lazy_imports(), "normal")

        sys.set_lazy_imports("all")
        self.assertEqual(sys.get_lazy_imports(), "all")

        sys.set_lazy_imports("none")
        self.assertEqual(sys.get_lazy_imports(), "none")

    def test_get_lazy_imports_filter_default(self):
        """get_lazy_imports_filter should return None by default."""
        sys.set_lazy_imports_filter(None)
        self.assertIsNone(sys.get_lazy_imports_filter())

    def test_set_and_get_lazy_imports_filter(self):
        """set/get_lazy_imports_filter should round-trip filter function."""
        def my_filter(name):
            return name.startswith("test.")

        sys.set_lazy_imports_filter(my_filter)
        self.assertIs(sys.get_lazy_imports_filter(), my_filter)

    def test_get_lazy_modules_returns_set(self):
        """get_lazy_modules should return a set per PEP 810."""
        result = sys.get_lazy_modules()
        self.assertIsInstance(result, set)

    def test_lazy_modules_attribute_is_set(self):
        """sys.lazy_modules should be a set per PEP 810."""
        self.assertIsInstance(sys.lazy_modules, set)
        self.assertIs(sys.lazy_modules, sys.get_lazy_modules())

    def test_lazy_modules_tracks_lazy_imports(self):
        """sys.lazy_modules should track lazily imported module names."""
        code = textwrap.dedent("""
            import sys
            initial_count = len(sys.lazy_modules)
            import test.test_import.data.lazy_imports.basic_unused
            assert "test.test_import.data.lazy_imports.basic2" in sys.lazy_modules
            assert len(sys.lazy_modules) > initial_count
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class ThreadSafetyTests(unittest.TestCase):
    """Tests for thread-safety of lazy imports."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_concurrent_lazy_import_reification(self):
        """Multiple threads racing to reify the same lazy import should succeed."""
        from test.test_import.data.lazy_imports import basic_unused

        num_threads = 10
        results = [None] * num_threads
        errors = []
        barrier = threading.Barrier(num_threads)

        def access_lazy_import(idx):
            try:
                barrier.wait()
                module = basic_unused.test.test_import.data.lazy_imports.basic2
                results[idx] = module
            except Exception as e:
                errors.append((idx, e))

        threads = [
            threading.Thread(target=access_lazy_import, args=(i,))
            for i in range(num_threads)
        ]

        for t in threads:
            t.start()
        for t in threads:
            t.join()

        self.assertEqual(errors, [], f"Errors occurred: {errors}")
        self.assertTrue(all(r is not None for r in results))
        first_module = results[0]
        for r in results[1:]:
            self.assertIs(r, first_module)

    def test_concurrent_reification_multiple_modules(self):
        """Multiple threads reifying different lazy imports concurrently."""
        code = textwrap.dedent("""
            import sys
            import threading

            sys.set_lazy_imports("all")

            lazy import json
            lazy import os
            lazy import io
            lazy import re

            num_threads = 8
            results = {}
            errors = []
            barrier = threading.Barrier(num_threads)

            def access_modules(idx):
                try:
                    barrier.wait()
                    mods = [json, os, io, re]
                    results[idx] = [type(m).__name__ for m in mods]
                except Exception as e:
                    errors.append((idx, e))

            threads = [
                threading.Thread(target=access_modules, args=(i,))
                for i in range(num_threads)
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()

            assert not errors, f"Errors: {errors}"
            for idx, mods in results.items():
                assert all(m == 'module' for m in mods), f"Thread {idx} got: {mods}"

            print("OK")
        """)

        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


if __name__ == '__main__':
    unittest.main()
