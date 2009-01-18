from importlib import Import

from contextlib import contextmanager
from functools import update_wrapper
import imp
import os.path
from test.support import unlink
import sys
from tempfile import gettempdir


using___import__ = False

def import_(*args, **kwargs):
    """Delegate to allow for injecting different implementations of import."""
    if using___import__:
        return __import__(*args, **kwargs)
    return Import()(*args, **kwargs)

def importlib_only(fxn):
    """Decorator to mark which tests are not supported by the current
    implementation of __import__()."""
    def inner(*args, **kwargs):
        if using___import__:
            return
        else:
            return fxn(*args, **kwargs)
    update_wrapper(inner, fxn)
    return inner

def writes_bytecode(fxn):
    """Decorator that returns the function if writing bytecode is enabled, else
    a stub function that accepts anything and simply returns None."""
    if sys.dont_write_bytecode:
        return lambda *args, **kwargs: None
    else:
        return fxn


def case_insensitive_tests(class_):
    """Class decorator that nullifies tests that require a case-insensitive
    file system."""
    if sys.platform not in ('win32', 'darwin', 'cygwin'):
        return object()
    else:
        return class_


@contextmanager
def uncache(*names):
    """Uncache a module from sys.modules.

    A basic sanity check is performed to prevent uncaching modules that either
    cannot/shouldn't be uncached.

    """
    for name in names:
        if name in ('sys', 'marshal', 'imp'):
            raise ValueError(
                "cannot uncache {0} as it will break _importlib".format(name))
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

@contextmanager
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
            raise ValueError(
                    'unrecognized arguments: {0}'.format(kwargs.keys()))
        yield
    finally:
        for attr, value in originals.items():
            setattr(sys, attr, value)


@contextmanager
def create_modules(*names):
    """Temporarily create each named module with an attribute (named 'attr')
    that contains the name passed into the context manager that caused the
    creation of the module.

    All files are created in a temporary directory specified by
    tempfile.gettempdir(). This directory is inserted at the beginning of
    sys.path. When the context manager exits all created files (source and
    bytecode) are explicitly deleted.

    No magic is performed when creating packages! This means that if you create
    a module within a package you must also create the package's __init__ as
    well.

    """
    source = 'attr = {0!r}'
    created_paths = []
    mapping = {}
    try:
        temp_dir = gettempdir()
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
            with open(file_path, 'w') as file:
                file.write(source.format(name))
            created_paths.append(file_path)
            mapping[name] = file_path
        uncache_manager = uncache(*import_names)
        uncache_manager.__enter__()
        state_manager = import_state(path=[temp_dir])
        state_manager.__enter__()
        yield mapping
    finally:
        state_manager.__exit__(None, None, None)
        uncache_manager.__exit__(None, None, None)
        # Reverse the order for path removal to unroll directory creation.
        for path in reversed(created_paths):
            if file_path.endswith('.py'):
                unlink(path)
                unlink(path + 'c')
                unlink(path + 'o')
            else:
                os.rmdir(path)


class mock_modules:

    """A mock importer/loader."""

    def __init__(self, *names):
        self.modules = {}
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
            module = imp.new_module(import_name)
            module.__loader__ = self
            module.__file__ = '<mock __file__>'
            module.__package__ = package
            module.attr = name
            if import_name != name:
                module.__path__ = ['<mock __path__>']
            self.modules[import_name] = module

    def __getitem__(self, name):
        return self.modules[name]

    def find_module(self, fullname, path=None):
        if fullname not in self.modules:
            return None
        else:
            return self

    def load_module(self, fullname):
        if fullname not in self.modules:
            raise ImportError
        else:
            sys.modules[fullname] = self.modules[fullname]
            return self.modules[fullname]

    def __enter__(self):
        self._uncache = uncache(*self.modules.keys())
        self._uncache.__enter__()
        return self

    def __exit__(self, *exc_info):
        self._uncache.__exit__(None, None, None)


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook


def bytecode_path(source_path):
    for suffix, _, type_ in imp.get_suffixes():
        if type_ == imp.PY_COMPILED:
            bc_suffix = suffix
            break
    else:
        raise ValueError("no bytecode suffix is defined")
    return os.path.splitext(source_path)[0] + bc_suffix
