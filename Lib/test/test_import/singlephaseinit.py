import importlib.util
import json
import os
import sys
import textwrap
import types
import unittest
from importlib.machinery import AppleFrameworkLoader, ExtensionFileLoader

from test.test_import import (
    requires_singlephase_init,
    requires_subinterpreters,
)
from test.support import (
    Py_TRACE_REFS,
    is_apple_mobile,
    run_in_subinterp,
)

try:
    import _interpreters
except ModuleNotFoundError:
    _interpreters = None
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


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


class TestSinglePhaseSnapshot(ModuleSnapshot):
    """A representation of a single-phase init module for testing.

    Fields from ModuleSnapshot:

    * id - id(mod)
    * module - mod or a SimpleNamespace with __file__ & __spec__
    * ns - a shallow copy of mod.__dict__
    * ns_id - id(mod.__dict__)
    * cached - sys.modules[name] (or None if not there or not snapshotable)
    * cached_id - id(sys.modules[name]) (or None if not there)

    Extra fields:

    * summed - the result of calling "mod.sum(1, 2)"
    * lookedup - the result of calling "mod.look_up_self()"
    * lookedup_id - the object ID of self.lookedup
    * state_initialized - the result of calling "mod.state_initialized()"
    * init_count - (optional) the result of calling "mod.initialized_count()"

    Overridden methods from ModuleSnapshot:

    * from_module()
    * parse()

    Other methods from ModuleSnapshot:

    * build_script()
    * from_subinterp()

    ----

    There are 5 modules in Modules/_testsinglephase.c:

    * _testsinglephase
       * has global state
       * extra loads skip the init function, copy def.m_base.m_copy
       * counts calls to init function
    * _testsinglephase_basic_wrapper
       * _testsinglephase by another name (and separate init function symbol)
    * _testsinglephase_basic_copy
       * same as _testsinglephase but with own def (and init func)
    * _testsinglephase_with_reinit
       * has no global or module state
       * mod.state_initialized returns None
       * an extra load in the main interpreter calls the cached init func
       * an extra load in legacy subinterpreters does a full load
    * _testsinglephase_with_state
       * has module state
       * an extra load in the main interpreter calls the cached init func
       * an extra load in legacy subinterpreters does a full load

    (See Modules/_testsinglephase.c for more info.)

    For all those modules, the snapshot after the initial load (not in
    the global extensions cache) would look like the following:

    * initial load
       * id: ID of nww module object
       * ns: exactly what the module init put there
       * ns_id: ID of new module's __dict__
       * cached_id: same as self.id
       * summed: 3  (never changes)
       * lookedup_id: same as self.id
       * state_initialized: a timestamp between the time of the load
         and the time of the snapshot
       * init_count: 1  (None for _testsinglephase_with_reinit)

    For the other scenarios it varies.

    For the _testsinglephase, _testsinglephase_basic_wrapper, and
    _testsinglephase_basic_copy modules, the snapshot should look
    like the following:

    * reloaded
       * id: no change
       * ns: matches what the module init function put there,
         including the IDs of all contained objects,
         plus any extra attributes added before the reload
       * ns_id: no change
       * cached_id: no change
       * lookedup_id: no change
       * state_initialized: no change
       * init_count: no change
    * already loaded
       * (same as initial load except for ns and state_initialized)
       * ns: matches the initial load, incl. IDs of contained objects
       * state_initialized: no change from initial load

    For _testsinglephase_with_reinit:

    * reloaded: same as initial load (old module & ns is discarded)
    * already loaded: same as initial load (old module & ns is discarded)

    For _testsinglephase_with_state:

    * reloaded
       * (same as initial load (old module & ns is discarded),
         except init_count)
       * init_count: increase by 1
    * already loaded: same as reloaded
    """

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
        cls.LOADER = type(spec.loader)

        # Apple extensions must be distributed as frameworks. This requires
        # a specialist loader, and we need to differentiate between the
        # spec.origin and the original file location.
        if is_apple_mobile:
            assert cls.LOADER is AppleFrameworkLoader

            cls.ORIGIN = spec.origin
            with open(spec.origin + ".origin", "r") as f:
                cls.FILE = os.path.join(
                    os.path.dirname(sys.executable),
                    f.read().strip()
                )
        else:
            assert cls.LOADER is ExtensionFileLoader

            cls.ORIGIN = spec.origin
            cls.FILE = spec.origin

        # Start fresh.
        cls.clean_up()

    def tearDown(self):
        # Clean up the module.
        self.clean_up()

    @classmethod
    def clean_up(cls):
        name = cls.NAME
        if name in sys.modules:
            if hasattr(sys.modules[name], '_clear_globals'):
                assert sys.modules[name].__file__ == cls.FILE, \
                    f"{sys.modules[name].__file__} != {cls.FILE}"

                sys.modules[name]._clear_globals()
            del sys.modules[name]
        # Clear all internally cached data for the extension.
        _testinternalcapi.clear_extension(name, cls.ORIGIN)

    #########################
    # helpers

    def add_module_cleanup(self, name):
        def clean_up():
            # Clear all internally cached data for the extension.
            _testinternalcapi.clear_extension(name, self.ORIGIN)
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
        mod = self._load_dynamic(name, self.ORIGIN)
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
        reloaded = self._load_dynamic(name, self.ORIGIN)
        return types.SimpleNamespace(
            name=name,
            module=reloaded,
            snapshot=TestSinglePhaseSnapshot.from_module(reloaded),
        )

    # subinterpreters

    def add_subinterpreter(self):
        interpid = _interpreters.create('legacy')
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
                _testinternalcapi.clear_extension(name, {self.ORIGIN!r})
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
                _testinternalcapi.clear_extension(name, {self.ORIGIN!r})
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
        self.assertEqual(mod.__spec__.origin, self.ORIGIN)
        if not isolated:
            self.assertIsSubclass(mod.error, Exception)
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

    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
    def test_check_state_first(self):
        for variant in ['', '_with_reinit', '_with_state']:
            name = f'{self.NAME}{variant}_check_cache_first'
            with self.subTest(name):
                mod = self._load_dynamic(name, self.ORIGIN)
                self.assertEqual(mod.__name__, name)
                sys.modules.pop(name, None)
                _testinternalcapi.clear_extension(name, self.ORIGIN)

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
        #  * m_copy is NULL (cleared when the interpreter was destroyed)
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
        _testinternalcapi.clear_extension(self.NAME, self.ORIGIN)

        # At this point:
        #  * alive in 0 interpreters
        #  * module def loaded already
        #  * module def was in _PyRuntime.imports.extensions, but cleared
        #  * mod init func ran for the first time (since reset, at least)
        #  * m_copy was set, but cleared (was NULL)
        #  * module's global state was initialized but cleared

        # Start with an interpreter that gets destroyed right away.
        base = self.import_in_subinterp(
            postscript='''
                # Attrs set after loading are not in m_copy.
                mod.spam = 'spam, spam, mash, spam, eggs, and spam'
                ''')
        self.check_common(base)
        self.check_fresh(base)

        # At this point:
        #  * alive in 0 interpreters
        #  * module def in _PyRuntime.imports.extensions
        #  * mod init func ran for the first time (since reset)
        #  * m_copy is still set (owned by main interpreter)
        #  * module's global state was initialized, not reset

        # Use a subinterpreter that sticks around.
        loaded_interp1 = self.import_in_subinterp(interpid1)
        self.check_common(loaded_interp1)
        self.check_copied(loaded_interp1, base)

        # At this point:
        #  * alive in 1 interpreter (interp1)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func did not run again
        #  * m_copy was not changed
        #  * module's global state was not touched

        # Use a subinterpreter while the previous one is still alive.
        loaded_interp2 = self.import_in_subinterp(interpid2)
        self.check_common(loaded_interp2)
        self.check_copied(loaded_interp2, loaded_interp1)

        # At this point:
        #  * alive in 2 interpreters (interp1, interp2)
        #  * module def still in _PyRuntime.imports.extensions
        #  * mod init func did not run again
        #  * m_copy was not changed
        #  * module's global state was not touched

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
        #  * m_copy is NULL (cleared when the interpreter was destroyed)
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


if __name__ == '__main__':
    unittest.main()
