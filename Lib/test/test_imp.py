import gc
import json
import importlib
import importlib.util
import os
import os.path
import py_compile
import sys
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import script_helper
from test.support import warnings_helper
import textwrap
import types
import unittest
import warnings
imp = warnings_helper.import_deprecated('imp')
import _imp
import _testinternalcapi
try:
    import _xxsubinterpreters as _interpreters
except ModuleNotFoundError:
    _interpreters = None


OS_PATH_NAME = os.path.__name__


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(_interpreters is None,
                           'subinterpreters required')(meth)


def requires_load_dynamic(meth):
    """Decorator to skip a test if not running under CPython or lacking
    imp.load_dynamic()."""
    meth = support.cpython_only(meth)
    return unittest.skipIf(getattr(imp, 'load_dynamic', None) is None,
                           'imp.load_dynamic() required')(meth)


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
            ret = support.run_in_subinterp(script)
            if ret != 0:
                raise AssertionError(f'{ret} != 0')
        else:
            _interpreters.run_string(interpid, script)

        # Parse the results.
        text = os.read(r, 1000)
        return cls.parse(text.decode())


class LockTests(unittest.TestCase):

    """Very basic test of import lock functions."""

    def verify_lock_state(self, expected):
        self.assertEqual(imp.lock_held(), expected,
                             "expected imp.lock_held() to be %r" % expected)
    def testLock(self):
        LOOPS = 50

        # The import lock may already be held, e.g. if the test suite is run
        # via "import test.autotest".
        lock_held_at_start = imp.lock_held()
        self.verify_lock_state(lock_held_at_start)

        for i in range(LOOPS):
            imp.acquire_lock()
            self.verify_lock_state(True)

        for i in range(LOOPS):
            imp.release_lock()

        # The original state should be restored now.
        self.verify_lock_state(lock_held_at_start)

        if not lock_held_at_start:
            try:
                imp.release_lock()
            except RuntimeError:
                pass
            else:
                self.fail("release_lock() without lock should raise "
                            "RuntimeError")

class ImportTests(unittest.TestCase):
    def setUp(self):
        mod = importlib.import_module('test.encoded_modules')
        self.test_strings = mod.test_strings
        self.test_path = mod.__path__

    # test_import_encoded_module moved to test_source_encoding.py

    def test_find_module_encoding(self):
        for mod, encoding, _ in self.test_strings:
            with imp.find_module('module_' + mod, self.test_path)[0] as fd:
                self.assertEqual(fd.encoding, encoding)

        path = [os.path.dirname(__file__)]
        with self.assertRaises(SyntaxError):
            imp.find_module('badsyntax_pep3120', path)

    def test_issue1267(self):
        for mod, encoding, _ in self.test_strings:
            fp, filename, info  = imp.find_module('module_' + mod,
                                                  self.test_path)
            with fp:
                self.assertNotEqual(fp, None)
                self.assertEqual(fp.encoding, encoding)
                self.assertEqual(fp.tell(), 0)
                self.assertEqual(fp.readline(), '# test %s encoding\n'
                                 % encoding)

        fp, filename, info = imp.find_module("tokenize")
        with fp:
            self.assertNotEqual(fp, None)
            self.assertEqual(fp.encoding, "utf-8")
            self.assertEqual(fp.tell(), 0)
            self.assertEqual(fp.readline(),
                             '"""Tokenization help for Python programs.\n')

    def test_issue3594(self):
        temp_mod_name = 'test_imp_helper'
        sys.path.insert(0, '.')
        try:
            with open(temp_mod_name + '.py', 'w', encoding="latin-1") as file:
                file.write("# coding: cp1252\nu = 'test.test_imp'\n")
            file, filename, info = imp.find_module(temp_mod_name)
            file.close()
            self.assertEqual(file.encoding, 'cp1252')
        finally:
            del sys.path[0]
            os_helper.unlink(temp_mod_name + '.py')
            os_helper.unlink(temp_mod_name + '.pyc')

    def test_issue5604(self):
        # Test cannot cover imp.load_compiled function.
        # Martin von Loewis note what shared library cannot have non-ascii
        # character because init_xxx function cannot be compiled
        # and issue never happens for dynamic modules.
        # But sources modified to follow generic way for processing paths.

        # the return encoding could be uppercase or None
        fs_encoding = sys.getfilesystemencoding()

        # covers utf-8 and Windows ANSI code pages
        # one non-space symbol from every page
        # (http://en.wikipedia.org/wiki/Code_page)
        known_locales = {
            'utf-8' : b'\xc3\xa4',
            'cp1250' : b'\x8C',
            'cp1251' : b'\xc0',
            'cp1252' : b'\xc0',
            'cp1253' : b'\xc1',
            'cp1254' : b'\xc0',
            'cp1255' : b'\xe0',
            'cp1256' : b'\xe0',
            'cp1257' : b'\xc0',
            'cp1258' : b'\xc0',
            }

        if sys.platform == 'darwin':
            self.assertEqual(fs_encoding, 'utf-8')
            # Mac OS X uses the Normal Form D decomposition
            # http://developer.apple.com/mac/library/qa/qa2001/qa1173.html
            special_char = b'a\xcc\x88'
        else:
            special_char = known_locales.get(fs_encoding)

        if not special_char:
            self.skipTest("can't run this test with %s as filesystem encoding"
                          % fs_encoding)
        decoded_char = special_char.decode(fs_encoding)
        temp_mod_name = 'test_imp_helper_' + decoded_char
        test_package_name = 'test_imp_helper_package_' + decoded_char
        init_file_name = os.path.join(test_package_name, '__init__.py')
        try:
            # if the curdir is not in sys.path the test fails when run with
            # ./python ./Lib/test/regrtest.py test_imp
            sys.path.insert(0, os.curdir)
            with open(temp_mod_name + '.py', 'w', encoding="utf-8") as file:
                file.write('a = 1\n')
            file, filename, info = imp.find_module(temp_mod_name)
            with file:
                self.assertIsNotNone(file)
                self.assertTrue(filename[:-3].endswith(temp_mod_name))
                self.assertEqual(info[0], '.py')
                self.assertEqual(info[1], 'r')
                self.assertEqual(info[2], imp.PY_SOURCE)

                mod = imp.load_module(temp_mod_name, file, filename, info)
                self.assertEqual(mod.a, 1)

            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                mod = imp.load_source(temp_mod_name, temp_mod_name + '.py')
            self.assertEqual(mod.a, 1)

            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                if not sys.dont_write_bytecode:
                    mod = imp.load_compiled(
                        temp_mod_name,
                        imp.cache_from_source(temp_mod_name + '.py'))
            self.assertEqual(mod.a, 1)

            if not os.path.exists(test_package_name):
                os.mkdir(test_package_name)
            with open(init_file_name, 'w', encoding="utf-8") as file:
                file.write('b = 2\n')
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                package = imp.load_package(test_package_name, test_package_name)
            self.assertEqual(package.b, 2)
        finally:
            del sys.path[0]
            for ext in ('.py', '.pyc'):
                os_helper.unlink(temp_mod_name + ext)
                os_helper.unlink(init_file_name + ext)
            os_helper.rmtree(test_package_name)
            os_helper.rmtree('__pycache__')

    def test_issue9319(self):
        path = os.path.dirname(__file__)
        self.assertRaises(SyntaxError,
                          imp.find_module, "badsyntax_pep3120", [path])

    def test_load_from_source(self):
        # Verify that the imp module can correctly load and find .py files
        # XXX (ncoghlan): It would be nice to use import_helper.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with import_helper.CleanImport('os', 'os.path', OS_PATH_NAME):
            import os
            orig_path = os.path
            orig_getenv = os.getenv
            with os_helper.EnvironmentVarGuard():
                x = imp.find_module("os")
                self.addCleanup(x[0].close)
                new_os = imp.load_module("os", *x)
                self.assertIs(os, new_os)
                self.assertIs(orig_path, new_os.path)
                self.assertIsNot(orig_getenv, new_os.getenv)

    @requires_load_dynamic
    def test_issue15828_load_extensions(self):
        # Issue 15828 picked up that the adapter between the old imp API
        # and importlib couldn't handle C extensions
        example = "_heapq"
        x = imp.find_module(example)
        file_ = x[0]
        if file_ is not None:
            self.addCleanup(file_.close)
        mod = imp.load_module(example, *x)
        self.assertEqual(mod.__name__, example)

    @requires_load_dynamic
    def test_issue16421_multiple_modules_in_one_dll(self):
        # Issue 16421: loading several modules from the same compiled file fails
        m = '_testimportmultiple'
        fileobj, pathname, description = imp.find_module(m)
        fileobj.close()
        mod0 = imp.load_dynamic(m, pathname)
        mod1 = imp.load_dynamic('_testimportmultiple_foo', pathname)
        mod2 = imp.load_dynamic('_testimportmultiple_bar', pathname)
        self.assertEqual(mod0.__name__, m)
        self.assertEqual(mod1.__name__, '_testimportmultiple_foo')
        self.assertEqual(mod2.__name__, '_testimportmultiple_bar')
        with self.assertRaises(ImportError):
            imp.load_dynamic('nonexistent', pathname)

    @requires_load_dynamic
    def test_load_dynamic_ImportError_path(self):
        # Issue #1559549 added `name` and `path` attributes to ImportError
        # in order to provide better detail. Issue #10854 implemented those
        # attributes on import failures of extensions on Windows.
        path = 'bogus file path'
        name = 'extension'
        with self.assertRaises(ImportError) as err:
            imp.load_dynamic(name, path)
        self.assertIn(path, err.exception.path)
        self.assertEqual(name, err.exception.name)

    @requires_load_dynamic
    def test_load_module_extension_file_is_None(self):
        # When loading an extension module and the file is None, open one
        # on the behalf of imp.load_dynamic().
        # Issue #15902
        name = '_testimportmultiple'
        found = imp.find_module(name)
        if found[0] is not None:
            found[0].close()
        if found[2][2] != imp.C_EXTENSION:
            self.skipTest("found module doesn't appear to be a C extension")
        imp.load_module(name, None, *found[1:])

    @requires_load_dynamic
    def test_issue24748_load_module_skips_sys_modules_check(self):
        name = 'test.imp_dummy'
        try:
            del sys.modules[name]
        except KeyError:
            pass
        try:
            module = importlib.import_module(name)
            spec = importlib.util.find_spec('_testmultiphase')
            module = imp.load_dynamic(name, spec.origin)
            self.assertEqual(module.__name__, name)
            self.assertEqual(module.__spec__.name, name)
            self.assertEqual(module.__spec__.origin, spec.origin)
            self.assertRaises(AttributeError, getattr, module, 'dummy_name')
            self.assertEqual(module.int_const, 1969)
            self.assertIs(sys.modules[name], module)
        finally:
            try:
                del sys.modules[name]
            except KeyError:
                pass

    @unittest.skipIf(sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")
    def test_bug7732(self):
        with os_helper.temp_cwd():
            source = os_helper.TESTFN + '.py'
            os.mkdir(source)
            self.assertRaisesRegex(ImportError, '^No module',
                imp.find_module, os_helper.TESTFN, ["."])

    def test_multiple_calls_to_get_data(self):
        # Issue #18755: make sure multiple calls to get_data() can succeed.
        loader = imp._LoadSourceCompatibility('imp', imp.__file__,
                                              open(imp.__file__, encoding="utf-8"))
        loader.get_data(imp.__file__)  # File should be closed
        loader.get_data(imp.__file__)  # Will need to create a newly opened file

    def test_load_source(self):
        # Create a temporary module since load_source(name) modifies
        # sys.modules[name] attributes like __loader___
        modname = f"tmp{__name__}"
        mod = type(sys.modules[__name__])(modname)
        with support.swap_item(sys.modules, modname, mod):
            with self.assertRaisesRegex(ValueError, 'embedded null'):
                imp.load_source(modname, __file__ + "\0")

    @support.cpython_only
    def test_issue31315(self):
        # There shouldn't be an assertion failure in imp.create_dynamic(),
        # when spec.name is not a string.
        create_dynamic = support.get_attribute(imp, 'create_dynamic')
        class BadSpec:
            name = None
            origin = 'foo'
        with self.assertRaises(TypeError):
            create_dynamic(BadSpec())

    def test_issue_35321(self):
        # Both _frozen_importlib and _frozen_importlib_external
        # should have a spec origin of "frozen" and
        # no need to clean up imports in this case.

        import _frozen_importlib_external
        self.assertEqual(_frozen_importlib_external.__spec__.origin, "frozen")

        import _frozen_importlib
        self.assertEqual(_frozen_importlib.__spec__.origin, "frozen")

    def test_source_hash(self):
        self.assertEqual(_imp.source_hash(42, b'hi'), b'\xfb\xd9G\x05\xaf$\x9b~')
        self.assertEqual(_imp.source_hash(43, b'hi'), b'\xd0/\x87C\xccC\xff\xe2')

    def test_pyc_invalidation_mode_from_cmdline(self):
        cases = [
            ([], "default"),
            (["--check-hash-based-pycs", "default"], "default"),
            (["--check-hash-based-pycs", "always"], "always"),
            (["--check-hash-based-pycs", "never"], "never"),
        ]
        for interp_args, expected in cases:
            args = interp_args + [
                "-c",
                "import _imp; print(_imp.check_hash_based_pycs)",
            ]
            res = script_helper.assert_python_ok(*args)
            self.assertEqual(res.out.strip().decode('utf-8'), expected)

    def test_find_and_load_checked_pyc(self):
        # issue 34056
        with os_helper.temp_cwd():
            with open('mymod.py', 'wb') as fp:
                fp.write(b'x = 42\n')
            py_compile.compile(
                'mymod.py',
                doraise=True,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH,
            )
            file, path, description = imp.find_module('mymod', path=['.'])
            mod = imp.load_module('mymod', file, path, description)
        self.assertEqual(mod.x, 42)

    def test_issue98354(self):
        # _imp.create_builtin should raise TypeError
        # if 'name' attribute of 'spec' argument is not a 'str' instance

        create_builtin = support.get_attribute(_imp, "create_builtin")

        class FakeSpec:
            def __init__(self, name):
                self.name = self
        spec = FakeSpec("time")
        with self.assertRaises(TypeError):
            create_builtin(spec)

        class FakeSpec2:
            name = [1, 2, 3, 4]
        spec = FakeSpec2()
        with self.assertRaises(TypeError):
            create_builtin(spec)

        import builtins
        class UnicodeSubclass(str):
            pass
        class GoodSpec:
            name = UnicodeSubclass("builtins")
        spec = GoodSpec()
        bltin = create_builtin(spec)
        self.assertEqual(bltin, builtins)

        class UnicodeSubclassFakeSpec(str):
            def __init__(self, name):
                self.name = self
        spec = UnicodeSubclassFakeSpec("builtins")
        bltin = create_builtin(spec)
        self.assertEqual(bltin, builtins)

    @support.cpython_only
    def test_create_builtin_subinterp(self):
        # gh-99578: create_builtin() behavior changes after the creation of the
        # first sub-interpreter. Test both code paths, before and after the
        # creation of a sub-interpreter. Previously, create_builtin() had
        # a reference leak after the creation of the first sub-interpreter.

        import builtins
        create_builtin = support.get_attribute(_imp, "create_builtin")
        class Spec:
            name = "builtins"
        spec = Spec()

        def check_get_builtins():
            refcnt = sys.getrefcount(builtins)
            mod = _imp.create_builtin(spec)
            self.assertIs(mod, builtins)
            self.assertEqual(sys.getrefcount(builtins), refcnt + 1)
            # Check that a GC collection doesn't crash
            gc.collect()

        check_get_builtins()

        ret = support.run_in_subinterp("import builtins")
        self.assertEqual(ret, 0)

        check_get_builtins()


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

    SCRIPT_BODY = ModuleSnapshot.SCRIPT_BODY + textwrap.dedent(f'''
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


@requires_load_dynamic
class SinglephaseInitTests(unittest.TestCase):

    NAME = '_testsinglephase'

    @classmethod
    def setUpClass(cls):
        if '-R' in sys.argv or '--huntrleaks' in sys.argv:
            # https://github.com/python/cpython/issues/102251
            raise unittest.SkipTest('unresolved refleaks (see gh-102251)')
        fileobj, filename, _ = imp.find_module(cls.NAME)
        fileobj.close()
        cls.FILE = filename

        # Start fresh.
        cls.clean_up()

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

    def load(self, name):
        try:
            already_loaded = self.already_loaded
        except AttributeError:
            already_loaded = self.already_loaded = {}
        assert name not in already_loaded
        mod = imp.load_dynamic(name, self.FILE)
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
        reloaded = imp.load_dynamic(name, self.FILE)
        return types.SimpleNamespace(
            name=name,
            module=reloaded,
            snapshot=TestSinglePhaseSnapshot.from_module(reloaded),
        )

    # subinterpreters

    def add_subinterpreter(self):
        interpid = _interpreters.create(isolated=False)
        _interpreters.run_string(interpid, textwrap.dedent('''
            import sys
            import _testinternalcapi
            '''))
        def clean_up():
            _interpreters.run_string(interpid, textwrap.dedent(f'''
                name = {self.NAME!r}
                if name in sys.modules:
                    sys.modules[name]._clear_globals()
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

        for name in [
            f'{self.NAME}_with_reinit',  # m_size == 0
            f'{self.NAME}_with_state',  # m_size > 0
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

    @requires_subinterpreters
    def test_basic_multiple_interpreters_deleted_no_reset(self):
        # without resetting; already loaded in a deleted interpreter

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
    @requires_load_dynamic
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


class ReloadTests(unittest.TestCase):

    """Very basic tests to make sure that imp.reload() operates just like
    reload()."""

    def test_source(self):
        # XXX (ncoghlan): It would be nice to use test.import_helper.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with os_helper.EnvironmentVarGuard():
            import os
            imp.reload(os)

    def test_extension(self):
        with import_helper.CleanImport('time'):
            import time
            imp.reload(time)

    def test_builtin(self):
        with import_helper.CleanImport('marshal'):
            import marshal
            imp.reload(marshal)

    def test_with_deleted_parent(self):
        # see #18681
        from html import parser
        html = sys.modules.pop('html')
        def cleanup():
            sys.modules['html'] = html
        self.addCleanup(cleanup)
        with self.assertRaisesRegex(ImportError, 'html'):
            imp.reload(parser)


class PEP3147Tests(unittest.TestCase):
    """Tests of PEP 3147."""

    tag = imp.get_tag()

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(imp.cache_from_source(path, True), expect)

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag to not be '
                         'None')
    def test_source_from_cache(self):
        # Given the path to a PEP 3147 defined .pyc file, return the path to
        # its source.  This tests the good path.
        path = os.path.join('foo', 'bar', 'baz', '__pycache__',
                            'qux.{}.pyc'.format(self.tag))
        expect = os.path.join('foo', 'bar', 'baz', 'qux.py')
        self.assertEqual(imp.source_from_cache(path), expect)


class NullImporterTests(unittest.TestCase):
    @unittest.skipIf(os_helper.TESTFN_UNENCODABLE is None,
                     "Need an undecodeable filename")
    def test_unencodeable(self):
        name = os_helper.TESTFN_UNENCODABLE
        os.mkdir(name)
        try:
            self.assertRaises(ImportError, imp.NullImporter, name)
        finally:
            os.rmdir(name)


if __name__ == "__main__":
    unittest.main()
