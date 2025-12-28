import builtins
import contextlib
import errno
import functools
from importlib import machinery, util, invalidate_caches
import marshal
import os
import os.path
from test import support
from test.support import import_helper
from test.support import is_apple_mobile
from test.support import os_helper
import unittest
import sys
import tempfile
import types

# gh-116303: Skip test module dependent tests if test modules are unavailable
import_helper.import_module("_testmultiphase")


BUILTINS = types.SimpleNamespace()
BUILTINS.good_name = None
BUILTINS.bad_name = None
if 'errno' in sys.builtin_module_names:
    BUILTINS.good_name = 'errno'
if 'importlib' not in sys.builtin_module_names:
    BUILTINS.bad_name = 'importlib'

if support.is_wasi:
    # dlopen() is a shim for WASI as of WASI SDK which fails by default.
    # We don't provide an implementation, so tests will fail.
    # But we also don't want to turn off dynamic loading for those that provide
    # a working implementation.
    def _extension_details():
        global EXTENSIONS
        EXTENSIONS = None
else:
    EXTENSIONS = types.SimpleNamespace()
    EXTENSIONS.path = None
    EXTENSIONS.ext = None
    EXTENSIONS.filename = None
    EXTENSIONS.file_path = None
    EXTENSIONS.name = '_testsinglephase'

    def _extension_details():
        global EXTENSIONS
        for path in sys.path:
            for ext in machinery.EXTENSION_SUFFIXES:
                # Apple mobile platforms mechanically load .so files,
                # but the findable files are labelled .fwork
                if is_apple_mobile:
                    ext = ext.replace(".so", ".fwork")

                filename = EXTENSIONS.name + ext
                file_path = os.path.join(path, filename)
                if os.path.exists(file_path):
                    EXTENSIONS.path = path
                    EXTENSIONS.ext = ext
                    EXTENSIONS.filename = filename
                    EXTENSIONS.file_path = file_path
                    return

_extension_details()


def import_importlib(module_name):
    """Import a module from importlib both w/ and w/o _frozen_importlib."""
    fresh = ('importlib',) if '.' in module_name else ()
    frozen = import_helper.import_fresh_module(module_name)
    source = import_helper.import_fresh_module(module_name, fresh=fresh,
                                         blocked=('_frozen_importlib', '_frozen_importlib_external'))
    return {'Frozen': frozen, 'Source': source}


def specialize_class(cls, kind, base=None, **kwargs):
    # XXX Support passing in submodule names--load (and cache) them?
    # That would clean up the test modules a bit more.
    if base is None:
        base = unittest.TestCase
    elif not isinstance(base, type):
        base = base[kind]
    name = '{}_{}'.format(kind, cls.__name__)
    bases = (cls, base)
    specialized = types.new_class(name, bases)
    specialized.__module__ = cls.__module__
    specialized._NAME = cls.__name__
    specialized._KIND = kind
    for attr, values in kwargs.items():
        value = values[kind]
        setattr(specialized, attr, value)
    return specialized


def split_frozen(cls, base=None, **kwargs):
    frozen = specialize_class(cls, 'Frozen', base, **kwargs)
    source = specialize_class(cls, 'Source', base, **kwargs)
    return frozen, source


def test_both(test_class, base=None, **kwargs):
    return split_frozen(test_class, base, **kwargs)


CASE_INSENSITIVE_FS = True
# Windows is the only OS that is *always* case-insensitive
# (OS X *can* be case-sensitive).
if sys.platform not in ('win32', 'cygwin'):
    changed_name = __file__.upper()
    if changed_name == __file__:
        changed_name = __file__.lower()
    if not os.path.exists(changed_name):
        CASE_INSENSITIVE_FS = False

source_importlib = import_importlib('importlib')['Source']
__import__ = {'Frozen': staticmethod(builtins.__import__),
              'Source': staticmethod(source_importlib.__import__)}


def case_insensitive_tests(test):
    """Class decorator that nullifies tests requiring a case-insensitive
    file system."""
    return unittest.skipIf(not CASE_INSENSITIVE_FS,
                            "requires a case-insensitive filesystem")(test)


def submodule(parent, name, pkg_dir, content=''):
    path = os.path.join(pkg_dir, name + '.py')
    with open(path, 'w', encoding='utf-8') as subfile:
        subfile.write(content)
    return '{}.{}'.format(parent, name), path


def get_code_from_pyc(pyc_path):
    """Reads a pyc file and returns the unmarshalled code object within.

    No header validation is performed.
    """
    with open(pyc_path, 'rb') as pyc_f:
        pyc_f.seek(16)
        return marshal.load(pyc_f)


@contextlib.contextmanager
def uncache(*names):
    """Uncache a module from sys.modules.

    A basic sanity check is performed to prevent uncaching modules that either
    cannot/shouldn't be uncached.

    """
    for name in names:
        if name in ('sys', 'marshal'):
            raise ValueError("cannot uncache {}".format(name))
        try:
            del sys.modules[name]
        except KeyError:
            pass
    try:
        yield
    finally:
        for name in names:
            try:
                del sys.modules[name]
            except KeyError:
                pass


@contextlib.contextmanager
def temp_module(name, content='', *, pkg=False):
    conflicts = [n for n in sys.modules if n.partition('.')[0] == name]
    with os_helper.temp_cwd(None) as cwd:
        with uncache(name, *conflicts):
            with import_helper.DirsOnSysPath(cwd):
                invalidate_caches()

                location = os.path.join(cwd, name)
                if pkg:
                    modpath = os.path.join(location, '__init__.py')
                    os.mkdir(name)
                else:
                    modpath = location + '.py'
                    if content is None:
                        # Make sure the module file gets created.
                        content = ''
                if content is not None:
                    # not a namespace package
                    with open(modpath, 'w', encoding='utf-8') as modfile:
                        modfile.write(content)
                yield location


@contextlib.contextmanager
def import_state(**kwargs):
    """Context manager to manage the various importers and stored state in the
    sys module.

    The 'modules' attribute is not supported as the interpreter state stores a
    pointer to the dict that the interpreter uses internally;
    reassigning to sys.modules does not have the desired effect.

    """
    originals = {}
    try:
        for attr, default in (('meta_path', []), ('path', []),
                              ('path_hooks', []),
                              ('path_importer_cache', {})):
            originals[attr] = getattr(sys, attr)
            if attr in kwargs:
                new_value = kwargs[attr]
                del kwargs[attr]
            else:
                new_value = default
            setattr(sys, attr, new_value)
        if len(kwargs):
            raise ValueError('unrecognized arguments: {}'.format(kwargs))
        yield
    finally:
        for attr, value in originals.items():
            setattr(sys, attr, value)


class _ImporterMock:

    """Base class to help with creating importer mocks."""

    def __init__(self, *names, module_code={}):
        self.modules = {}
        self.module_code = {}
        for name in names:
            if not name.endswith('.__init__'):
                import_name = name
            else:
                import_name = name[:-len('.__init__')]
            if '.' not in name:
                package = None
            elif import_name == name:
                package = name.rsplit('.', 1)[0]
            else:
                package = import_name
            module = types.ModuleType(import_name)
            module.__loader__ = self
            module.__file__ = '<mock __file__>'
            module.__package__ = package
            module.attr = name
            if import_name != name:
                module.__path__ = ['<mock __path__>']
            self.modules[import_name] = module
            if import_name in module_code:
                self.module_code[import_name] = module_code[import_name]

    def __getitem__(self, name):
        return self.modules[name]

    def __enter__(self):
        self._uncache = uncache(*self.modules.keys())
        self._uncache.__enter__()
        return self

    def __exit__(self, *exc_info):
        self._uncache.__exit__(None, None, None)


class mock_spec(_ImporterMock):

    """Importer mock using PEP 451 APIs."""

    def find_spec(self, fullname, path=None, parent=None):
        try:
            module = self.modules[fullname]
        except KeyError:
            return None
        spec = util.spec_from_file_location(
                fullname, module.__file__, loader=self,
                submodule_search_locations=getattr(module, '__path__', None))
        return spec

    def create_module(self, spec):
        if spec.name not in self.modules:
            raise ImportError
        return self.modules[spec.name]

    def exec_module(self, module):
        try:
            self.module_code[module.__spec__.name]()
        except KeyError:
            pass


def writes_bytecode_files(fxn):
    """Decorator to protect sys.dont_write_bytecode from mutation and to skip
    tests that require it to be set to False."""
    if sys.dont_write_bytecode:
        return unittest.skip("relies on writing bytecode")(fxn)
    @functools.wraps(fxn)
    def wrapper(*args, **kwargs):
        original = sys.dont_write_bytecode
        sys.dont_write_bytecode = False
        try:
            to_return = fxn(*args, **kwargs)
        finally:
            sys.dont_write_bytecode = original
        return to_return
    return wrapper


def ensure_bytecode_path(bytecode_path):
    """Ensure that the __pycache__ directory for PEP 3147 pyc file exists.

    :param bytecode_path: File system path to PEP 3147 pyc file.
    """
    try:
        os.mkdir(os.path.dirname(bytecode_path))
    except OSError as error:
        if error.errno != errno.EEXIST:
            raise


@contextlib.contextmanager
def temporary_pycache_prefix(prefix):
    """Adjust and restore sys.pycache_prefix."""
    _orig_prefix = sys.pycache_prefix
    sys.pycache_prefix = prefix
    try:
        yield
    finally:
        sys.pycache_prefix = _orig_prefix


@contextlib.contextmanager
def create_modules(*names):
    """Temporarily create each named module with an attribute (named 'attr')
    that contains the name passed into the context manager that caused the
    creation of the module.

    All files are created in a temporary directory returned by
    tempfile.mkdtemp(). This directory is inserted at the beginning of
    sys.path. When the context manager exits all created files (source and
    bytecode) are explicitly deleted.

    No magic is performed when creating packages! This means that if you create
    a module within a package you must also create the package's __init__ as
    well.

    """
    source = 'attr = {0!r}'
    created_paths = []
    mapping = {}
    state_manager = None
    uncache_manager = None
    try:
        temp_dir = tempfile.mkdtemp()
        mapping['.root'] = temp_dir
        import_names = set()
        for name in names:
            if not name.endswith('__init__'):
                import_name = name
            else:
                import_name = name[:-len('.__init__')]
            import_names.add(import_name)
            if import_name in sys.modules:
                del sys.modules[import_name]
            name_parts = name.split('.')
            file_path = temp_dir
            for directory in name_parts[:-1]:
                file_path = os.path.join(file_path, directory)
                if not os.path.exists(file_path):
                    os.mkdir(file_path)
                    created_paths.append(file_path)
            file_path = os.path.join(file_path, name_parts[-1] + '.py')
            with open(file_path, 'w', encoding='utf-8') as file:
                file.write(source.format(name))
            created_paths.append(file_path)
            mapping[name] = file_path
        uncache_manager = uncache(*import_names)
        uncache_manager.__enter__()
        state_manager = import_state(path=[temp_dir])
        state_manager.__enter__()
        yield mapping
    finally:
        if state_manager is not None:
            state_manager.__exit__(None, None, None)
        if uncache_manager is not None:
            uncache_manager.__exit__(None, None, None)
        os_helper.rmtree(temp_dir)


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook


class CASEOKTestBase:

    def caseok_env_changed(self, *, should_exist):
        possibilities = b'PYTHONCASEOK', 'PYTHONCASEOK'
        if any(x in self.importlib._bootstrap_external._os.environ
                    for x in possibilities) != should_exist:
            self.skipTest('os.environ changes not reflected in _os.environ')
