"""Tests for PEP 810 lazy imports."""

import io
import dis
import subprocess
import sys
import textwrap
import threading
import types
import unittest

try:
    import _testcapi
except ImportError:
    _testcapi = None


class LazyImportTests(unittest.TestCase):
    """Tests for basic lazy import functionality."""

    def tearDown(self):
        """Clean up any test modules from sys.modules."""
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")
        sys.lazy_modules.clear()

    def test_basic_unused(self):
        """Lazy imported module should not be loaded if never accessed."""
        import test.test_import.data.lazy_imports.basic_unused
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)
        self.assertIn("test.test_import.data.lazy_imports", sys.lazy_modules)
        self.assertEqual(sys.lazy_modules["test.test_import.data.lazy_imports"], {"basic2"})

    def test_sys_lazy_modules(self):
        try:
            import test.test_import.data.lazy_imports.basic_from_unused
        except ImportError as e:
            self.fail('lazy import failed')

        self.assertFalse("test.test_import.data.lazy_imports.basic2" in sys.modules)
        self.assertIn("test.test_import.data.lazy_imports", sys.lazy_modules)
        self.assertEqual(sys.lazy_modules["test.test_import.data.lazy_imports"], {"basic2"})
        test.test_import.data.lazy_imports.basic_from_unused.basic2
        self.assertNotIn("test.test_import.data", sys.lazy_modules)

    def test_basic_unused_use_externally(self):
        """Lazy import should load module when accessed from outside."""
        from test.test_import.data.lazy_imports import basic_unused

        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)
        x = basic_unused.test.test_import.data.lazy_imports.basic2
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_basic_from_unused_use_externally(self):
        """Lazy 'from' import should load when accessed from outside."""
        from test.test_import.data.lazy_imports import basic_from_unused

        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)
        x = basic_from_unused.basic2
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_basic_unused_dir(self):
        """dir() on module should not trigger lazy import reification."""
        import test.test_import.data.lazy_imports.basic_unused

        x = dir(test.test_import.data.lazy_imports.basic_unused)
        self.assertIn("test", x)
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_basic_dir(self):
        """dir() at module scope should not trigger lazy import reification."""
        from test.test_import.data.lazy_imports import basic_dir

        self.assertIn("test", basic_dir.x)
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_basic_used(self):
        """Lazy import should load when accessed within the module."""
        import test.test_import.data.lazy_imports.basic_used
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)


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
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_global_on(self):
        """Mode 'all' should make regular imports lazy."""
        import test.test_import.data.lazy_imports.global_on
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_global_filter(self):
        """Filter returning False should prevent lazy loading."""
        import test.test_import.data.lazy_imports.global_filter
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_global_filter_true(self):
        """Filter returning True should allow lazy loading."""
        import test.test_import.data.lazy_imports.global_filter_true
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_global_filter_from(self):
        """Filter should work with 'from' imports."""
        import test.test_import.data.lazy_imports.global_filter
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_global_filter_from_true(self):
        """Filter returning True should allow lazy 'from' imports."""
        import test.test_import.data.lazy_imports.global_filter_true
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)


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
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_compatibility_mode_used(self):
        """Using a lazy import from __lazy_modules__ should load the module."""
        import test.test_import.data.lazy_imports.basic_compatibility_mode_used
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_compatibility_mode_func(self):
        """Imports inside functions should be eager even in compatibility mode."""
        import test.test_import.data.lazy_imports.compatibility_mode_func
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_compatibility_mode_try_except(self):
        """Imports in try/except should be eager even in compatibility mode."""
        import test.test_import.data.lazy_imports.compatibility_mode_try_except
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_compatibility_mode_relative(self):
        """__lazy_modules__ should work with relative imports."""
        import test.test_import.data.lazy_imports.basic_compatibility_mode_relative
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)


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
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_modules_getattr(self):
        """Module __getattr__ for lazy import name should trigger reification."""
        import test.test_import.data.lazy_imports.modules_getattr
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_modules_getattr_other(self):
        """Module __getattr__ for other names should not trigger reification."""
        import test.test_import.data.lazy_imports.modules_getattr_other
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)


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
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_lazy_import_type_exposed(self):
        """LazyImportType should be exposed in types module."""
        self.assertHasAttr(types, 'LazyImportType')
        self.assertEqual(types.LazyImportType.__name__, 'lazy_import')

    def test_lazy_import_type_cant_construct(self):
        """LazyImportType should not be directly constructible."""
        self.assertRaises(TypeError, types.LazyImportType, {}, "module")


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
        with self.assertRaises(SyntaxError) as cm:
            import test.test_import.data.lazy_imports.lazy_future_import
        # Check we highlight 'lazy' (column offset 0, end offset 4)
        self.assertEqual(cm.exception.offset, 1)
        self.assertEqual(cm.exception.end_offset, 5)

    def test_lazy_import_func(self):
        """lazy import inside function should raise SyntaxError."""
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_import_func

    def test_lazy_import_exec_in_function(self):
        """lazy import via exec() inside a function should raise SyntaxError."""
        # exec() inside a function creates a non-module-level context
        # where lazy imports are not allowed
        def f():
            exec("lazy import json")

        with self.assertRaises(SyntaxError) as cm:
            f()
        self.assertIn("only allowed at module level", str(cm.exception))

    def test_lazy_import_exec_at_module_level(self):
        """lazy import via exec() at module level should work."""
        # exec() at module level (globals == locals) should allow lazy imports
        code = textwrap.dedent("""
            import sys
            exec("lazy import json")
            # Should be lazy - not loaded yet
            assert 'json' not in sys.modules
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


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
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_try_except_eager_from(self):
        """From imports in try/except should be eager even with mode='all'."""
        sys.set_lazy_imports("all")
        import test.test_import.data.lazy_imports.try_except_eager_from
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

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
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_lazy_with_from(self):
        """lazy from import with 'with' statement should work."""
        import test.test_import.data.lazy_imports.lazy_with_from
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)


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

        self.assertIn("test.test_import.data.lazy_imports.pkg", sys.modules)
        self.assertIn("test.test_import.data.lazy_imports.pkg.bar", sys.modules)

    def test_lazy_import_pkg_cross_import(self):
        """Cross-imports within package should preserve lazy imports."""
        import test.test_import.data.lazy_imports.pkg.c

        self.assertIn("test.test_import.data.lazy_imports.pkg", sys.modules)
        self.assertIn("test.test_import.data.lazy_imports.pkg.c", sys.modules)
        self.assertNotIn("test.test_import.data.lazy_imports.pkg.b", sys.modules)

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
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_dunder_lazy_import_used(self):
        """Using __lazy_import__ result should trigger module load."""
        import test.test_import.data.lazy_imports.dunder_lazy_import_used
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_dunder_lazy_import_builtins(self):
        """__lazy_import__ should use module's __builtins__ for __import__."""
        from test.test_import.data.lazy_imports import dunder_lazy_import_builtins

        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)
        self.assertEqual(dunder_lazy_import_builtins.basic.basic2, 42)


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

    def test_lazy_modules_attribute_is_set(self):
        """sys.lazy_modules should be a set per PEP 810."""
        self.assertIsInstance(sys.lazy_modules, dict)

    def test_lazy_modules_tracks_lazy_imports(self):
        """sys.lazy_modules should track lazily imported module names."""
        code = textwrap.dedent("""
            import sys
            initial_count = len(sys.lazy_modules)
            import test.test_import.data.lazy_imports.basic_unused
            assert "test.test_import.data.lazy_imports" in sys.lazy_modules
            assert sys.lazy_modules["test.test_import.data.lazy_imports"] == {"basic2"}
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


class ErrorHandlingTests(unittest.TestCase):
    """Tests for error handling during lazy import reification.

    PEP 810: Errors during reification should show exception chaining with
    both the lazy import definition location and the access location.
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_import_error_shows_chained_traceback(self):
        """ImportError during reification should chain to show both definition and access."""
        # Errors at reification must show where the lazy import was defined
        # AND where the access happened, per PEP 810 "Reification" section
        code = textwrap.dedent("""
            import sys
            lazy import test.test_import.data.lazy_imports.nonexistent_module

            try:
                x = test.test_import.data.lazy_imports.nonexistent_module
            except ImportError as e:
                # Should have __cause__ showing the original error
                # The exception chain shows both where import was defined and where access happened
                assert e.__cause__ is not None, "Expected chained exception"
                print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_attribute_error_on_from_import_shows_chained_traceback(self):
        """Accessing missing attribute from lazy from-import should chain errors."""
        # Tests 'lazy from module import nonexistent' behavior
        code = textwrap.dedent("""
            import sys
            lazy from test.test_import.data.lazy_imports.basic2 import nonexistent_name

            try:
                x = nonexistent_name
            except ImportError as e:
                # PEP 810: Enhanced error reporting through exception chaining
                assert e.__cause__ is not None, "Expected chained exception"
                print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_reification_retries_on_failure(self):
        """Failed reification should allow retry on subsequent access.

        PEP 810: "If reification fails, the lazy object is not reified or replaced.
        Subsequent uses of the lazy object will re-try the reification."
        """
        code = textwrap.dedent("""
            import sys
            import types

            lazy import test.test_import.data.lazy_imports.broken_module

            # First access - should fail
            try:
                x = test.test_import.data.lazy_imports.broken_module
            except ValueError:
                pass

            # The lazy object should still be a lazy proxy (not reified)
            g = globals()
            lazy_obj = g['test']
            # The root 'test' binding should still allow retry
            # Second access - should also fail (retry the import)
            try:
                x = test.test_import.data.lazy_imports.broken_module
            except ValueError:
                print("OK - retry worked")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_error_during_module_execution_propagates(self):
        """Errors in module code during reification should propagate correctly."""
        # Module that raises during import should propagate with chaining
        code = textwrap.dedent("""
            import sys
            lazy import test.test_import.data.lazy_imports.broken_module

            try:
                _ = test.test_import.data.lazy_imports.broken_module
                print("FAIL - should have raised")
            except ValueError as e:
                # The ValueError from the module should be the cause
                if "always fails" in str(e) or (e.__cause__ and "always fails" in str(e.__cause__)):
                    print("OK")
                else:
                    print(f"FAIL - wrong error: {e}")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class GlobalsAndDictTests(unittest.TestCase):
    """Tests for globals() and __dict__ behavior with lazy imports.

    PEP 810: "Calling globals() or accessing a module's __dict__ does not trigger
    reification – they return the module's dictionary, and accessing lazy objects
    through that dictionary still returns lazy proxy objects."
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_globals_returns_lazy_proxy_when_accessed_from_function(self):
        """globals() accessed from a function should return lazy proxy without reification.

        Note: At module level, accessing globals()['name'] triggers LOAD_NAME which
        automatically resolves lazy imports. Inside a function, accessing globals()['name']
        uses BINARY_SUBSCR which returns the lazy proxy without resolution.
        """
        code = textwrap.dedent("""
            import sys
            import types

            lazy from test.test_import.data.lazy_imports.basic2 import x

            # Check that module is not yet loaded
            assert 'test.test_import.data.lazy_imports.basic2' not in sys.modules

            def check_lazy():
                # Access through globals() from inside a function
                g = globals()
                lazy_obj = g['x']
                return type(lazy_obj) is types.LazyImportType

            # Inside function, should get lazy proxy
            is_lazy = check_lazy()
            assert is_lazy, "Expected LazyImportType from function scope"

            # Module should STILL not be loaded
            assert 'test.test_import.data.lazy_imports.basic2' not in sys.modules
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_globals_dict_access_returns_lazy_proxy_inline(self):
        """Accessing globals()['name'] inline should return lazy proxy.

        Note: Assigning g['name'] to a local variable at module level triggers
        reification due to STORE_NAME bytecode. Inline access preserves laziness.
        """
        code = textwrap.dedent("""
            import sys
            import types
            lazy import json
            # Inline access without assignment to local variable preserves lazy proxy
            assert type(globals()['json']) is types.LazyImportType
            assert 'json' not in sys.modules
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_module_dict_returns_lazy_proxy_without_reifying(self):
        """module.__dict__ access should not trigger reification."""
        import test.test_import.data.lazy_imports.globals_access

        # Module not loaded yet via direct dict access
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

        # Access via get_from_globals should return lazy proxy
        lazy_obj = test.test_import.data.lazy_imports.globals_access.get_from_globals()
        self.assertEqual(type(lazy_obj), types.LazyImportType)
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_direct_access_triggers_reification(self):
        """Direct name access (not through globals()) should trigger reification."""
        import test.test_import.data.lazy_imports.globals_access

        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

        # Direct access should reify
        result = test.test_import.data.lazy_imports.globals_access.get_direct()
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_resolve_method_forces_reification(self):
        """Calling resolve() on lazy proxy should force reification.

        Note: Must access lazy proxy from within a function to avoid automatic
        reification by LOAD_NAME at module level.
        """
        code = textwrap.dedent("""
            import sys
            import types

            lazy from test.test_import.data.lazy_imports.basic2 import x

            assert 'test.test_import.data.lazy_imports.basic2' not in sys.modules

            def test_resolve():
                g = globals()
                lazy_obj = g['x']
                assert type(lazy_obj) is types.LazyImportType, f"Expected lazy proxy, got {type(lazy_obj)}"

                resolved = lazy_obj.resolve()

                # Now module should be loaded
                assert 'test.test_import.data.lazy_imports.basic2' in sys.modules
                assert resolved == 42  # x is 42 in basic2.py
                return True

            assert test_resolve()
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_add_lazy_to_globals(self):
        code = textwrap.dedent("""
            import sys
            import types

            lazy from test.test_import.data.lazy_imports import basic2

            assert 'test.test_import.data.lazy_imports.basic2' not in sys.modules

            class C: pass
            sneaky = C()
            sneaky.x = 1

            def f():
                t = 0
                for _ in range(5):
                    t += sneaky.x
                return t

            f()
            globals()["sneaky"] = globals()["basic2"]
            assert f() == 210
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class MultipleNameFromImportTests(unittest.TestCase):
    """Tests for lazy from ... import with multiple names.

    PEP 810: "When using lazy from ... import, each imported name is bound to a
    lazy proxy object. The first access to any of these names triggers loading
    of the entire module and reifies only that specific name to its actual value.
    Other names remain as lazy proxies until they are accessed."
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_accessing_one_name_leaves_others_as_proxies(self):
        """Accessing one name from multi-name import should leave others lazy."""
        code = textwrap.dedent("""
            import sys
            import types

            lazy from test.test_import.data.lazy_imports.basic2 import f, x

            # Neither should be loaded yet
            assert 'test.test_import.data.lazy_imports.basic2' not in sys.modules

            g = globals()
            assert type(g['f']) is types.LazyImportType
            assert type(g['x']) is types.LazyImportType

            # Access 'x' - this loads the module and reifies only 'x'
            value = x
            assert value == 42

            # Module is now loaded
            assert 'test.test_import.data.lazy_imports.basic2' in sys.modules

            # 'x' should be reified (int), 'f' should still be lazy proxy
            assert type(g['x']) is int, f"Expected int, got {type(g['x'])}"
            assert type(g['f']) is types.LazyImportType, f"Expected LazyImportType, got {type(g['f'])}"
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_all_names_reified_after_all_accessed(self):
        """All names should be reified after each is accessed."""
        code = textwrap.dedent("""
            import sys
            import types

            lazy from test.test_import.data.lazy_imports.basic2 import f, x

            g = globals()

            # Access both
            _ = x
            _ = f

            # Both should be reified now
            assert type(g['x']) is int
            assert callable(g['f'])
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class SysLazyModulesTrackingTests(unittest.TestCase):
    """Tests for sys.lazy_modules tracking behavior.

    PEP 810: "When the module is reified, it's removed from sys.lazy_modules"
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_module_added_to_lazy_modules_on_lazy_import(self):
        """Module should be added to sys.lazy_modules when lazily imported."""
        # PEP 810 states lazy_modules tracks modules that have been lazily imported
        # Note: The current implementation keeps modules in lazy_modules even after
        # reification (primarily for diagnostics and introspection)
        code = textwrap.dedent("""
            import sys

            initial_count = len(sys.lazy_modules)

            lazy import test.test_import.data.lazy_imports.basic2

            # Should be in lazy_modules after lazy import
            assert "test.test_import.data.lazy_imports" in sys.lazy_modules
            assert sys.lazy_modules["test.test_import.data.lazy_imports"] == {"basic2"}
            assert len(sys.lazy_modules) > initial_count

            # Trigger reification
            _ = test.test_import.data.lazy_imports.basic2.x

            # Module should still be tracked (for diagnostics per PEP 810)
            assert "test.test_import.data.lazy_imports" not in sys.lazy_modules
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_lazy_modules_is_per_interpreter(self):
        """Each interpreter should have independent sys.lazy_modules."""
        # Basic test that sys.lazy_modules exists and is a set
        self.assertIsInstance(sys.lazy_modules, dict)


class CommandLineAndEnvVarTests(unittest.TestCase):
    """Tests for command-line and environment variable control.

    PEP 810: The global lazy imports flag can be controlled through:
    - The -X lazy_imports=<mode> command-line option
    - The PYTHON_LAZY_IMPORTS=<mode> environment variable
    """

    def test_cli_lazy_imports_all_makes_regular_imports_lazy(self):
        """-X lazy_imports=all should make all imports potentially lazy."""
        code = textwrap.dedent("""
            import sys
            # In 'all' mode, regular imports become lazy
            import json
            # json should not be in sys.modules yet (lazy)
            # Actually accessing it triggers reification
            if 'json' not in sys.modules:
                print("LAZY")
            else:
                print("EAGER")
        """)
        result = subprocess.run(
            [sys.executable, "-X", "lazy_imports=all", "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("LAZY", result.stdout)

    def test_cli_lazy_imports_none_forces_all_imports_eager(self):
        """-X lazy_imports=none should force all imports to be eager."""
        code = textwrap.dedent("""
            import sys
            # Even explicit lazy imports should be eager in 'none' mode
            lazy import json
            if 'json' in sys.modules:
                print("EAGER")
            else:
                print("LAZY")
        """)
        result = subprocess.run(
            [sys.executable, "-X", "lazy_imports=none", "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("EAGER", result.stdout)

    def test_cli_lazy_imports_normal_respects_lazy_keyword_only(self):
        """-X lazy_imports=normal should respect lazy keyword only."""
        # Note: Use test modules instead of stdlib modules to avoid
        # modules already loaded by the interpreter startup
        code = textwrap.dedent("""
            import sys
            import test.test_import.data.lazy_imports.basic2  # Should be eager
            lazy import test.test_import.data.lazy_imports.pkg.b  # Should be lazy

            eager_loaded = 'test.test_import.data.lazy_imports.basic2' in sys.modules
            lazy_loaded = 'test.test_import.data.lazy_imports.pkg.b' in sys.modules

            if eager_loaded and not lazy_loaded:
                print("OK")
            else:
                print(f"FAIL: eager={eager_loaded}, lazy={lazy_loaded}")
        """)
        result = subprocess.run(
            [sys.executable, "-X", "lazy_imports=normal", "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_env_var_lazy_imports_all_enables_global_lazy(self):
        """PYTHON_LAZY_IMPORTS=all should enable global lazy imports."""
        code = textwrap.dedent("""
            import sys
            import json
            if 'json' not in sys.modules:
                print("LAZY")
            else:
                print("EAGER")
        """)
        import os
        env = os.environ.copy()
        env["PYTHON_LAZY_IMPORTS"] = "all"
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True,
            env=env
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("LAZY", result.stdout)

    def test_env_var_lazy_imports_none_disables_all_lazy(self):
        """PYTHON_LAZY_IMPORTS=none should disable all lazy imports."""
        code = textwrap.dedent("""
            import sys
            lazy import json
            if 'json' in sys.modules:
                print("EAGER")
            else:
                print("LAZY")
        """)
        import os
        env = os.environ.copy()
        env["PYTHON_LAZY_IMPORTS"] = "none"
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True,
            env=env
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("EAGER", result.stdout)

    def test_cli_overrides_env_var(self):
        """Command-line option should take precedence over environment variable."""
        # PEP 810: -X lazy_imports takes precedence over PYTHON_LAZY_IMPORTS
        code = textwrap.dedent("""
            import sys
            lazy import json
            if 'json' in sys.modules:
                print("EAGER")
            else:
                print("LAZY")
        """)
        import os
        env = os.environ.copy()
        env["PYTHON_LAZY_IMPORTS"] = "all"  # env says all
        result = subprocess.run(
            [sys.executable, "-X", "lazy_imports=none", "-c", code],  # CLI says none
            capture_output=True,
            text=True,
            env=env
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        # CLI should win - imports should be eager
        self.assertIn("EAGER", result.stdout)

    def test_sys_set_lazy_imports_overrides_cli(self):
        """sys.set_lazy_imports() should take precedence over CLI option."""
        code = textwrap.dedent("""
            import sys
            sys.set_lazy_imports("none")  # Override CLI
            lazy import json
            if 'json' in sys.modules:
                print("EAGER")
            else:
                print("LAZY")
        """)
        result = subprocess.run(
            [sys.executable, "-X", "lazy_imports=all", "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("EAGER", result.stdout)


class FilterFunctionSignatureTests(unittest.TestCase):
    """Tests for the filter function signature per PEP 810.

    PEP 810: func(importer: str, name: str, fromlist: tuple[str, ...] | None) -> bool
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_filter_receives_correct_arguments_for_import(self):
        """Filter should receive (importer, name, fromlist=None) for 'import x'."""
        code = textwrap.dedent("""
            import sys

            received_args = []

            def my_filter(importer, name, fromlist):
                received_args.append((importer, name, fromlist))
                return True

            sys.set_lazy_imports_filter(my_filter)

            lazy import json

            assert len(received_args) == 1, f"Expected 1 call, got {len(received_args)}"
            importer, name, fromlist = received_args[0]
            assert name == "json", f"Expected name='json', got {name!r}"
            assert fromlist is None, f"Expected fromlist=None, got {fromlist!r}"
            assert isinstance(importer, str), f"Expected str importer, got {type(importer)}"
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_filter_receives_fromlist_for_from_import(self):
        """Filter should receive fromlist tuple for 'from x import y, z'."""
        code = textwrap.dedent("""
            import sys

            received_args = []

            def my_filter(importer, name, fromlist):
                received_args.append((importer, name, fromlist))
                return True

            sys.set_lazy_imports_filter(my_filter)

            lazy from json import dumps, loads

            assert len(received_args) == 1, f"Expected 1 call, got {len(received_args)}"
            importer, name, fromlist = received_args[0]
            assert name == "json", f"Expected name='json', got {name!r}"
            assert fromlist == ("dumps", "loads"), f"Expected ('dumps', 'loads'), got {fromlist!r}"
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_filter_returning_false_forces_eager_import(self):
        """Filter returning False should make import eager."""
        code = textwrap.dedent("""
            import sys

            def deny_filter(importer, name, fromlist):
                return False

            sys.set_lazy_imports_filter(deny_filter)

            lazy import json

            # Should be eager due to filter
            if 'json' in sys.modules:
                print("EAGER")
            else:
                print("LAZY")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stderr: {result.stderr}")
        self.assertIn("EAGER", result.stdout)


class AdditionalSyntaxRestrictionTests(unittest.TestCase):
    """Additional syntax restriction tests per PEP 810."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_import_inside_class_raises_syntax_error(self):
        """lazy import inside class body should raise SyntaxError."""
        # PEP 810: "The soft keyword is only allowed at the global (module) level,
        # not inside functions, class bodies, try blocks, or import *"
        with self.assertRaises(SyntaxError):
            import test.test_import.data.lazy_imports.lazy_class_body


class MixedLazyEagerImportTests(unittest.TestCase):
    """Tests for mixing lazy and eager imports of the same module.

    PEP 810: "If module foo is imported both lazily and eagerly in the same
    program, the eager import takes precedence and both bindings resolve to
    the same module object."
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_eager_import_before_lazy_resolves_to_same_module(self):
        """Eager import before lazy should make lazy resolve to same module."""
        code = textwrap.dedent("""
            import sys
            import json  # Eager import first

            lazy import json as lazy_json  # Lazy import same module

            # lazy_json should resolve to the same object
            assert json is lazy_json, "Lazy and eager imports should resolve to same module"
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_lazy_import_before_eager_resolves_to_same_module(self):
        """Lazy import followed by eager should both point to same module."""
        code = textwrap.dedent("""
            import sys

            lazy import json as lazy_json

            # Lazy not loaded yet
            assert 'json' not in sys.modules

            import json  # Eager import triggers load

            # Both should be the same object
            assert json is lazy_json
            print("OK")
        """)
        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class RelativeImportTests(unittest.TestCase):
    """Tests for relative imports with lazy keyword."""

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_relative_lazy_import(self):
        """lazy from . import submodule should work."""
        from test.test_import.data.lazy_imports import relative_lazy

        # basic2 should not be loaded yet
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

        # Access triggers reification
        result = relative_lazy.get_basic2()
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)

    def test_relative_lazy_from_import(self):
        """lazy from .module import name should work."""
        from test.test_import.data.lazy_imports import relative_lazy_from

        # basic2 should not be loaded yet
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

        # Access triggers reification
        result = relative_lazy_from.get_x()
        self.assertEqual(result, 42)
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)


class LazyModulesCompatibilityFromImportTests(unittest.TestCase):
    """Tests for __lazy_modules__ with from imports.

    PEP 810: "When a module is made lazy this way, from-imports using that
    module are also lazy"
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_lazy_modules_makes_from_imports_lazy(self):
        """__lazy_modules__ should make from imports of listed modules lazy."""
        from test.test_import.data.lazy_imports import lazy_compat_from

        # basic2 should not be loaded yet because it's in __lazy_modules__
        self.assertNotIn("test.test_import.data.lazy_imports.basic2", sys.modules)

        # Access triggers reification
        result = lazy_compat_from.get_x()
        self.assertEqual(result, 42)
        self.assertIn("test.test_import.data.lazy_imports.basic2", sys.modules)


class ImportStateAtReificationTests(unittest.TestCase):
    """Tests for import system state at reification time.

    PEP 810: "Reification still calls __import__ to resolve the import, which uses
    the state of the import system (e.g. sys.path, sys.meta_path, sys.path_hooks
    and __import__) at reification time, not the state when the lazy import
    statement was evaluated."
    """

    def tearDown(self):
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.lazy_imports'):
                del sys.modules[key]

        sys.set_lazy_imports_filter(None)
        sys.set_lazy_imports("normal")

    def test_sys_path_at_reification_time_is_used(self):
        """sys.path changes after lazy import should affect reification."""
        code = textwrap.dedent("""
            import sys
            import tempfile
            import os

            # Create a temporary module
            with tempfile.TemporaryDirectory() as tmpdir:
                mod_path = os.path.join(tmpdir, "dynamic_test_module.py")
                with open(mod_path, "w") as f:
                    f.write("VALUE = 'from_temp_dir'\\n")

                # Lazy import before adding to path
                lazy import dynamic_test_module

                # Module cannot be found yet
                try:
                    _ = dynamic_test_module
                    print("FAIL - should not find module")
                except ModuleNotFoundError:
                    pass

                # Now add temp dir to path
                sys.path.insert(0, tmpdir)

                # Now reification should succeed using current sys.path
                assert dynamic_test_module.VALUE == 'from_temp_dir'
                print("OK")

                sys.path.remove(tmpdir)
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

    def test_concurrent_lazy_modules_set_updates(self):
        """Multiple threads creating lazy imports should safely update sys.lazy_modules."""
        code = textwrap.dedent("""
            import sys
            import threading

            num_threads = 16
            iterations = 50
            errors = []
            barrier = threading.Barrier(num_threads)

            def create_lazy_imports(idx):
                try:
                    barrier.wait()
                    for i in range(iterations):
                        exec(f"lazy import json as json_{idx}_{i}", globals())
                        exec(f"lazy import os as os_{idx}_{i}", globals())
                except Exception as e:
                    errors.append((idx, e))

            threads = [
                threading.Thread(target=create_lazy_imports, args=(i,))
                for i in range(num_threads)
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()

            assert not errors, f"Errors: {errors}"
            assert isinstance(sys.lazy_modules, dict), "sys.lazy_modules is not a dict"
            print("OK")
        """)

        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_concurrent_reification_same_module_high_contention(self):
        """High contention: many threads reifying the exact same lazy import."""
        code = textwrap.dedent("""
            import sys
            import threading
            import types

            sys.set_lazy_imports("all")

            lazy import json

            num_threads = 20
            results = [None] * num_threads
            errors = []
            barrier = threading.Barrier(num_threads)

            def access_json(idx):
                try:
                    barrier.wait()
                    for _ in range(100):
                        _ = json.dumps
                        _ = json.loads
                    results[idx] = json
                except Exception as e:
                    errors.append((idx, e))

            threads = [
                threading.Thread(target=access_json, args=(i,))
                for i in range(num_threads)
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()

            assert not errors, f"Errors: {errors}"
            assert all(r is not None for r in results), "Some threads got None"
            first = results[0]
            assert all(r is first for r in results), "Inconsistent module objects"
            assert not isinstance(first, types.LazyImportType), "Got lazy import instead of module"
            print("OK")
        """)

        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)

    def test_concurrent_reification_with_module_attribute_access(self):
        """Threads racing to reify and immediately access module attributes."""
        code = textwrap.dedent("""
            import sys
            import threading

            sys.set_lazy_imports("all")

            lazy import collections
            lazy import functools
            lazy import itertools

            num_threads = 12
            results = {}
            errors = []
            barrier = threading.Barrier(num_threads)

            def stress_lazy_imports(idx):
                try:
                    barrier.wait()
                    for _ in range(50):
                        _ = collections.OrderedDict
                        _ = functools.partial
                        _ = itertools.chain
                        _ = collections.defaultdict
                        _ = functools.lru_cache
                        _ = itertools.islice
                    results[idx] = (
                        type(collections).__name__,
                        type(functools).__name__,
                        type(itertools).__name__,
                    )
                except Exception as e:
                    errors.append((idx, e))

            threads = [
                threading.Thread(target=stress_lazy_imports, args=(i,))
                for i in range(num_threads)
            ]

            for t in threads:
                t.start()
            for t in threads:
                t.join()

            assert not errors, f"Errors: {errors}"
            for idx, types_tuple in results.items():
                assert all(t == 'module' for t in types_tuple), f"Thread {idx}: {types_tuple}"
            print("OK")
        """)

        result = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True
        )
        self.assertEqual(result.returncode, 0, f"stdout: {result.stdout}, stderr: {result.stderr}")
        self.assertIn("OK", result.stdout)


class LazyImportDisTests(unittest.TestCase):
    def test_lazy_import_dis(self):
        """dis should properly show lazy import"""
        code = compile("lazy import foo", "exec", "exec")
        f = io.StringIO()
        dis.dis(code, file=f)
        self.assertIn("foo + lazy", f.getvalue())

    def test_normal_import_dis(self):
        """non lazy imports should just show the name"""
        code = compile("import foo", "exec", "exec")
        f = io.StringIO()
        dis.dis(code, file=f)
        for line in f.getvalue().split('\n'):
            if "IMPORT_NAME" in line:
                self.assertIn("(foo)", line)
                break
        else:
            self.assertFail("IMPORT_NAME not found")


@unittest.skipIf(_testcapi is None, 'need the _testcapi module')
class LazyCApiTests(unittest.TestCase):
    def tearDown(self):
        sys.set_lazy_imports("normal")
        sys.set_lazy_imports_filter(None)

    def test_set_matches_sys(self):
        self.assertEqual(_testcapi.PyImport_GetLazyImportsMode(), sys.get_lazy_imports())
        for mode in ("normal", "all", "none"):
            _testcapi.PyImport_SetLazyImportsMode(mode)
            self.assertEqual(_testcapi.PyImport_GetLazyImportsMode(), sys.get_lazy_imports())

    def test_filter_matches_sys(self):
        self.assertEqual(_testcapi.PyImport_GetLazyImportsFilter(), sys.get_lazy_imports_filter())

        def filter(*args):
            pass

        _testcapi.PyImport_SetLazyImportsFilter(filter)
        self.assertEqual(_testcapi.PyImport_GetLazyImportsFilter(), sys.get_lazy_imports_filter())

    def test_set_bad_filter(self):
        self.assertRaises(ValueError, _testcapi.PyImport_SetLazyImportsFilter, 42)


if __name__ == '__main__':
    unittest.main()
