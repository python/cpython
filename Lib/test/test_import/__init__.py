import builtins
import errno
import glob
import json
import importlib.util
from importlib._bootstrap_external import _get_sourcefile
from importlib.machinery import (
    BuiltinImporter, ExtensionFileLoader, FrozenImporter, SourceFileLoader,
)
import marshal
import os
import py_compile
import random
import shutil
import stat
import subprocess
import sys
import textwrap
import threading
import time
import types
import unittest
from unittest import mock
import _imp

from test.support import os_helper
from test.support import (
    STDLIB_DIR, swap_attr, swap_item, cpython_only, is_emscripten,
    is_wasi, run_in_subinterp, run_in_subinterp_with_config, Py_TRACE_REFS)
from test.support.import_helper import (
    forget, make_legacy_pyc, unlink, unload, ready_to_import,
    DirsOnSysPath, CleanImport)
from test.support.os_helper import (
    TESTFN, rmtree, temp_umask, TESTFN_UNENCODABLE)
from test.support import script_helper
from test.support import threading_helper
from test.test_importlib.util import uncache
from types import ModuleType
try:
    import _testsinglephase
except ImportError:
    _testsinglephase = None
try:
    import _testmultiphase
except ImportError:
    _testmultiphase = None
try:
    import _xxsubinterpreters as _interpreters
except ModuleNotFoundError:
    _interpreters = None
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


skip_if_dont_write_bytecode = unittest.skipIf(
        sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")


def _require_loader(module, loader, skip):
    if isinstance(module, str):
        module = __import__(module)

    MODULE_KINDS = {
        BuiltinImporter: 'built-in',
        ExtensionFileLoader: 'extension',
        FrozenImporter: 'frozen',
        SourceFileLoader: 'pure Python',
    }

    expected = loader
    assert isinstance(expected, type), expected
    expected = MODULE_KINDS[expected]

    actual = module.__spec__.loader
    if not isinstance(actual, type):
        actual = type(actual)
    actual = MODULE_KINDS[actual]

    if actual != expected:
        err = f'expected module to be {expected}, got {module.__spec__}'
        if skip:
            raise unittest.SkipTest(err)
        raise Exception(err)
    return module

def require_builtin(module, *, skip=False):
    module = _require_loader(module, BuiltinImporter, skip)
    assert module.__spec__.origin == 'built-in', module.__spec__

def require_extension(module, *, skip=False):
    _require_loader(module, ExtensionFileLoader, skip)

def require_frozen(module, *, skip=True):
    module = _require_loader(module, FrozenImporter, skip)
    assert module.__spec__.origin == 'frozen', module.__spec__

def require_pure_python(module, *, skip=False):
    _require_loader(module, SourceFileLoader, skip)

def remove_files(name):
    for f in (name + ".py",
              name + ".pyc",
              name + ".pyw",
              name + "$py.class"):
        unlink(f)
    rmtree('__pycache__')


def no_rerun(reason):
    """Skip rerunning for a particular test.

    WARNING: Use this decorator with care; skipping rerunning makes it
    impossible to find reference leaks. Provide a clear reason for skipping the
    test using the 'reason' parameter.
    """
    def deco(func):
        _has_run = False
        def wrapper(self):
            nonlocal _has_run
            if _has_run:
                self.skipTest(reason)
            func(self)
            _has_run = True
        return wrapper
    return deco


if _testsinglephase is not None:
    def restore__testsinglephase(*, _orig=_testsinglephase):
        # We started with the module imported and want to restore
        # it to its nominal state.
        sys.modules.pop('_testsinglephase', None)
        _orig._clear_globals()
        _testinternalcapi.clear_extension('_testsinglephase', _orig.__file__)
        import _testsinglephase


def requires_singlephase_init(meth):
    """Decorator to skip if single-phase init modules are not supported."""
    if not isinstance(meth, type):
        def meth(self, _meth=meth):
            try:
                return _meth(self)
            finally:
                restore__testsinglephase()
    meth = cpython_only(meth)
    return unittest.skipIf(_testsinglephase is None,
                           'test requires _testsinglephase module')(meth)


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(_interpreters is None,
                           'subinterpreters required')(meth)


class ModuleSnapshot(types.SimpleNamespace):
    """A representation of a module for testing.

    Fields:

    * id - the module's object ID
    * module - the actual module or an adequate substitute
       * __file__
       * __spec__
          * name
          * origin
    * ns - a copy (dict) of the module's __dict__ (or None)
    * ns_id - the object ID of the module's __dict__
    * cached - the sys.modules[mod.__spec__.name] entry (or None)
    * cached_id - the object ID of the sys.modules entry (or None)

    In cases where the value is not available (e.g. due to serialization),
    the value will be None.
    """
    _fields = tuple('id module ns ns_id cached cached_id'.split())

    @classmethod
    def from_module(cls, mod):
        name = mod.__spec__.name
        cached = sys.modules.get(name)
        return cls(
            id=id(mod),
            module=mod,
            ns=types.SimpleNamespace(**mod.__dict__),
            ns_id=id(mod.__dict__),
            cached=cached,
            cached_id=id(cached),
        )

    SCRIPT = textwrap.dedent('''
        {imports}

        name = {name!r}

        {prescript}

        mod = {name}

        {body}

        {postscript}
        ''')
    IMPORTS = textwrap.dedent('''
        import sys
        ''').strip()
    SCRIPT_BODY = textwrap.dedent('''
        # Capture the snapshot data.
        cached = sys.modules.get(name)
        snapshot = dict(
            id=id(mod),
            module=dict(
                __file__=mod.__file__,
                __spec__=dict(
                    name=mod.__spec__.name,
                    origin=mod.__spec__.origin,
                ),
            ),
            ns=None,
            ns_id=id(mod.__dict__),
            cached=None,
            cached_id=id(cached) if cached else None,
        )
        ''').strip()
    CLEANUP_SCRIPT = textwrap.dedent('''
        # Clean up the module.
        sys.modules.pop(name, None)
        ''').strip()

    @classmethod
    def build_script(cls, name, *,
                     prescript=None,
                     import_first=False,
                     postscript=None,
                     postcleanup=False,
                     ):
        if postcleanup is True:
            postcleanup = cls.CLEANUP_SCRIPT
        elif isinstance(postcleanup, str):
            postcleanup = textwrap.dedent(postcleanup).strip()
            postcleanup = cls.CLEANUP_SCRIPT + os.linesep + postcleanup
        else:
            postcleanup = ''
        prescript = textwrap.dedent(prescript).strip() if prescript else ''
        postscript = textwrap.dedent(postscript).strip() if postscript else ''

        if postcleanup:
            if postscript:
                postscript = postscript + os.linesep * 2 + postcleanup
            else:
                postscript = postcleanup

        if import_first:
            prescript += textwrap.dedent(f'''

                # Now import the module.
                assert name not in sys.modules
                import {name}''')

        return cls.SCRIPT.format(
            imports=cls.IMPORTS.strip(),
            name=name,
            prescript=prescript.strip(),
            body=cls.SCRIPT_BODY.strip(),
            postscript=postscript,
        )

    @classmethod
    def parse(cls, text):
        raw = json.loads(text)
        mod = raw['module']
        mod['__spec__'] = types.SimpleNamespace(**mod['__spec__'])
        raw['module'] = types.SimpleNamespace(**mod)
        return cls(**raw)

    @classmethod
    def from_subinterp(cls, name, interpid=None, *, pipe=None, **script_kwds):
        if pipe is not None:
            return cls._from_subinterp(name, interpid, pipe, script_kwds)
        pipe = os.pipe()
        try:
            return cls._from_subinterp(name, interpid, pipe, script_kwds)
        finally:
            r, w = pipe
            os.close(r)
            os.close(w)

    @classmethod
    def _from_subinterp(cls, name, interpid, pipe, script_kwargs):
        r, w = pipe

        # Build the script.
        postscript = textwrap.dedent(f'''
            # Send the result over the pipe.
            import json
            import os
            os.write({w}, json.dumps(snapshot).encode())

            ''')
        _postscript = script_kwargs.get('postscript')
        if _postscript:
            _postscript = textwrap.dedent(_postscript).lstrip()
            postscript += _postscript
        script_kwargs['postscript'] = postscript.strip()
        script = cls.build_script(name, **script_kwargs)

        # Run the script.
        if interpid is None:
            ret = run_in_subinterp(script)
            if ret != 0:
                raise AssertionError(f'{ret} != 0')
        else:
            _interpreters.run_string(interpid, script)

        # Parse the results.
        text = os.read(r, 1000)
        return cls.parse(text.decode())


class ImportTests(unittest.TestCase):

    def setUp(self):
        remove_files(TESTFN)
        importlib.invalidate_caches()

    def tearDown(self):
        unload(TESTFN)

    def test_import_raises_ModuleNotFoundError(self):
        with self.assertRaises(ModuleNotFoundError):
            import something_that_should_not_exist_anywhere

    def test_from_import_missing_module_raises_ModuleNotFoundError(self):
        with self.assertRaises(ModuleNotFoundError):
            from something_that_should_not_exist_anywhere import blah

    def test_from_import_missing_attr_raises_ImportError(self):
        with self.assertRaises(ImportError):
            from importlib import something_that_should_not_exist_anywhere

    def test_from_import_missing_attr_has_name_and_path(self):
        with CleanImport('os'):
            import os
            with self.assertRaises(ImportError) as cm:
                from os import i_dont_exist
        self.assertEqual(cm.exception.name, 'os')
        self.assertEqual(cm.exception.path, os.__file__)
        self.assertRegex(str(cm.exception), r"cannot import name 'i_dont_exist' from 'os' \(.*os.py\)")

    @cpython_only
    def test_from_import_missing_attr_has_name_and_so_path(self):
        import _testcapi
        with self.assertRaises(ImportError) as cm:
            from _testcapi import i_dont_exist
        self.assertEqual(cm.exception.name, '_testcapi')
        if hasattr(_testcapi, "__file__"):
            self.assertEqual(cm.exception.path, _testcapi.__file__)
            self.assertRegex(
                str(cm.exception),
                r"cannot import name 'i_dont_exist' from '_testcapi' \(.*\.(so|pyd)\)"
            )
        else:
            self.assertEqual(
                str(cm.exception),
                "cannot import name 'i_dont_exist' from '_testcapi' (unknown location)"
            )

    def test_from_import_missing_attr_has_name(self):
        with self.assertRaises(ImportError) as cm:
            # _warning has no path as it's a built-in module.
            from _warning import i_dont_exist
        self.assertEqual(cm.exception.name, '_warning')
        self.assertIsNone(cm.exception.path)

    def test_from_import_missing_attr_path_is_canonical(self):
        with self.assertRaises(ImportError) as cm:
            from os.path import i_dont_exist
        self.assertIn(cm.exception.name, {'posixpath', 'ntpath'})
        self.assertIsNotNone(cm.exception)

    def test_from_import_star_invalid_type(self):
        import re
        with ready_to_import() as (name, path):
            with open(path, 'w', encoding='utf-8') as f:
                f.write("__all__ = [b'invalid_type']")
            globals = {}
            with self.assertRaisesRegex(
                TypeError, f"{re.escape(name)}\\.__all__ must be str"
            ):
                exec(f"from {name} import *", globals)
            self.assertNotIn(b"invalid_type", globals)
        with ready_to_import() as (name, path):
            with open(path, 'w', encoding='utf-8') as f:
                f.write("globals()[b'invalid_type'] = object()")
            globals = {}
            with self.assertRaisesRegex(
                TypeError, f"{re.escape(name)}\\.__dict__ must be str"
            ):
                exec(f"from {name} import *", globals)
            self.assertNotIn(b"invalid_type", globals)

    def test_case_sensitivity(self):
        # Brief digression to test that import is case-sensitive:  if we got
        # this far, we know for sure that "random" exists.
        with self.assertRaises(ImportError):
            import RAnDoM

    def test_double_const(self):
        # Importing double_const checks that float constants
        # serialiazed by marshal as PYC files don't lose precision
        # (SF bug 422177).
        from test.test_import.data import double_const
        unload('test.test_import.data.double_const')
        from test.test_import.data import double_const

    def test_import(self):
        def test_with_extension(ext):
            # The extension is normally ".py", perhaps ".pyw".
            source = TESTFN + ext
            pyc = TESTFN + ".pyc"

            with open(source, "w", encoding='utf-8') as f:
                print("# This tests Python's ability to import a",
                      ext, "file.", file=f)
                a = random.randrange(1000)
                b = random.randrange(1000)
                print("a =", a, file=f)
                print("b =", b, file=f)

            if TESTFN in sys.modules:
                del sys.modules[TESTFN]
            importlib.invalidate_caches()
            try:
                try:
                    mod = __import__(TESTFN)
                except ImportError as err:
                    self.fail("import from %s failed: %s" % (ext, err))

                self.assertEqual(mod.a, a,
                    "module loaded (%s) but contents invalid" % mod)
                self.assertEqual(mod.b, b,
                    "module loaded (%s) but contents invalid" % mod)
            finally:
                forget(TESTFN)
                unlink(source)
                unlink(pyc)

        sys.path.insert(0, os.curdir)
        try:
            test_with_extension(".py")
            if sys.platform.startswith("win"):
                for ext in [".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw"]:
                    test_with_extension(ext)
        finally:
            del sys.path[0]

    def test_module_with_large_stack(self, module='longlist'):
        # Regression test for http://bugs.python.org/issue561858.
        filename = module + '.py'

        # Create a file with a list of 65000 elements.
        with open(filename, 'w', encoding='utf-8') as f:
            f.write('d = [\n')
            for i in range(65000):
                f.write('"",\n')
            f.write(']')

        try:
            # Compile & remove .py file; we only need .pyc.
            # Bytecode must be relocated from the PEP 3147 bytecode-only location.
            py_compile.compile(filename)
        finally:
            unlink(filename)

        # Need to be able to load from current dir.
        sys.path.append('')
        importlib.invalidate_caches()

        namespace = {}
        try:
            make_legacy_pyc(filename)
            # This used to crash.
            exec('import ' + module, None, namespace)
        finally:
            # Cleanup.
            del sys.path[-1]
            unlink(filename + 'c')
            unlink(filename + 'o')

            # Remove references to the module (unload the module)
            namespace.clear()
            try:
                del sys.modules[module]
            except KeyError:
                pass

    def test_failing_import_sticks(self):
        source = TESTFN + ".py"
        with open(source, "w", encoding='utf-8') as f:
            print("a = 1/0", file=f)

        # New in 2.4, we shouldn't be able to import that no matter how often
        # we try.
        sys.path.insert(0, os.curdir)
        importlib.invalidate_caches()
        if TESTFN in sys.modules:
            del sys.modules[TESTFN]
        try:
            for i in [1, 2, 3]:
                self.assertRaises(ZeroDivisionError, __import__, TESTFN)
                self.assertNotIn(TESTFN, sys.modules,
                                 "damaged module in sys.modules on %i try" % i)
        finally:
            del sys.path[0]
            remove_files(TESTFN)

    def test_import_name_binding(self):
        # import x.y.z binds x in the current namespace
        import test as x
        import test.support
        self.assertIs(x, test, x.__name__)
        self.assertTrue(hasattr(test.support, "__file__"))

        # import x.y.z as w binds z as w
        import test.support as y
        self.assertIs(y, test.support, y.__name__)

    def test_issue31286(self):
        # import in a 'finally' block resulted in SystemError
        try:
            x = ...
        finally:
            import test.support.script_helper as x

        # import in a 'while' loop resulted in stack overflow
        i = 0
        while i < 10:
            import test.support.script_helper as x
            i += 1

        # import in a 'for' loop resulted in segmentation fault
        for i in range(2):
            import test.support.script_helper as x

    def test_failing_reload(self):
        # A failing reload should leave the module object in sys.modules.
        source = TESTFN + os.extsep + "py"
        with open(source, "w", encoding='utf-8') as f:
            f.write("a = 1\nb=2\n")

        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assertIn(TESTFN, sys.modules)
            self.assertEqual(mod.a, 1, "module has wrong attribute values")
            self.assertEqual(mod.b, 2, "module has wrong attribute values")

            # On WinXP, just replacing the .py file wasn't enough to
            # convince reload() to reparse it.  Maybe the timestamp didn't
            # move enough.  We force it to get reparsed by removing the
            # compiled file too.
            remove_files(TESTFN)

            # Now damage the module.
            with open(source, "w", encoding='utf-8') as f:
                f.write("a = 10\nb=20//0\n")

            self.assertRaises(ZeroDivisionError, importlib.reload, mod)
            # But we still expect the module to be in sys.modules.
            mod = sys.modules.get(TESTFN)
            self.assertIsNotNone(mod, "expected module to be in sys.modules")

            # We should have replaced a w/ 10, but the old b value should
            # stick.
            self.assertEqual(mod.a, 10, "module has wrong attribute values")
            self.assertEqual(mod.b, 2, "module has wrong attribute values")

        finally:
            del sys.path[0]
            remove_files(TESTFN)
            unload(TESTFN)

    @skip_if_dont_write_bytecode
    def test_file_to_source(self):
        # check if __file__ points to the source file where available
        source = TESTFN + ".py"
        with open(source, "w", encoding='utf-8') as f:
            f.write("test = None\n")

        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assertTrue(mod.__file__.endswith('.py'))
            os.remove(source)
            del sys.modules[TESTFN]
            make_legacy_pyc(source)
            importlib.invalidate_caches()
            mod = __import__(TESTFN)
            base, ext = os.path.splitext(mod.__file__)
            self.assertEqual(ext, '.pyc')
        finally:
            del sys.path[0]
            remove_files(TESTFN)
            if TESTFN in sys.modules:
                del sys.modules[TESTFN]

    def test_import_by_filename(self):
        path = os.path.abspath(TESTFN)
        encoding = sys.getfilesystemencoding()
        try:
            path.encode(encoding)
        except UnicodeEncodeError:
            self.skipTest('path is not encodable to {}'.format(encoding))
        with self.assertRaises(ImportError) as c:
            __import__(path)

    def test_import_in_del_does_not_crash(self):
        # Issue 4236
        testfn = script_helper.make_script('', TESTFN, textwrap.dedent("""\
            import sys
            class C:
               def __del__(self):
                  import importlib
            sys.argv.insert(0, C())
            """))
        script_helper.assert_python_ok(testfn)

    @skip_if_dont_write_bytecode
    def test_timestamp_overflow(self):
        # A modification timestamp larger than 2**32 should not be a problem
        # when importing a module (issue #11235).
        sys.path.insert(0, os.curdir)
        try:
            source = TESTFN + ".py"
            compiled = importlib.util.cache_from_source(source)
            with open(source, 'w', encoding='utf-8') as f:
                pass
            try:
                os.utime(source, (2 ** 33 - 5, 2 ** 33 - 5))
            except OverflowError:
                self.skipTest("cannot set modification time to large integer")
            except OSError as e:
                if e.errno not in (getattr(errno, 'EOVERFLOW', None),
                                   getattr(errno, 'EINVAL', None)):
                    raise
                self.skipTest("cannot set modification time to large integer ({})".format(e))
            __import__(TESTFN)
            # The pyc file was created.
            os.stat(compiled)
        finally:
            del sys.path[0]
            remove_files(TESTFN)

    def test_bogus_fromlist(self):
        try:
            __import__('http', fromlist=['blah'])
        except ImportError:
            self.fail("fromlist must allow bogus names")

    @cpython_only
    def test_delete_builtins_import(self):
        args = ["-c", "del __builtins__.__import__; import os"]
        popen = script_helper.spawn_python(*args)
        stdout, stderr = popen.communicate()
        self.assertIn(b"ImportError", stdout)

    def test_from_import_message_for_nonexistent_module(self):
        with self.assertRaisesRegex(ImportError, "^No module named 'bogus'"):
            from bogus import foo

    def test_from_import_message_for_existing_module(self):
        with self.assertRaisesRegex(ImportError, "^cannot import name 'bogus'"):
            from re import bogus

    def test_from_import_AttributeError(self):
        # Issue #24492: trying to import an attribute that raises an
        # AttributeError should lead to an ImportError.
        class AlwaysAttributeError:
            def __getattr__(self, _):
                raise AttributeError

        module_name = 'test_from_import_AttributeError'
        self.addCleanup(unload, module_name)
        sys.modules[module_name] = AlwaysAttributeError()
        with self.assertRaises(ImportError) as cm:
            from test_from_import_AttributeError import does_not_exist

        self.assertEqual(str(cm.exception),
            "cannot import name 'does_not_exist' from '<unknown module name>' (unknown location)")

    @cpython_only
    def test_issue31492(self):
        # There shouldn't be an assertion failure in case of failing to import
        # from a module with a bad __name__ attribute, or in case of failing
        # to access an attribute of such a module.
        with swap_attr(os, '__name__', None):
            with self.assertRaises(ImportError):
                from os import does_not_exist

            with self.assertRaises(AttributeError):
                os.does_not_exist

    @threading_helper.requires_working_threading()
    def test_concurrency(self):
        # bpo 38091: this is a hack to slow down the code that calls
        # has_deadlock(); the logic was itself sometimes deadlocking.
        def delay_has_deadlock(frame, event, arg):
            if event == 'call' and frame.f_code.co_name == 'has_deadlock':
                time.sleep(0.1)

        sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'data'))
        try:
            exc = None
            def run():
                sys.settrace(delay_has_deadlock)
                event.wait()
                try:
                    import package
                except BaseException as e:
                    nonlocal exc
                    exc = e
                sys.settrace(None)

            for i in range(10):
                event = threading.Event()
                threads = [threading.Thread(target=run) for x in range(2)]
                try:
                    with threading_helper.start_threads(threads, event.set):
                        time.sleep(0)
                finally:
                    sys.modules.pop('package', None)
                    sys.modules.pop('package.submodule', None)
                if exc is not None:
                    raise exc
        finally:
            del sys.path[0]

    @unittest.skipUnless(sys.platform == "win32", "Windows-specific")
    def test_dll_dependency_import(self):
        from _winapi import GetModuleFileName
        dllname = GetModuleFileName(sys.dllhandle)
        pydname = importlib.util.find_spec("_sqlite3").origin
        depname = os.path.join(
            os.path.dirname(pydname),
            "sqlite3{}.dll".format("_d" if "_d" in pydname else ""))

        with os_helper.temp_dir() as tmp:
            tmp2 = os.path.join(tmp, "DLLs")
            os.mkdir(tmp2)

            pyexe = os.path.join(tmp, os.path.basename(sys.executable))
            shutil.copy(sys.executable, pyexe)
            shutil.copy(dllname, tmp)
            for f in glob.glob(os.path.join(glob.escape(sys.prefix), "vcruntime*.dll")):
                shutil.copy(f, tmp)

            shutil.copy(pydname, tmp2)

            env = None
            env = {k.upper(): os.environ[k] for k in os.environ}
            env["PYTHONPATH"] = tmp2 + ";" + STDLIB_DIR

            # Test 1: import with added DLL directory
            subprocess.check_call([
                pyexe, "-Sc", ";".join([
                    "import os",
                    "p = os.add_dll_directory({!r})".format(
                        os.path.dirname(depname)),
                    "import _sqlite3",
                    "p.close"
                ])],
                stderr=subprocess.STDOUT,
                env=env,
                cwd=os.path.dirname(pyexe))

            # Test 2: import with DLL adjacent to PYD
            shutil.copy(depname, tmp2)
            subprocess.check_call([pyexe, "-Sc", "import _sqlite3"],
                                    stderr=subprocess.STDOUT,
                                    env=env,
                                    cwd=os.path.dirname(pyexe))

    def test_issue105979(self):
        # this used to crash
        with self.assertRaises(ImportError) as cm:
            _imp.get_frozen_object("x", b"6\'\xd5Cu\x12")
        self.assertIn("Frozen object named 'x' is invalid",
                      str(cm.exception))


@skip_if_dont_write_bytecode
class FilePermissionTests(unittest.TestCase):
    # tests for file mode on cached .pyc files

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    @unittest.skipIf(
        is_emscripten or is_wasi,
        "Emscripten's/WASI's umask is a stub."
    )
    def test_creation_mode(self):
        mask = 0o022
        with temp_umask(mask), ready_to_import() as (name, path):
            cached_path = importlib.util.cache_from_source(path)
            module = __import__(name)
            if not os.path.exists(cached_path):
                self.fail("__import__ did not result in creation of "
                          "a .pyc file")
            stat_info = os.stat(cached_path)

        # Check that the umask is respected, and the executable bits
        # aren't set.
        self.assertEqual(oct(stat.S_IMODE(stat_info.st_mode)),
                         oct(0o666 & ~mask))

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    @os_helper.skip_unless_working_chmod
    def test_cached_mode_issue_2051(self):
        # permissions of .pyc should match those of .py, regardless of mask
        mode = 0o600
        with temp_umask(0o022), ready_to_import() as (name, path):
            cached_path = importlib.util.cache_from_source(path)
            os.chmod(path, mode)
            __import__(name)
            if not os.path.exists(cached_path):
                self.fail("__import__ did not result in creation of "
                          "a .pyc file")
            stat_info = os.stat(cached_path)

        self.assertEqual(oct(stat.S_IMODE(stat_info.st_mode)), oct(mode))

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    @os_helper.skip_unless_working_chmod
    def test_cached_readonly(self):
        mode = 0o400
        with temp_umask(0o022), ready_to_import() as (name, path):
            cached_path = importlib.util.cache_from_source(path)
            os.chmod(path, mode)
            __import__(name)
            if not os.path.exists(cached_path):
                self.fail("__import__ did not result in creation of "
                          "a .pyc file")
            stat_info = os.stat(cached_path)

        expected = mode | 0o200 # Account for fix for issue #6074
        self.assertEqual(oct(stat.S_IMODE(stat_info.st_mode)), oct(expected))

    def test_pyc_always_writable(self):
        # Initially read-only .pyc files on Windows used to cause problems
        # with later updates, see issue #6074 for details
        with ready_to_import() as (name, path):
            # Write a Python file, make it read-only and import it
            with open(path, 'w', encoding='utf-8') as f:
                f.write("x = 'original'\n")
            # Tweak the mtime of the source to ensure pyc gets updated later
            s = os.stat(path)
            os.utime(path, (s.st_atime, s.st_mtime-100000000))
            os.chmod(path, 0o400)
            m = __import__(name)
            self.assertEqual(m.x, 'original')
            # Change the file and then reimport it
            os.chmod(path, 0o600)
            with open(path, 'w', encoding='utf-8') as f:
                f.write("x = 'rewritten'\n")
            unload(name)
            importlib.invalidate_caches()
            m = __import__(name)
            self.assertEqual(m.x, 'rewritten')
            # Now delete the source file and check the pyc was rewritten
            unlink(path)
            unload(name)
            importlib.invalidate_caches()
            bytecode_only = path + "c"
            os.rename(importlib.util.cache_from_source(path), bytecode_only)
            m = __import__(name)
            self.assertEqual(m.x, 'rewritten')


class PycRewritingTests(unittest.TestCase):
    # Test that the `co_filename` attribute on code objects always points
    # to the right file, even when various things happen (e.g. both the .py
    # and the .pyc file are renamed).

    module_name = "unlikely_module_name"
    module_source = """
import sys
code_filename = sys._getframe().f_code.co_filename
module_filename = __file__
constant = 1
def func():
    pass
func_filename = func.__code__.co_filename
"""
    dir_name = os.path.abspath(TESTFN)
    file_name = os.path.join(dir_name, module_name) + os.extsep + "py"
    compiled_name = importlib.util.cache_from_source(file_name)

    def setUp(self):
        self.sys_path = sys.path[:]
        self.orig_module = sys.modules.pop(self.module_name, None)
        os.mkdir(self.dir_name)
        with open(self.file_name, "w", encoding='utf-8') as f:
            f.write(self.module_source)
        sys.path.insert(0, self.dir_name)
        importlib.invalidate_caches()

    def tearDown(self):
        sys.path[:] = self.sys_path
        if self.orig_module is not None:
            sys.modules[self.module_name] = self.orig_module
        else:
            unload(self.module_name)
        unlink(self.file_name)
        unlink(self.compiled_name)
        rmtree(self.dir_name)

    def import_module(self):
        ns = globals()
        __import__(self.module_name, ns, ns)
        return sys.modules[self.module_name]

    def test_basics(self):
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.file_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)
        del sys.modules[self.module_name]
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.file_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)

    def test_incorrect_code_name(self):
        py_compile.compile(self.file_name, dfile="another_module.py")
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.file_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)

    def test_module_without_source(self):
        target = "another_module.py"
        py_compile.compile(self.file_name, dfile=target)
        os.remove(self.file_name)
        pyc_file = make_legacy_pyc(self.file_name)
        importlib.invalidate_caches()
        mod = self.import_module()
        self.assertEqual(mod.module_filename, pyc_file)
        self.assertEqual(mod.code_filename, target)
        self.assertEqual(mod.func_filename, target)

    def test_foreign_code(self):
        py_compile.compile(self.file_name)
        with open(self.compiled_name, "rb") as f:
            header = f.read(16)
            code = marshal.load(f)
        constants = list(code.co_consts)
        foreign_code = importlib.import_module.__code__
        pos = constants.index(1)
        constants[pos] = foreign_code
        code = code.replace(co_consts=tuple(constants))
        with open(self.compiled_name, "wb") as f:
            f.write(header)
            marshal.dump(code, f)
        mod = self.import_module()
        self.assertEqual(mod.constant.co_filename, foreign_code.co_filename)


class PathsTests(unittest.TestCase):
    SAMPLES = ('test', 'test\u00e4\u00f6\u00fc\u00df', 'test\u00e9\u00e8',
               'test\u00b0\u00b3\u00b2')
    path = TESTFN

    def setUp(self):
        os.mkdir(self.path)
        self.syspath = sys.path[:]

    def tearDown(self):
        rmtree(self.path)
        sys.path[:] = self.syspath

    # Regression test for http://bugs.python.org/issue1293.
    def test_trailing_slash(self):
        with open(os.path.join(self.path, 'test_trailing_slash.py'),
                  'w', encoding='utf-8') as f:
            f.write("testdata = 'test_trailing_slash'")
        sys.path.append(self.path+'/')
        mod = __import__("test_trailing_slash")
        self.assertEqual(mod.testdata, 'test_trailing_slash')
        unload("test_trailing_slash")

    # Regression test for http://bugs.python.org/issue3677.
    @unittest.skipUnless(sys.platform == 'win32', 'Windows-specific')
    def test_UNC_path(self):
        with open(os.path.join(self.path, 'test_unc_path.py'), 'w') as f:
            f.write("testdata = 'test_unc_path'")
        importlib.invalidate_caches()
        # Create the UNC path, like \\myhost\c$\foo\bar.
        path = os.path.abspath(self.path)
        import socket
        hn = socket.gethostname()
        drive = path[0]
        unc = "\\\\%s\\%s$"%(hn, drive)
        unc += path[2:]
        try:
            os.listdir(unc)
        except OSError as e:
            if e.errno in (errno.EPERM, errno.EACCES, errno.ENOENT):
                # See issue #15338
                self.skipTest("cannot access administrative share %r" % (unc,))
            raise
        sys.path.insert(0, unc)
        try:
            mod = __import__("test_unc_path")
        except ImportError as e:
            self.fail("could not import 'test_unc_path' from %r: %r"
                      % (unc, e))
        self.assertEqual(mod.testdata, 'test_unc_path')
        self.assertTrue(mod.__file__.startswith(unc), mod.__file__)
        unload("test_unc_path")


class RelativeImportTests(unittest.TestCase):

    def tearDown(self):
        unload("test.relimport")
    setUp = tearDown

    def test_relimport_star(self):
        # This will import * from .test_import.
        from .. import relimport
        self.assertTrue(hasattr(relimport, "RelativeImportTests"))

    def test_issue3221(self):
        # Note for mergers: the 'absolute' tests from the 2.x branch
        # are missing in Py3k because implicit relative imports are
        # a thing of the past
        #
        # Regression test for http://bugs.python.org/issue3221.
        def check_relative():
            exec("from . import relimport", ns)

        # Check relative import OK with __package__ and __name__ correct
        ns = dict(__package__='test', __name__='test.notarealmodule')
        check_relative()

        # Check relative import OK with only __name__ wrong
        ns = dict(__package__='test', __name__='notarealpkg.notarealmodule')
        check_relative()

        # Check relative import fails with only __package__ wrong
        ns = dict(__package__='foo', __name__='test.notarealmodule')
        self.assertRaises(ModuleNotFoundError, check_relative)

        # Check relative import fails with __package__ and __name__ wrong
        ns = dict(__package__='foo', __name__='notarealpkg.notarealmodule')
        self.assertRaises(ModuleNotFoundError, check_relative)

        # Check relative import fails with package set to a non-string
        ns = dict(__package__=object())
        self.assertRaises(TypeError, check_relative)

    def test_parentless_import_shadowed_by_global(self):
        # Test as if this were done from the REPL where this error most commonly occurs (bpo-37409).
        script_helper.assert_python_failure('-W', 'ignore', '-c',
            "foo = 1; from . import foo")

    def test_absolute_import_without_future(self):
        # If explicit relative import syntax is used, then do not try
        # to perform an absolute import in the face of failure.
        # Issue #7902.
        with self.assertRaises(ImportError):
            from .os import sep
            self.fail("explicit relative import triggered an "
                      "implicit absolute import")

    def test_import_from_non_package(self):
        path = os.path.join(os.path.dirname(__file__), 'data', 'package2')
        with uncache('submodule1', 'submodule2'), DirsOnSysPath(path):
            with self.assertRaises(ImportError):
                import submodule1
            self.assertNotIn('submodule1', sys.modules)
            self.assertNotIn('submodule2', sys.modules)

    def test_import_from_unloaded_package(self):
        with uncache('package2', 'package2.submodule1', 'package2.submodule2'), \
             DirsOnSysPath(os.path.join(os.path.dirname(__file__), 'data')):
            import package2.submodule1
            package2.submodule1.submodule2


class OverridingImportBuiltinTests(unittest.TestCase):
    def test_override_builtin(self):
        # Test that overriding builtins.__import__ can bypass sys.modules.
        import os

        def foo():
            import os
            return os
        self.assertEqual(foo(), os)  # Quick sanity check.

        with swap_attr(builtins, "__import__", lambda *x: 5):
            self.assertEqual(foo(), 5)

        # Test what happens when we shadow __import__ in globals(); this
        # currently does not impact the import process, but if this changes,
        # other code will need to change, so keep this test as a tripwire.
        with swap_item(globals(), "__import__", lambda *x: 5):
            self.assertEqual(foo(), os)


class PycacheTests(unittest.TestCase):
    # Test the various PEP 3147/488-related behaviors.

    def _clean(self):
        forget(TESTFN)
        rmtree('__pycache__')
        unlink(self.source)

    def setUp(self):
        self.source = TESTFN + '.py'
        self._clean()
        with open(self.source, 'w', encoding='utf-8') as fp:
            print('# This is a test file written by test_import.py', file=fp)
        sys.path.insert(0, os.curdir)
        importlib.invalidate_caches()

    def tearDown(self):
        assert sys.path[0] == os.curdir, 'Unexpected sys.path[0]'
        del sys.path[0]
        self._clean()

    @skip_if_dont_write_bytecode
    def test_import_pyc_path(self):
        self.assertFalse(os.path.exists('__pycache__'))
        __import__(TESTFN)
        self.assertTrue(os.path.exists('__pycache__'))
        pyc_path = importlib.util.cache_from_source(self.source)
        self.assertTrue(os.path.exists(pyc_path),
                        'bytecode file {!r} for {!r} does not '
                        'exist'.format(pyc_path, TESTFN))

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    @skip_if_dont_write_bytecode
    @os_helper.skip_unless_working_chmod
    @os_helper.skip_if_dac_override
    @unittest.skipIf(is_emscripten, "umask is a stub")
    def test_unwritable_directory(self):
        # When the umask causes the new __pycache__ directory to be
        # unwritable, the import still succeeds but no .pyc file is written.
        with temp_umask(0o222):
            __import__(TESTFN)
        self.assertTrue(os.path.exists('__pycache__'))
        pyc_path = importlib.util.cache_from_source(self.source)
        self.assertFalse(os.path.exists(pyc_path),
                        'bytecode file {!r} for {!r} '
                        'exists'.format(pyc_path, TESTFN))

    @skip_if_dont_write_bytecode
    def test_missing_source(self):
        # With PEP 3147 cache layout, removing the source but leaving the pyc
        # file does not satisfy the import.
        __import__(TESTFN)
        pyc_file = importlib.util.cache_from_source(self.source)
        self.assertTrue(os.path.exists(pyc_file))
        os.remove(self.source)
        forget(TESTFN)
        importlib.invalidate_caches()
        self.assertRaises(ImportError, __import__, TESTFN)

    @skip_if_dont_write_bytecode
    def test_missing_source_legacy(self):
        # Like test_missing_source() except that for backward compatibility,
        # when the pyc file lives where the py file would have been (and named
        # without the tag), it is importable.  The __file__ of the imported
        # module is the pyc location.
        __import__(TESTFN)
        # pyc_file gets removed in _clean() via tearDown().
        pyc_file = make_legacy_pyc(self.source)
        os.remove(self.source)
        unload(TESTFN)
        importlib.invalidate_caches()
        m = __import__(TESTFN)
        try:
            self.assertEqual(m.__file__,
                             os.path.join(os.getcwd(), os.path.relpath(pyc_file)))
        finally:
            os.remove(pyc_file)

    def test___cached__(self):
        # Modules now also have an __cached__ that points to the pyc file.
        m = __import__(TESTFN)
        pyc_file = importlib.util.cache_from_source(TESTFN + '.py')
        self.assertEqual(m.__cached__, os.path.join(os.getcwd(), pyc_file))

    @skip_if_dont_write_bytecode
    def test___cached___legacy_pyc(self):
        # Like test___cached__() except that for backward compatibility,
        # when the pyc file lives where the py file would have been (and named
        # without the tag), it is importable.  The __cached__ of the imported
        # module is the pyc location.
        __import__(TESTFN)
        # pyc_file gets removed in _clean() via tearDown().
        pyc_file = make_legacy_pyc(self.source)
        os.remove(self.source)
        unload(TESTFN)
        importlib.invalidate_caches()
        m = __import__(TESTFN)
        self.assertEqual(m.__cached__,
                         os.path.join(os.getcwd(), os.path.relpath(pyc_file)))

    @skip_if_dont_write_bytecode
    def test_package___cached__(self):
        # Like test___cached__ but for packages.
        def cleanup():
            rmtree('pep3147')
            unload('pep3147.foo')
            unload('pep3147')
        os.mkdir('pep3147')
        self.addCleanup(cleanup)
        # Touch the __init__.py
        with open(os.path.join('pep3147', '__init__.py'), 'wb'):
            pass
        with open(os.path.join('pep3147', 'foo.py'), 'wb'):
            pass
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        init_pyc = importlib.util.cache_from_source(
            os.path.join('pep3147', '__init__.py'))
        self.assertEqual(m.__cached__, os.path.join(os.getcwd(), init_pyc))
        foo_pyc = importlib.util.cache_from_source(os.path.join('pep3147', 'foo.py'))
        self.assertEqual(sys.modules['pep3147.foo'].__cached__,
                         os.path.join(os.getcwd(), foo_pyc))

    def test_package___cached___from_pyc(self):
        # Like test___cached__ but ensuring __cached__ when imported from a
        # PEP 3147 pyc file.
        def cleanup():
            rmtree('pep3147')
            unload('pep3147.foo')
            unload('pep3147')
        os.mkdir('pep3147')
        self.addCleanup(cleanup)
        # Touch the __init__.py
        with open(os.path.join('pep3147', '__init__.py'), 'wb'):
            pass
        with open(os.path.join('pep3147', 'foo.py'), 'wb'):
            pass
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        unload('pep3147.foo')
        unload('pep3147')
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        init_pyc = importlib.util.cache_from_source(
            os.path.join('pep3147', '__init__.py'))
        self.assertEqual(m.__cached__, os.path.join(os.getcwd(), init_pyc))
        foo_pyc = importlib.util.cache_from_source(os.path.join('pep3147', 'foo.py'))
        self.assertEqual(sys.modules['pep3147.foo'].__cached__,
                         os.path.join(os.getcwd(), foo_pyc))

    def test_recompute_pyc_same_second(self):
        # Even when the source file doesn't change timestamp, a change in
        # source size is enough to trigger recomputation of the pyc file.
        __import__(TESTFN)
        unload(TESTFN)
        with open(self.source, 'a', encoding='utf-8') as fp:
            print("x = 5", file=fp)
        m = __import__(TESTFN)
        self.assertEqual(m.x, 5)


class TestSymbolicallyLinkedPackage(unittest.TestCase):
    package_name = 'sample'
    tagged = package_name + '-tagged'

    def setUp(self):
        os_helper.rmtree(self.tagged)
        os_helper.rmtree(self.package_name)
        self.orig_sys_path = sys.path[:]

        # create a sample package; imagine you have a package with a tag and
        #  you want to symbolically link it from its untagged name.
        os.mkdir(self.tagged)
        self.addCleanup(os_helper.rmtree, self.tagged)
        init_file = os.path.join(self.tagged, '__init__.py')
        os_helper.create_empty_file(init_file)
        assert os.path.exists(init_file)

        # now create a symlink to the tagged package
        # sample -> sample-tagged
        os.symlink(self.tagged, self.package_name, target_is_directory=True)
        self.addCleanup(os_helper.unlink, self.package_name)
        importlib.invalidate_caches()

        self.assertEqual(os.path.isdir(self.package_name), True)

        assert os.path.isfile(os.path.join(self.package_name, '__init__.py'))

    def tearDown(self):
        sys.path[:] = self.orig_sys_path

    # regression test for issue6727
    @unittest.skipUnless(
        not hasattr(sys, 'getwindowsversion')
        or sys.getwindowsversion() >= (6, 0),
        "Windows Vista or later required")
    @os_helper.skip_unless_symlink
    def test_symlinked_dir_importable(self):
        # make sure sample can only be imported from the current directory.
        sys.path[:] = ['.']
        assert os.path.exists(self.package_name)
        assert os.path.exists(os.path.join(self.package_name, '__init__.py'))

        # Try to import the package
        importlib.import_module(self.package_name)


@cpython_only
class ImportlibBootstrapTests(unittest.TestCase):
    # These tests check that importlib is bootstrapped.

    def test_frozen_importlib(self):
        mod = sys.modules['_frozen_importlib']
        self.assertTrue(mod)

    def test_frozen_importlib_is_bootstrap(self):
        from importlib import _bootstrap
        mod = sys.modules['_frozen_importlib']
        self.assertIs(mod, _bootstrap)
        self.assertEqual(mod.__name__, 'importlib._bootstrap')
        self.assertEqual(mod.__package__, 'importlib')
        self.assertTrue(mod.__file__.endswith('_bootstrap.py'), mod.__file__)

    def test_frozen_importlib_external_is_bootstrap_external(self):
        from importlib import _bootstrap_external
        mod = sys.modules['_frozen_importlib_external']
        self.assertIs(mod, _bootstrap_external)
        self.assertEqual(mod.__name__, 'importlib._bootstrap_external')
        self.assertEqual(mod.__package__, 'importlib')
        self.assertTrue(mod.__file__.endswith('_bootstrap_external.py'), mod.__file__)

    def test_there_can_be_only_one(self):
        # Issue #15386 revealed a tricky loophole in the bootstrapping
        # This test is technically redundant, since the bug caused importing
        # this test module to crash completely, but it helps prove the point
        from importlib import machinery
        mod = sys.modules['_frozen_importlib']
        self.assertIs(machinery.ModuleSpec, mod.ModuleSpec)


@cpython_only
class GetSourcefileTests(unittest.TestCase):

    """Test importlib._bootstrap_external._get_sourcefile() as used by the C API.

    Because of the peculiarities of the need of this function, the tests are
    knowingly whitebox tests.

    """

    def test_get_sourcefile(self):
        # Given a valid bytecode path, return the path to the corresponding
        # source file if it exists.
        with mock.patch('importlib._bootstrap_external._path_isfile') as _path_isfile:
            _path_isfile.return_value = True
            path = TESTFN + '.pyc'
            expect = TESTFN + '.py'
            self.assertEqual(_get_sourcefile(path), expect)

    def test_get_sourcefile_no_source(self):
        # Given a valid bytecode path without a corresponding source path,
        # return the original bytecode path.
        with mock.patch('importlib._bootstrap_external._path_isfile') as _path_isfile:
            _path_isfile.return_value = False
            path = TESTFN + '.pyc'
            self.assertEqual(_get_sourcefile(path), path)

    def test_get_sourcefile_bad_ext(self):
        # Given a path with an invalid bytecode extension, return the
        # bytecode path passed as the argument.
        path = TESTFN + '.bad_ext'
        self.assertEqual(_get_sourcefile(path), path)


class ImportTracebackTests(unittest.TestCase):

    def setUp(self):
        os.mkdir(TESTFN)
        self.old_path = sys.path[:]
        sys.path.insert(0, TESTFN)

    def tearDown(self):
        sys.path[:] = self.old_path
        rmtree(TESTFN)

    def create_module(self, mod, contents, ext=".py"):
        fname = os.path.join(TESTFN, mod + ext)
        with open(fname, "w", encoding='utf-8') as f:
            f.write(contents)
        self.addCleanup(unload, mod)
        importlib.invalidate_caches()
        return fname

    def assert_traceback(self, tb, files):
        deduped_files = []
        while tb:
            code = tb.tb_frame.f_code
            fn = code.co_filename
            if not deduped_files or fn != deduped_files[-1]:
                deduped_files.append(fn)
            tb = tb.tb_next
        self.assertEqual(len(deduped_files), len(files), deduped_files)
        for fn, pat in zip(deduped_files, files):
            self.assertIn(pat, fn)

    def test_nonexistent_module(self):
        try:
            # assertRaises() clears __traceback__
            import nonexistent_xyzzy
        except ImportError as e:
            tb = e.__traceback__
        else:
            self.fail("ImportError should have been raised")
        self.assert_traceback(tb, [__file__])

    def test_nonexistent_module_nested(self):
        self.create_module("foo", "import nonexistent_xyzzy")
        try:
            import foo
        except ImportError as e:
            tb = e.__traceback__
        else:
            self.fail("ImportError should have been raised")
        self.assert_traceback(tb, [__file__, 'foo.py'])

    def test_exec_failure(self):
        self.create_module("foo", "1/0")
        try:
            import foo
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ZeroDivisionError should have been raised")
        self.assert_traceback(tb, [__file__, 'foo.py'])

    def test_exec_failure_nested(self):
        self.create_module("foo", "import bar")
        self.create_module("bar", "1/0")
        try:
            import foo
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ZeroDivisionError should have been raised")
        self.assert_traceback(tb, [__file__, 'foo.py', 'bar.py'])

    # A few more examples from issue #15425
    def test_syntax_error(self):
        self.create_module("foo", "invalid syntax is invalid")
        try:
            import foo
        except SyntaxError as e:
            tb = e.__traceback__
        else:
            self.fail("SyntaxError should have been raised")
        self.assert_traceback(tb, [__file__])

    def _setup_broken_package(self, parent, child):
        pkg_name = "_parent_foo"
        self.addCleanup(unload, pkg_name)
        pkg_path = os.path.join(TESTFN, pkg_name)
        os.mkdir(pkg_path)
        # Touch the __init__.py
        init_path = os.path.join(pkg_path, '__init__.py')
        with open(init_path, 'w', encoding='utf-8') as f:
            f.write(parent)
        bar_path = os.path.join(pkg_path, 'bar.py')
        with open(bar_path, 'w', encoding='utf-8') as f:
            f.write(child)
        importlib.invalidate_caches()
        return init_path, bar_path

    def test_broken_submodule(self):
        init_path, bar_path = self._setup_broken_package("", "1/0")
        try:
            import _parent_foo.bar
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ZeroDivisionError should have been raised")
        self.assert_traceback(tb, [__file__, bar_path])

    def test_broken_from(self):
        init_path, bar_path = self._setup_broken_package("", "1/0")
        try:
            from _parent_foo import bar
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ImportError should have been raised")
        self.assert_traceback(tb, [__file__, bar_path])

    def test_broken_parent(self):
        init_path, bar_path = self._setup_broken_package("1/0", "")
        try:
            import _parent_foo.bar
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ZeroDivisionError should have been raised")
        self.assert_traceback(tb, [__file__, init_path])

    def test_broken_parent_from(self):
        init_path, bar_path = self._setup_broken_package("1/0", "")
        try:
            from _parent_foo import bar
        except ZeroDivisionError as e:
            tb = e.__traceback__
        else:
            self.fail("ZeroDivisionError should have been raised")
        self.assert_traceback(tb, [__file__, init_path])

    @cpython_only
    def test_import_bug(self):
        # We simulate a bug in importlib and check that it's not stripped
        # away from the traceback.
        self.create_module("foo", "")
        importlib = sys.modules['_frozen_importlib_external']
        if 'load_module' in vars(importlib.SourceLoader):
            old_exec_module = importlib.SourceLoader.exec_module
        else:
            old_exec_module = None
        try:
            def exec_module(*args):
                1/0
            importlib.SourceLoader.exec_module = exec_module
            try:
                import foo
            except ZeroDivisionError as e:
                tb = e.__traceback__
            else:
                self.fail("ZeroDivisionError should have been raised")
            self.assert_traceback(tb, [__file__, '<frozen importlib', __file__])
        finally:
            if old_exec_module is None:
                del importlib.SourceLoader.exec_module
            else:
                importlib.SourceLoader.exec_module = old_exec_module

    @unittest.skipUnless(TESTFN_UNENCODABLE, 'need TESTFN_UNENCODABLE')
    def test_unencodable_filename(self):
        # Issue #11619: The Python parser and the import machinery must not
        # encode filenames, especially on Windows
        pyname = script_helper.make_script('', TESTFN_UNENCODABLE, 'pass')
        self.addCleanup(unlink, pyname)
        name = pyname[:-3]
        script_helper.assert_python_ok("-c", "mod = __import__(%a)" % name,
                                       __isolated=False)


class CircularImportTests(unittest.TestCase):

    """See the docstrings of the modules being imported for the purpose of the
    test."""

    def tearDown(self):
        """Make sure no modules pre-exist in sys.modules which are being used to
        test."""
        for key in list(sys.modules.keys()):
            if key.startswith('test.test_import.data.circular_imports'):
                del sys.modules[key]

    def test_direct(self):
        try:
            import test.test_import.data.circular_imports.basic
        except ImportError:
            self.fail('circular import through relative imports failed')

    def test_indirect(self):
        try:
            import test.test_import.data.circular_imports.indirect
        except ImportError:
            self.fail('relative import in module contributing to circular '
                      'import failed')

    def test_subpackage(self):
        try:
            import test.test_import.data.circular_imports.subpackage
        except ImportError:
            self.fail('circular import involving a subpackage failed')

    def test_rebinding(self):
        try:
            import test.test_import.data.circular_imports.rebinding as rebinding
        except ImportError:
            self.fail('circular import with rebinding of module attribute failed')
        from test.test_import.data.circular_imports.subpkg import util
        self.assertIs(util.util, rebinding.util)

    def test_binding(self):
        try:
            import test.test_import.data.circular_imports.binding
        except ImportError:
            self.fail('circular import with binding a submodule to a name failed')

    def test_crossreference1(self):
        import test.test_import.data.circular_imports.use
        import test.test_import.data.circular_imports.source

    def test_crossreference2(self):
        with self.assertRaises(AttributeError) as cm:
            import test.test_import.data.circular_imports.source
        errmsg = str(cm.exception)
        self.assertIn('test.test_import.data.circular_imports.source', errmsg)
        self.assertIn('spam', errmsg)
        self.assertIn('partially initialized module', errmsg)
        self.assertIn('circular import', errmsg)

    def test_circular_from_import(self):
        with self.assertRaises(ImportError) as cm:
            import test.test_import.data.circular_imports.from_cycle1
        self.assertIn(
            "cannot import name 'b' from partially initialized module "
            "'test.test_import.data.circular_imports.from_cycle1' "
            "(most likely due to a circular import)",
            str(cm.exception),
        )

    def test_circular_import(self):
        with self.assertRaisesRegex(
            AttributeError,
            r"partially initialized module 'test.test_import.data.circular_imports.import_cycle' "
            r"from '.*' has no attribute 'some_attribute' \(most likely due to a circular import\)"
        ):
            import test.test_import.data.circular_imports.import_cycle

    def test_absolute_circular_submodule(self):
        with self.assertRaises(AttributeError) as cm:
            import test.test_import.data.circular_imports.subpkg2.parent
        self.assertIn(
            "cannot access submodule 'parent' of module "
            "'test.test_import.data.circular_imports.subpkg2' "
            "(most likely due to a circular import)",
            str(cm.exception),
        )

    def test_unwritable_module(self):
        self.addCleanup(unload, "test.test_import.data.unwritable")
        self.addCleanup(unload, "test.test_import.data.unwritable.x")

        import test.test_import.data.unwritable as unwritable
        with self.assertWarns(ImportWarning):
            from test.test_import.data.unwritable import x

        self.assertNotEqual(type(unwritable), ModuleType)
        self.assertEqual(type(x), ModuleType)
        with self.assertRaises(AttributeError):
            unwritable.x = 42


class SubinterpImportTests(unittest.TestCase):

    RUN_KWARGS = dict(
        allow_fork=False,
        allow_exec=False,
        allow_threads=True,
        allow_daemon_threads=False,
        # Isolation-related config values aren't included here.
    )
    ISOLATED = dict(
        use_main_obmalloc=False,
        gil=2,
    )
    NOT_ISOLATED = {k: not v for k, v in ISOLATED.items()}
    NOT_ISOLATED['gil'] = 1

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def pipe(self):
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        if hasattr(os, 'set_blocking'):
            os.set_blocking(r, False)
        return (r, w)

    def import_script(self, name, fd, filename=None, check_override=None):
        override_text = ''
        if check_override is not None:
            override_text = f'''
                import _imp
                _imp._override_multi_interp_extensions_check({check_override})
                '''
        if filename:
            return textwrap.dedent(f'''
                from importlib.util import spec_from_loader, module_from_spec
                from importlib.machinery import ExtensionFileLoader
                import os, sys
                {override_text}
                loader = ExtensionFileLoader({name!r}, {filename!r})
                spec = spec_from_loader({name!r}, loader)
                try:
                    module = module_from_spec(spec)
                    loader.exec_module(module)
                except ImportError as exc:
                    text = 'ImportError: ' + str(exc)
                else:
                    text = 'okay'
                os.write({fd}, text.encode('utf-8'))
                ''')
        else:
            return textwrap.dedent(f'''
                import os, sys
                {override_text}
                try:
                    import {name}
                except ImportError as exc:
                    text = 'ImportError: ' + str(exc)
                else:
                    text = 'okay'
                os.write({fd}, text.encode('utf-8'))
                ''')

    def run_here(self, name, filename=None, *,
                 check_singlephase_setting=False,
                 check_singlephase_override=None,
                 isolated=False,
                 ):
        """
        Try importing the named module in a subinterpreter.

        The subinterpreter will be in the current process.
        The module will have already been imported in the main interpreter.
        Thus, for extension/builtin modules, the module definition will
        have been loaded already and cached globally.

        "check_singlephase_setting" determines whether or not
        the interpreter will be configured to check for modules
        that are not compatible with use in multiple interpreters.

        This should always return "okay" for all modules if the
        setting is False (with no override).
        """
        __import__(name)

        kwargs = dict(
            **self.RUN_KWARGS,
            **(self.ISOLATED if isolated else self.NOT_ISOLATED),
            check_multi_interp_extensions=check_singlephase_setting,
        )

        r, w = self.pipe()
        script = self.import_script(name, w, filename,
                                    check_singlephase_override)

        ret = run_in_subinterp_with_config(script, **kwargs)
        self.assertEqual(ret, 0)
        return os.read(r, 100)

    def check_compatible_here(self, name, filename=None, *,
                              strict=False,
                              isolated=False,
                              ):
        # Verify that the named module may be imported in a subinterpreter.
        # (See run_here() for more info.)
        out = self.run_here(name, filename,
                            check_singlephase_setting=strict,
                            isolated=isolated,
                            )
        self.assertEqual(out, b'okay')

    def check_incompatible_here(self, name, filename=None, *, isolated=False):
        # Differences from check_compatible_here():
        #  * verify that import fails
        #  * "strict" is always True
        out = self.run_here(name, filename,
                            check_singlephase_setting=True,
                            isolated=isolated,
                            )
        self.assertEqual(
            out.decode('utf-8'),
            f'ImportError: module {name} does not support loading in subinterpreters',
        )

    def check_compatible_fresh(self, name, *, strict=False, isolated=False):
        # Differences from check_compatible_here():
        #  * subinterpreter in a new process
        #  * module has never been imported before in that process
        #  * this tests importing the module for the first time
        kwargs = dict(
            **self.RUN_KWARGS,
            **(self.ISOLATED if isolated else self.NOT_ISOLATED),
            check_multi_interp_extensions=strict,
        )
        _, out, err = script_helper.assert_python_ok('-c', textwrap.dedent(f'''
            import _testinternalcapi, sys
            assert (
                {name!r} in sys.builtin_module_names or
                {name!r} not in sys.modules
            ), repr({name!r})
            ret = _testinternalcapi.run_in_subinterp_with_config(
                {self.import_script(name, "sys.stdout.fileno()")!r},
                **{kwargs},
            )
            assert ret == 0, ret
            '''))
        self.assertEqual(err, b'')
        self.assertEqual(out, b'okay')

    def check_incompatible_fresh(self, name, *, isolated=False):
        # Differences from check_compatible_fresh():
        #  * verify that import fails
        #  * "strict" is always True
        kwargs = dict(
            **self.RUN_KWARGS,
            **(self.ISOLATED if isolated else self.NOT_ISOLATED),
            check_multi_interp_extensions=True,
        )
        _, out, err = script_helper.assert_python_ok('-c', textwrap.dedent(f'''
            import _testinternalcapi, sys
            assert {name!r} not in sys.modules, {name!r}
            ret = _testinternalcapi.run_in_subinterp_with_config(
                {self.import_script(name, "sys.stdout.fileno()")!r},
                **{kwargs},
            )
            assert ret == 0, ret
            '''))
        self.assertEqual(err, b'')
        self.assertEqual(
            out.decode('utf-8'),
            f'ImportError: module {name} does not support loading in subinterpreters',
        )

    def test_builtin_compat(self):
        # For now we avoid using sys or builtins
        # since they still don't implement multi-phase init.
        module = '_imp'
        require_builtin(module)
        with self.subTest(f'{module}: not strict'):
            self.check_compatible_here(module, strict=False)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_compatible_here(module, strict=True)

    @cpython_only
    def test_frozen_compat(self):
        module = '_frozen_importlib'
        require_frozen(module, skip=True)
        if __import__(module).__spec__.origin != 'frozen':
            raise unittest.SkipTest(f'{module} is unexpectedly not frozen')
        with self.subTest(f'{module}: not strict'):
            self.check_compatible_here(module, strict=False)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_compatible_here(module, strict=True)

    @requires_singlephase_init
    def test_single_init_extension_compat(self):
        module = '_testsinglephase'
        require_extension(module)
        with self.subTest(f'{module}: not strict'):
            self.check_compatible_here(module, strict=False)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_incompatible_here(module)
        with self.subTest(f'{module}: strict, fresh'):
            self.check_incompatible_fresh(module)

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_multi_init_extension_compat(self):
        module = '_testmultiphase'
        require_extension(module)
        with self.subTest(f'{module}: not strict'):
            self.check_compatible_here(module, strict=False)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_compatible_here(module, strict=True)
        with self.subTest(f'{module}: strict, fresh'):
            self.check_compatible_fresh(module, strict=True)

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_multi_init_extension_non_isolated_compat(self):
        modname = '_test_non_isolated'
        filename = _testmultiphase.__file__
        loader = ExtensionFileLoader(modname, filename)
        spec = importlib.util.spec_from_loader(modname, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
        sys.modules[modname] = module

        require_extension(module)
        with self.subTest(f'{modname}: isolated'):
            self.check_incompatible_here(modname, filename, isolated=True)
        with self.subTest(f'{modname}: not isolated'):
            self.check_incompatible_here(modname, filename, isolated=False)
        with self.subTest(f'{modname}: not strict'):
            self.check_compatible_here(modname, filename, strict=False)

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_multi_init_extension_per_interpreter_gil_compat(self):
        modname = '_test_shared_gil_only'
        filename = _testmultiphase.__file__
        loader = ExtensionFileLoader(modname, filename)
        spec = importlib.util.spec_from_loader(modname, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
        sys.modules[modname] = module

        require_extension(module)
        with self.subTest(f'{modname}: isolated, strict'):
            self.check_incompatible_here(modname, filename, isolated=True)
        with self.subTest(f'{modname}: not isolated, strict'):
            self.check_compatible_here(modname, filename,
                                       strict=True, isolated=False)
        with self.subTest(f'{modname}: not isolated, not strict'):
            self.check_compatible_here(modname, filename,
                                       strict=False, isolated=False)

    def test_python_compat(self):
        module = 'threading'
        require_pure_python(module)
        with self.subTest(f'{module}: not strict'):
            self.check_compatible_here(module, strict=False)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_compatible_here(module, strict=True)
        with self.subTest(f'{module}: strict, fresh'):
            self.check_compatible_fresh(module, strict=True)

    @requires_singlephase_init
    def test_singlephase_check_with_setting_and_override(self):
        module = '_testsinglephase'
        require_extension(module)

        def check_compatible(setting, override):
            out = self.run_here(
                module,
                check_singlephase_setting=setting,
                check_singlephase_override=override,
            )
            self.assertEqual(out, b'okay')

        def check_incompatible(setting, override):
            out = self.run_here(
                module,
                check_singlephase_setting=setting,
                check_singlephase_override=override,
            )
            self.assertNotEqual(out, b'okay')

        with self.subTest('config: check enabled; override: enabled'):
            check_incompatible(True, 1)
        with self.subTest('config: check enabled; override: use config'):
            check_incompatible(True, 0)
        with self.subTest('config: check enabled; override: disabled'):
            check_compatible(True, -1)

        with self.subTest('config: check disabled; override: enabled'):
            check_incompatible(False, 1)
        with self.subTest('config: check disabled; override: use config'):
            check_compatible(False, 0)
        with self.subTest('config: check disabled; override: disabled'):
            check_compatible(False, -1)

    def test_isolated_config(self):
        module = 'threading'
        require_pure_python(module)
        with self.subTest(f'{module}: strict, not fresh'):
            self.check_compatible_here(module, strict=True, isolated=True)
        with self.subTest(f'{module}: strict, fresh'):
            self.check_compatible_fresh(module, strict=True, isolated=True)

    @requires_subinterpreters
    @requires_singlephase_init
    def test_disallowed_reimport(self):
        # See https://github.com/python/cpython/issues/104621.
        script = textwrap.dedent('''
            import _testsinglephase
            print(_testsinglephase)
            ''')
        interpid = _interpreters.create()
        self.addCleanup(lambda: _interpreters.destroy(interpid))

        excsnap = _interpreters.run_string(interpid, script)
        self.assertIsNot(excsnap, None)

        excsnap = _interpreters.run_string(interpid, script)
        self.assertIsNot(excsnap, None)


class TestSinglePhaseSnapshot(ModuleSnapshot):

    @classmethod
    def from_module(cls, mod):
        self = super().from_module(mod)
        self.summed = mod.sum(1, 2)
        self.lookedup = mod.look_up_self()
        self.lookedup_id = id(self.lookedup)
        self.state_initialized = mod.state_initialized()
        if hasattr(mod, 'initialized_count'):
            self.init_count = mod.initialized_count()
        return self

    SCRIPT_BODY = ModuleSnapshot.SCRIPT_BODY + textwrap.dedent('''
        snapshot['module'].update(dict(
            int_const=mod.int_const,
            str_const=mod.str_const,
            _module_initialized=mod._module_initialized,
        ))
        snapshot.update(dict(
            summed=mod.sum(1, 2),
            lookedup_id=id(mod.look_up_self()),
            state_initialized=mod.state_initialized(),
            init_count=mod.initialized_count(),
            has_spam=hasattr(mod, 'spam'),
            spam=getattr(mod, 'spam', None),
        ))
        ''').rstrip()

    @classmethod
    def parse(cls, text):
        self = super().parse(text)
        if not self.has_spam:
            del self.spam
        del self.has_spam
        return self


@requires_singlephase_init
class SinglephaseInitTests(unittest.TestCase):

    NAME = '_testsinglephase'

    @classmethod
    def setUpClass(cls):
        spec = importlib.util.find_spec(cls.NAME)
        from importlib.machinery import ExtensionFileLoader
        cls.FILE = spec.origin
        cls.LOADER = type(spec.loader)
        assert cls.LOADER is ExtensionFileLoader

        # Start fresh.
        cls.clean_up()

    @classmethod
    def tearDownClass(cls):
        restore__testsinglephase()

    def tearDown(self):
        # Clean up the module.
        self.clean_up()

    @classmethod
    def clean_up(cls):
        name = cls.NAME
        filename = cls.FILE
        if name in sys.modules:
            if hasattr(sys.modules[name], '_clear_globals'):
                assert sys.modules[name].__file__ == filename
                sys.modules[name]._clear_globals()
            del sys.modules[name]
        # Clear all internally cached data for the extension.
        _testinternalcapi.clear_extension(name, filename)

    #########################
    # helpers

    def add_module_cleanup(self, name):
        def clean_up():
            # Clear all internally cached data for the extension.
            _testinternalcapi.clear_extension(name, self.FILE)
        self.addCleanup(clean_up)

    def _load_dynamic(self, name, path):
        """
        Load an extension module.
        """
        # This is essentially copied from the old imp module.
        from importlib._bootstrap import _load
        loader = self.LOADER(name, path)

        # Issue bpo-24748: Skip the sys.modules check in _load_module_shim;
        # always load new extension.
        spec = importlib.util.spec_from_file_location(name, path,
                                                      loader=loader)
        return _load(spec)

    def load(self, name):
        try:
            already_loaded = self.already_loaded
        except AttributeError:
            already_loaded = self.already_loaded = {}
        assert name not in already_loaded
        mod = self._load_dynamic(name, self.FILE)
        self.assertNotIn(mod, already_loaded.values())
        already_loaded[name] = mod
        return types.SimpleNamespace(
            name=name,
            module=mod,
            snapshot=TestSinglePhaseSnapshot.from_module(mod),
        )

    def re_load(self, name, mod):
        assert sys.modules[name] is mod
        assert mod.__dict__ == mod.__dict__
        reloaded = self._load_dynamic(name, self.FILE)
        return types.SimpleNamespace(
            name=name,
            module=reloaded,
            snapshot=TestSinglePhaseSnapshot.from_module(reloaded),
        )

    # subinterpreters

    def add_subinterpreter(self):
        interpid = _interpreters.create(isolated=False)
        def ensure_destroyed():
            try:
                _interpreters.destroy(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass
        self.addCleanup(ensure_destroyed)
        _interpreters.exec(interpid, textwrap.dedent('''
            import sys
            import _testinternalcapi
            '''))
        def clean_up():
            _interpreters.exec(interpid, textwrap.dedent(f'''
                name = {self.NAME!r}
                if name in sys.modules:
                    sys.modules.pop(name)._clear_globals()
                _testinternalcapi.clear_extension(name, {self.FILE!r})
                '''))
            _interpreters.destroy(interpid)
        self.addCleanup(clean_up)
        return interpid

    def import_in_subinterp(self, interpid=None, *,
                            postscript=None,
                            postcleanup=False,
                            ):
        name = self.NAME

        if postcleanup:
            import_ = 'import _testinternalcapi' if interpid is None else ''
            postcleanup = f'''
                {import_}
                mod._clear_globals()
                _testinternalcapi.clear_extension(name, {self.FILE!r})
                '''

        try:
            pipe = self._pipe
        except AttributeError:
            r, w = pipe = self._pipe = os.pipe()
            self.addCleanup(os.close, r)
            self.addCleanup(os.close, w)

        snapshot = TestSinglePhaseSnapshot.from_subinterp(
            name,
            interpid,
            pipe=pipe,
            import_first=True,
            postscript=postscript,
            postcleanup=postcleanup,
        )

        return types.SimpleNamespace(
            name=name,
            module=None,
            snapshot=snapshot,
        )

    # checks

    def check_common(self, loaded):
        isolated = False

        mod = loaded.module
        if not mod:
            # It came from a subinterpreter.
            isolated = True
            mod = loaded.snapshot.module
        # mod.__name__  might not match, but the spec will.
        self.assertEqual(mod.__spec__.name, loaded.name)
        self.assertEqual(mod.__file__, self.FILE)
        self.assertEqual(mod.__spec__.origin, self.FILE)
        if not isolated:
            self.assertTrue(issubclass(mod.error, Exception))
        self.assertEqual(mod.int_const, 1969)
        self.assertEqual(mod.str_const, 'something different')
        self.assertIsInstance(mod._module_initialized, float)
        self.assertGreater(mod._module_initialized, 0)

        snap = loaded.snapshot
        self.assertEqual(snap.summed, 3)
        if snap.state_initialized is not None:
            self.assertIsInstance(snap.state_initialized, float)
            self.assertGreater(snap.state_initialized, 0)
        if isolated:
            # The "looked up" module is interpreter-specific
            # (interp->imports.modules_by_index was set for the module).
            self.assertEqual(snap.lookedup_id, snap.id)
            self.assertEqual(snap.cached_id, snap.id)
            with self.assertRaises(AttributeError):
                snap.spam
        else:
            self.assertIs(snap.lookedup, mod)
            self.assertIs(snap.cached, mod)

    def check_direct(self, loaded):
        # The module has its own PyModuleDef, with a matching name.
        self.assertEqual(loaded.module.__name__, loaded.name)
        self.assertIs(loaded.snapshot.lookedup, loaded.module)

    def check_indirect(self, loaded, orig):
        # The module re-uses another's PyModuleDef, with a different name.
        assert orig is not loaded.module
        assert orig.__name__ != loaded.name
        self.assertNotEqual(loaded.module.__name__, loaded.name)
        self.assertIs(loaded.snapshot.lookedup, loaded.module)

    def check_basic(self, loaded, expected_init_count):
        # m_size == -1
        # The module loads fresh the first time and copies m_copy after.
        snap = loaded.snapshot
        self.assertIsNot(snap.state_initialized, None)
        self.assertIsInstance(snap.init_count, int)
        self.assertGreater(snap.init_count, 0)
        self.assertEqual(snap.init_count, expected_init_count)

    def check_with_reinit(self, loaded):
        # m_size >= 0
        # The module loads fresh every time.
        pass

    def check_fresh(self, loaded):
        """
        The module had not been loaded before (at least since fully reset).
        """
        snap = loaded.snapshot
        # The module's init func was run.
        # A copy of the module's __dict__ was stored in def->m_base.m_copy.
        # The previous m_copy was deleted first.
        # _PyRuntime.imports.extensions was set.
        self.assertEqual(snap.init_count, 1)
        # The global state was initialized.
        # The module attrs were initialized from that state.
        self.assertEqual(snap.module._module_initialized,
                         snap.state_initialized)

    def check_semi_fresh(self, loaded, base, prev):
        """
        The module had been loaded before and then reset
        (but the module global state wasn't).
        """
        snap = loaded.snapshot
        # The module's init func was run again.
        # A copy of the module's __dict__ was stored in def->m_base.m_copy.
        # The previous m_copy was deleted first.
        # The module globals did not get reset.
        self.assertNotEqual(snap.id, base.snapshot.id)
        self.assertNotEqual(snap.id, prev.snapshot.id)
        self.assertEqual(snap.init_count, prev.snapshot.init_count + 1)
        # The global state was updated.
        # The module attrs were initialized from that state.
        self.assertEqual(snap.module._module_initialized,
                         snap.state_initialized)
        self.assertNotEqual(snap.state_initialized,
                            base.snapshot.state_initialized)
        self.assertNotEqual(snap.state_initialized,
                            prev.snapshot.state_initialized)

    def check_copied(self, loaded, base):
        """
        The module had been loaded before and never reset.
        """
        snap = loaded.snapshot
        # The module's init func was not run again.
        # The interpreter copied m_copy, as set by the other interpreter,
        # with objects owned by the other interpreter.
        # The module globals did not get reset.
        self.assertNotEqual(snap.id, base.snapshot.id)
        self.assertEqual(snap.init_count, base.snapshot.init_count)
        # The global state was not updated since the init func did not run.
        # The module attrs were not directly initialized from that state.
        # The state and module attrs still match the previous loading.
        self.assertEqual(snap.module._module_initialized,
                         snap.state_initialized)
        self.assertEqual(snap.state_initialized,
                         base.snapshot.state_initialized)

    #########################
    # the tests

    def test_cleared_globals(self):
        loaded = self.load(self.NAME)
        _testsinglephase = loaded.module
        init_before = _testsinglephase.state_initialized()

        _testsinglephase._clear_globals()
        init_after = _testsinglephase.state_initialized()
        init_count = _testsinglephase.initialized_count()

        self.assertGreater(init_before, 0)
        self.assertEqual(init_after, 0)
        self.assertEqual(init_count, -1)

    def test_variants(self):
        # Exercise the most meaningful variants described in Python/import.c.
        self.maxDiff = None

        # Check the "basic" module.

        name = self.NAME
        expected_init_count = 1
        with self.subTest(name):
            loaded = self.load(name)

            self.check_common(loaded)
            self.check_direct(loaded)
            self.check_basic(loaded, expected_init_count)
        basic = loaded.module

        # Check its indirect variants.

        name = f'{self.NAME}_basic_wrapper'
        self.add_module_cleanup(name)
        expected_init_count += 1
        with self.subTest(name):
            loaded = self.load(name)

            self.check_common(loaded)
            self.check_indirect(loaded, basic)
            self.check_basic(loaded, expected_init_count)

            # Currently PyState_AddModule() always replaces the cached module.
            self.assertIs(basic.look_up_self(), loaded.module)
            self.assertEqual(basic.initialized_count(), expected_init_count)

        # The cached module shouldn't change after this point.
        basic_lookedup = loaded.module

        # Check its direct variant.

        name = f'{self.NAME}_basic_copy'
        self.add_module_cleanup(name)
        expected_init_count += 1
        with self.subTest(name):
            loaded = self.load(name)

            self.check_common(loaded)
            self.check_direct(loaded)
            self.check_basic(loaded, expected_init_count)

            # This should change the cached module for _testsinglephase.
            self.assertIs(basic.look_up_self(), basic_lookedup)
            self.assertEqual(basic.initialized_count(), expected_init_count)

        # Check the non-basic variant that has no state.

        name = f'{self.NAME}_with_reinit'
        self.add_module_cleanup(name)
        with self.subTest(name):
            loaded = self.load(name)

            self.check_common(loaded)
            self.assertIs(loaded.snapshot.state_initialized, None)
            self.check_direct(loaded)
            self.check_with_reinit(loaded)

            # This should change the cached module for _testsinglephase.
            self.assertIs(basic.look_up_self(), basic_lookedup)
            self.assertEqual(basic.initialized_count(), expected_init_count)

        # Check the basic variant that has state.

        name = f'{self.NAME}_with_state'
        self.add_module_cleanup(name)
        with self.subTest(name):
            loaded = self.load(name)
            self.addCleanup(loaded.module._clear_module_state)

            self.check_common(loaded)
            self.assertIsNot(loaded.snapshot.state_initialized, None)
            self.check_direct(loaded)
            self.check_with_reinit(loaded)

            # This should change the cached module for _testsinglephase.
            self.assertIs(basic.look_up_self(), basic_lookedup)
            self.assertEqual(basic.initialized_count(), expected_init_count)

    def test_basic_reloaded(self):
        # m_copy is copied into the existing module object.
        # Global state is not changed.
        self.maxDiff = None

        for name in [
            self.NAME,  # the "basic" module
            f'{self.NAME}_basic_wrapper',  # the indirect variant
            f'{self.NAME}_basic_copy',  # the direct variant
        ]:
            self.add_module_cleanup(name)
            with self.subTest(name):
                loaded = self.load(name)
                reloaded = self.re_load(name, loaded.module)

                self.check_common(loaded)
                self.check_common(reloaded)

                # Make sure the original __dict__ did not get replaced.
                self.assertEqual(id(loaded.module.__dict__),
                                 loaded.snapshot.ns_id)
                self.assertEqual(loaded.snapshot.ns.__dict__,
                                 loaded.module.__dict__)

                self.assertEqual(reloaded.module.__spec__.name, reloaded.name)
                self.assertEqual(reloaded.module.__name__,
                                 reloaded.snapshot.ns.__name__)

                self.assertIs(reloaded.module, loaded.module)
                self.assertIs(reloaded.module.__dict__, loaded.module.__dict__)
                # It only happens to be the same but that's good enough here.
                # We really just want to verify that the re-loaded attrs
                # didn't change.
                self.assertIs(reloaded.snapshot.lookedup,
                              loaded.snapshot.lookedup)
                self.assertEqual(reloaded.snapshot.state_initialized,
                                 loaded.snapshot.state_initialized)
                self.assertEqual(reloaded.snapshot.init_count,
                                 loaded.snapshot.init_count)

                self.assertIs(reloaded.snapshot.cached, reloaded.module)

    def test_with_reinit_reloaded(self):
        # The module's m_init func is run again.
        self.maxDiff = None

        # Keep a reference around.
        basic = self.load(self.NAME)

        for name, has_state in [
            (f'{self.NAME}_with_reinit', False),  # m_size == 0
            (f'{self.NAME}_with_state', True),    # m_size > 0
        ]:
            self.add_module_cleanup(name)
            with self.subTest(name=name, has_state=has_state):
                loaded = self.load(name)
                if has_state:
                    self.addCleanup(loaded.module._clear_module_state)

                reloaded = self.re_load(name, loaded.module)
                if has_state:
                    self.addCleanup(reloaded.module._clear_module_state)

                self.check_common(loaded)
                self.check_common(reloaded)

                # Make sure the original __dict__ did not get replaced.
                self.assertEqual(id(loaded.module.__dict__),
                                 loaded.snapshot.ns_id)
                self.assertEqual(loaded.snapshot.ns.__dict__,
                                 loaded.module.__dict__)

                self.assertEqual(reloaded.module.__spec__.name, reloaded.name)
                self.assertEqual(reloaded.module.__name__,
                                 reloaded.snapshot.ns.__name__)

                self.assertIsNot(reloaded.module, loaded.module)
                self.assertNotEqual(reloaded.module.__dict__,
                                    loaded.module.__dict__)
                self.assertIs(reloaded.snapshot.lookedup, reloaded.module)
                if loaded.snapshot.state_initialized is None:
                    self.assertIs(reloaded.snapshot.state_initialized, None)
                else:
                    self.assertGreater(reloaded.snapshot.state_initialized,
                                       loaded.snapshot.state_initialized)

                self.assertIs(reloaded.snapshot.cached, reloaded.module)

    # Currently, for every single-phrase init module loaded
    # in multiple interpreters, those interpreters share a
    # PyModuleDef for that object, which can be a problem.
    # Also, we test with a single-phase module that has global state,
    # which is shared by all interpreters.

    @requires_subinterpreters
    def test_basic_multiple_interpreters_main_no_reset(self):
        # without resetting; already loaded in main interpreter

        # At this point:
        #  * alive in 0 interpreters
        #  * module def may or may not be loaded already
        #  * module def not in _PyRuntime.imports.extensions
        #  * mod init func has not run yet (since reset, at least)
        #  * m_copy not set (hasn't been loaded yet or already cleared)
        #  * module's global state has not been initialized yet
        #    (or already cleared)

        main_loaded = self.load(self.NAME)
        _testsinglephase = main_loaded.module
        # Attrs set after loading are not in m_copy.
        _testsinglephase.spam = 'spam, spam, spam, spam, eggs, and spam'

        self.check_common(main_loaded)
        self.check_fresh(main_loaded)

        interpid1 = self.add_subinterpreter()
        interpid2 = self.add_subinterpreter()

        # At this point:
        #  * alive in 1 interpreter (main)
        #  * module def in _PyRuntime.imports.extensions
        #  * mod init func ran for the first time (since reset, at least)
        #  * m_copy was copied from the main interpreter (was NULL)
        #  * module's global state was initialized

        # Use an interpreter that gets destroyed right away.
        loaded = self.import_in_subinterp()
        self.check_common(loaded)
        self.check_copied(loaded, main_loaded)

        # At this point:
        #  * alive in 1 interpreter (main)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy is NULL (claered when the interpreter was destroyed)
        #    (was from main interpreter)
        #  * module's global state was updated, not reset

        # Use a subinterpreter that sticks around.
        loaded = self.import_in_subinterp(interpid1)
        self.check_common(loaded)
        self.check_copied(loaded, main_loaded)

        # At this point:
        #  * alive in 2 interpreters (main, interp1)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp1
        #  * module's global state was updated, not reset

        # Use a subinterpreter while the previous one is still alive.
        loaded = self.import_in_subinterp(interpid2)
        self.check_common(loaded)
        self.check_copied(loaded, main_loaded)

        # At this point:
        #  * alive in 3 interpreters (main, interp1, interp2)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp2 (was from interp1)
        #  * module's global state was updated, not reset

    @no_rerun(reason="rerun not possible; module state is never cleared (see gh-102251)")
    @requires_subinterpreters
    def test_basic_multiple_interpreters_deleted_no_reset(self):
        # without resetting; already loaded in a deleted interpreter

        if Py_TRACE_REFS:
            # It's a Py_TRACE_REFS build.
            # This test breaks interpreter isolation a little,
            # which causes problems on Py_TRACE_REF builds.
            raise unittest.SkipTest('crashes on Py_TRACE_REFS builds')

        # At this point:
        #  * alive in 0 interpreters
        #  * module def may or may not be loaded already
        #  * module def not in _PyRuntime.imports.extensions
        #  * mod init func has not run yet (since reset, at least)
        #  * m_copy not set (hasn't been loaded yet or already cleared)
        #  * module's global state has not been initialized yet
        #    (or already cleared)

        interpid1 = self.add_subinterpreter()
        interpid2 = self.add_subinterpreter()

        # First, load in the main interpreter but then completely clear it.
        loaded_main = self.load(self.NAME)
        loaded_main.module._clear_globals()
        _testinternalcapi.clear_extension(self.NAME, self.FILE)

        # At this point:
        #  * alive in 0 interpreters
        #  * module def loaded already
        #  * module def was in _PyRuntime.imports.extensions, but cleared
        #  * mod init func ran for the first time (since reset, at least)
        #  * m_copy was set, but cleared (was NULL)
        #  * module's global state was initialized but cleared

        # Start with an interpreter that gets destroyed right away.
        base = self.import_in_subinterp(postscript='''
            # Attrs set after loading are not in m_copy.
            mod.spam = 'spam, spam, mash, spam, eggs, and spam'
        ''')
        self.check_common(base)
        self.check_fresh(base)

        # At this point:
        #  * alive in 0 interpreters
        #  * module def in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy is NULL (claered when the interpreter was destroyed)
        #  * module's global state was initialized, not reset

        # Use a subinterpreter that sticks around.
        loaded_interp1 = self.import_in_subinterp(interpid1)
        self.check_common(loaded_interp1)
        self.check_semi_fresh(loaded_interp1, loaded_main, base)

        # At this point:
        #  * alive in 1 interpreter (interp1)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp1 (was NULL)
        #  * module's global state was updated, not reset

        # Use a subinterpreter while the previous one is still alive.
        loaded_interp2 = self.import_in_subinterp(interpid2)
        self.check_common(loaded_interp2)
        self.check_copied(loaded_interp2, loaded_interp1)

        # At this point:
        #  * alive in 2 interpreters (interp1, interp2)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp2 (was from interp1)
        #  * module's global state was updated, not reset

    @requires_subinterpreters
    def test_basic_multiple_interpreters_reset_each(self):
        # resetting between each interpreter

        # At this point:
        #  * alive in 0 interpreters
        #  * module def may or may not be loaded already
        #  * module def not in _PyRuntime.imports.extensions
        #  * mod init func has not run yet (since reset, at least)
        #  * m_copy not set (hasn't been loaded yet or already cleared)
        #  * module's global state has not been initialized yet
        #    (or already cleared)

        interpid1 = self.add_subinterpreter()
        interpid2 = self.add_subinterpreter()

        # Use an interpreter that gets destroyed right away.
        loaded = self.import_in_subinterp(
            postscript='''
            # Attrs set after loading are not in m_copy.
            mod.spam = 'spam, spam, mash, spam, eggs, and spam'
            ''',
            postcleanup=True,
        )
        self.check_common(loaded)
        self.check_fresh(loaded)

        # At this point:
        #  * alive in 0 interpreters
        #  * module def in _PyRuntime.imports.extensions
        #  * mod init func ran for the first time (since reset, at least)
        #  * m_copy is NULL (claered when the interpreter was destroyed)
        #  * module's global state was initialized, not reset

        # Use a subinterpreter that sticks around.
        loaded = self.import_in_subinterp(interpid1, postcleanup=True)
        self.check_common(loaded)
        self.check_fresh(loaded)

        # At this point:
        #  * alive in 1 interpreter (interp1)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp1 (was NULL)
        #  * module's global state was initialized, not reset

        # Use a subinterpreter while the previous one is still alive.
        loaded = self.import_in_subinterp(interpid2, postcleanup=True)
        self.check_common(loaded)
        self.check_fresh(loaded)

        # At this point:
        #  * alive in 2 interpreters (interp2, interp2)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func ran again
        #  * m_copy was copied from interp2 (was from interp1)
        #  * module's global state was initialized, not reset


@cpython_only
class CAPITests(unittest.TestCase):
    def test_pyimport_addmodule(self):
        # gh-105922: Test PyImport_AddModuleRef(), PyImport_AddModule()
        # and PyImport_AddModuleObject()
        import _testcapi
        for name in (
            'sys',     # frozen module
            'test',    # package
            __name__,  # package.module
        ):
            _testcapi.check_pyimport_addmodule(name)

    def test_pyimport_addmodule_create(self):
        # gh-105922: Test PyImport_AddModuleRef(), create a new module
        import _testcapi
        name = 'dontexist'
        self.assertNotIn(name, sys.modules)
        self.addCleanup(unload, name)

        mod = _testcapi.check_pyimport_addmodule(name)
        self.assertIs(mod, sys.modules[name])


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
