import contextlib
import _imp
import importlib
import importlib.machinery
import importlib.util
import os
import py_compile
import shutil
import sys
import textwrap
import unittest
import warnings

from .os_helper import unlink, temp_dir


@contextlib.contextmanager
def _ignore_deprecated_imports(ignore=True):
    """Context manager to suppress package and module deprecation
    warnings when importing them.

    If ignore is False, this context manager has no effect.
    """
    if ignore:
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", ".+ (module|package)",
                                    DeprecationWarning)
            yield
    else:
        yield


def unload(name):
    try:
        del sys.modules[name]
    except KeyError:
        pass


def forget(modname):
    """'Forget' a module was ever imported.

    This removes the module from sys.modules and deletes any PEP 3147/488 or
    legacy .pyc files.
    """
    unload(modname)
    for dirname in sys.path:
        source = os.path.join(dirname, modname + '.py')
        # It doesn't matter if they exist or not, unlink all possible
        # combinations of PEP 3147/488 and legacy pyc files.
        unlink(source + 'c')
        for opt in ('', 1, 2):
            try:
                unlink(importlib.util.cache_from_source(source, optimization=opt))
            except NotImplementedError:
                pass


def make_legacy_pyc(source, allow_compile=False):
    """Move a PEP 3147/488 pyc file to its legacy pyc location.

    :param source: The file system path to the source file.  The source file
        does not need to exist, however the PEP 3147/488 pyc file must exist or
        allow_compile must be set.
    :param allow_compile: If True, uses py_compile to create a .pyc if it does
        not exist. This should be passed as True if cache_tag may be None.
    :return: The file system path to the legacy pyc file.
    """
    assert source.endswith('.py')
    legacy_pyc = source + 'c'
    try:
        pyc_file = importlib.util.cache_from_source(source)
        shutil.move(pyc_file, legacy_pyc)
    except FileNotFoundError, NotImplementedError:
        if not allow_compile:
            raise
        py_compile.compile(source, legacy_pyc, doraise=True)
    return legacy_pyc


def import_module(name, deprecated=False, *, required_on=()):
    """Import and return the module to be tested, raising SkipTest if
    it is not available.

    If deprecated is True, any module or package deprecation messages
    will be suppressed. If a module is required on a platform but optional for
    others, set required_on to an iterable of platform prefixes which will be
    compared against sys.platform.
    """
    with _ignore_deprecated_imports(deprecated):
        try:
            return importlib.import_module(name)
        except ImportError as msg:
            if sys.platform.startswith(tuple(required_on)):
                raise
            raise unittest.SkipTest(str(msg))


def _save_and_remove_modules(names):
    orig_modules = {}
    prefixes = tuple(name + '.' for name in names)
    for modname in list(sys.modules):
        if modname in names or modname.startswith(prefixes):
            orig_modules[modname] = sys.modules.pop(modname)
    return orig_modules


@contextlib.contextmanager
def frozen_modules(enabled=True):
    """Force frozen modules to be used (or not).

    This only applies to modules that haven't been imported yet.
    Also, some essential modules will always be imported frozen.
    """
    _imp._override_frozen_modules_for_tests(1 if enabled else -1)
    try:
        yield
    finally:
        _imp._override_frozen_modules_for_tests(0)


@contextlib.contextmanager
def multi_interp_extensions_check(enabled=True):
    """Force legacy modules to be allowed in subinterpreters (or not).

    ("legacy" == single-phase init)

    This only applies to modules that haven't been imported yet.
    It overrides the PyInterpreterConfig.check_multi_interp_extensions
    setting (see support.run_in_subinterp_with_config() and
    _interpreters.create()).

    Also see importlib.utils.allowing_all_extensions().
    """
    old = _imp._override_multi_interp_extensions_check(1 if enabled else -1)
    try:
        yield
    finally:
        _imp._override_multi_interp_extensions_check(old)


def import_fresh_module(name, fresh=(), blocked=(), *,
                        deprecated=False,
                        usefrozen=False,
                        ):
    """Import and return a module, deliberately bypassing sys.modules.

    This function imports and returns a fresh copy of the named Python module
    by removing the named module from sys.modules before doing the import.
    Note that unlike reload, the original module is not affected by
    this operation.

    *fresh* is an iterable of additional module names that are also removed
    from the sys.modules cache before doing the import. If one of these
    modules can't be imported, None is returned.

    *blocked* is an iterable of module names that are replaced with None
    in the module cache during the import to ensure that attempts to import
    them raise ImportError.

    The named module and any modules named in the *fresh* and *blocked*
    parameters are saved before starting the import and then reinserted into
    sys.modules when the fresh import is complete.

    Module and package deprecation messages are suppressed during this import
    if *deprecated* is True.

    This function will raise ImportError if the named module cannot be
    imported.

    If "usefrozen" is False (the default) then the frozen importer is
    disabled (except for essential modules like importlib._bootstrap).
    """
    # NOTE: test_heapq, test_json and test_warnings include extra sanity checks
    # to make sure that this utility function is working as expected
    with _ignore_deprecated_imports(deprecated):
        # Keep track of modules saved for later restoration as well
        # as those which just need a blocking entry removed
        fresh = list(fresh)
        blocked = list(blocked)
        names = {name, *fresh, *blocked}
        orig_modules = _save_and_remove_modules(names)
        for modname in blocked:
            sys.modules[modname] = None

        try:
            with frozen_modules(usefrozen):
                # Return None when one of the "fresh" modules can not be imported.
                try:
                    for modname in fresh:
                        __import__(modname)
                except ImportError:
                    return None
                return importlib.import_module(name)
        finally:
            _save_and_remove_modules(names)
            sys.modules.update(orig_modules)


class CleanImport(object):
    """Context manager to force import to return a new module reference.

    This is useful for testing module-level behaviours, such as
    the emission of a DeprecationWarning on import.

    Use like this:

        with CleanImport("foo"):
            importlib.import_module("foo") # new reference

    If "usefrozen" is False (the default) then the frozen importer is
    disabled (except for essential modules like importlib._bootstrap).
    """

    def __init__(self, *module_names, usefrozen=False):
        self.original_modules = sys.modules.copy()
        for module_name in module_names:
            if module_name in sys.modules:
                module = sys.modules[module_name]
                # It is possible that module_name is just an alias for
                # another module (e.g. stub for modules renamed in 3.x).
                # In that case, we also need delete the real module to clear
                # the import cache.
                if module.__name__ != module_name:
                    del sys.modules[module.__name__]
                del sys.modules[module_name]
        self._frozen_modules = frozen_modules(usefrozen)

    def __enter__(self):
        self._frozen_modules.__enter__()
        return self

    def __exit__(self, *ignore_exc):
        sys.modules.update(self.original_modules)
        self._frozen_modules.__exit__(*ignore_exc)


class DirsOnSysPath(object):
    """Context manager to temporarily add directories to sys.path.

    This makes a copy of sys.path, appends any directories given
    as positional arguments, then reverts sys.path to the copied
    settings when the context ends.

    Note that *all* sys.path modifications in the body of the
    context manager, including replacement of the object,
    will be reverted at the end of the block.
    """

    def __init__(self, *paths):
        self.original_value = sys.path[:]
        self.original_object = sys.path
        sys.path.extend(paths)

    def __enter__(self):
        return self

    def __exit__(self, *ignore_exc):
        sys.path = self.original_object
        sys.path[:] = self.original_value


def modules_setup():
    return sys.modules.copy(),


def modules_cleanup(oldmodules):
    # Encoders/decoders are registered permanently within the internal
    # codec cache. If we destroy the corresponding modules their
    # globals will be set to None which will trip up the cached functions.
    encodings = [(k, v) for k, v in sys.modules.items()
                 if k.startswith('encodings.')]
    sys.modules.clear()
    sys.modules.update(encodings)
    # XXX: This kind of problem can affect more than just encodings.
    # In particular extension modules (such as _ssl) don't cope
    # with reloading properly. Really, test modules should be cleaning
    # out the test specific modules they know they added (ala test_runpy)
    # rather than relying on this function (as test_importhooks and test_pkg
    # do currently). Implicitly imported *real* modules should be left alone
    # (see issue 10556).
    sys.modules.update(oldmodules)


@contextlib.contextmanager
def isolated_modules():
    """
    Save modules on entry and cleanup on exit.
    """
    (saved,) = modules_setup()
    try:
        yield
    finally:
        modules_cleanup(saved)


def mock_register_at_fork(func):
    # bpo-30599: Mock os.register_at_fork() when importing the random module,
    # since this function doesn't allow to unregister callbacks and would leak
    # memory.
    from unittest import mock
    return mock.patch('os.register_at_fork', create=True)(func)


@contextlib.contextmanager
def ready_to_import(name=None, source=""):
    from test.support import script_helper

    # 1. Sets up a temporary directory and removes it afterwards
    # 2. Creates the module file
    # 3. Temporarily clears the module from sys.modules (if any)
    # 4. Reverts or removes the module when cleaning up
    name = name or "spam"
    with temp_dir() as tempdir:
        path = script_helper.make_script(tempdir, name, source)
        old_module = sys.modules.pop(name, None)
        try:
            sys.path.insert(0, tempdir)
            yield name, path
        finally:
            sys.path.remove(tempdir)
            if old_module is not None:
                sys.modules[name] = old_module
            else:
                sys.modules.pop(name, None)


def ensure_lazy_imports(imported_module, modules_to_block):
    """Test that when imported_module is imported, none of the modules in
    modules_to_block are imported as a side effect."""
    modules_to_block = frozenset(modules_to_block)
    script = textwrap.dedent(
        f"""
        import sys
        modules_to_block = {modules_to_block}
        if unexpected := modules_to_block & sys.modules.keys():
            startup = ", ".join(unexpected)
            raise AssertionError(f'unexpectedly imported at startup: {{startup}}')

        import {imported_module}
        if unexpected := modules_to_block & sys.modules.keys():
            after = ", ".join(unexpected)
            raise AssertionError(f'unexpectedly imported after importing {imported_module}: {{after}}')
        """
    )
    from .script_helper import assert_python_ok
    assert_python_ok("-S", "-c", script)


@contextlib.contextmanager
def module_restored(name):
    """A context manager that restores a module to the original state."""
    missing = object()
    orig = sys.modules.get(name, missing)
    if orig is None:
        mod = importlib.import_module(name)
    else:
        mod = type(sys)(name)
        mod.__dict__.update(orig.__dict__)
        sys.modules[name] = mod
    try:
        yield mod
    finally:
        if orig is missing:
            sys.modules.pop(name, None)
        else:
            sys.modules[name] = orig


def create_module(name, loader=None, *, ispkg=False):
    """Return a new, empty module."""
    spec = importlib.machinery.ModuleSpec(
        name,
        loader,
        origin='<import_helper>',
        is_package=ispkg,
    )
    return importlib.util.module_from_spec(spec)


def _ensure_module(name, ispkg, addparent, clearnone):
    try:
        mod = orig = sys.modules[name]
    except KeyError:
        mod = orig = None
        missing = True
    else:
        missing = False
        if mod is not None:
            # It was already imported.
            return mod, orig, missing
        # Otherwise, None means it was explicitly disabled.

    assert name != '__main__'
    if not missing:
        assert orig is None, (name, sys.modules[name])
        if not clearnone:
            raise ModuleNotFoundError(name)
        del sys.modules[name]
    # Try normal import, then fall back to adding the module.
    try:
        mod = importlib.import_module(name)
    except ModuleNotFoundError:
        if addparent and not clearnone:
            addparent = None
        mod = _add_module(name, ispkg, addparent)
    return mod, orig, missing


def _add_module(spec, ispkg, addparent):
    if isinstance(spec, str):
        name = spec
        mod = create_module(name, ispkg=ispkg)
        spec = mod.__spec__
    else:
        name = spec.name
        mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    if addparent is not False and spec.parent:
        _ensure_module(spec.parent, True, addparent, bool(addparent))
    return mod


def add_module(spec, *, parents=True):
    """Return the module after creating it and adding it to sys.modules.

    If parents is True then also create any missing parents.
    """
    return _add_module(spec, False, parents)


def add_package(spec, *, parents=True):
    """Return the module after creating it and adding it to sys.modules.

    If parents is True then also create any missing parents.
    """
    return _add_module(spec, True, parents)


def ensure_module_imported(name, *, clearnone=True):
    """Return the corresponding module.

    If it was already imported then return that.  Otherwise, try
    importing it (optionally clear it first if None).  If that fails
    then create a new empty module.

    It can be helpful to combine this with ready_to_import() and/or
    isolated_modules().
    """
    if sys.modules.get(name) is not None:
        mod = sys.modules[name]
    else:
        mod, _, _ = _ensure_module(name, False, True, clearnone)
    return mod
