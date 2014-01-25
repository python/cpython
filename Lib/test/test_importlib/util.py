from contextlib import contextmanager
from importlib import util, invalidate_caches
import os.path
from test import support
import unittest
import sys
import types


def import_importlib(module_name):
    """Import a module from importlib both w/ and w/o _frozen_importlib."""
    fresh = ('importlib',) if '.' in module_name else ()
    frozen = support.import_fresh_module(module_name)
    source = support.import_fresh_module(module_name, fresh=fresh,
                                         blocked=('_frozen_importlib',))
    return frozen, source


def test_both(test_class, **kwargs):
    frozen_tests = types.new_class('Frozen_'+test_class.__name__,
                                   (test_class, unittest.TestCase))
    source_tests = types.new_class('Source_'+test_class.__name__,
                                   (test_class, unittest.TestCase))
    frozen_tests.__module__ = source_tests.__module__ = test_class.__module__
    for attr, (frozen_value, source_value) in kwargs.items():
        setattr(frozen_tests, attr, frozen_value)
        setattr(source_tests, attr, source_value)
    return frozen_tests, source_tests


CASE_INSENSITIVE_FS = True
# Windows is the only OS that is *always* case-insensitive
# (OS X *can* be case-sensitive).
if sys.platform not in ('win32', 'cygwin'):
    changed_name = __file__.upper()
    if changed_name == __file__:
        changed_name = __file__.lower()
    if not os.path.exists(changed_name):
        CASE_INSENSITIVE_FS = False


def case_insensitive_tests(test):
    """Class decorator that nullifies tests requiring a case-insensitive
    file system."""
    return unittest.skipIf(not CASE_INSENSITIVE_FS,
                            "requires a case-insensitive filesystem")(test)


def submodule(parent, name, pkg_dir, content=''):
    path = os.path.join(pkg_dir, name + '.py')
    with open(path, 'w') as subfile:
        subfile.write(content)
    return '{}.{}'.format(parent, name), path


@contextmanager
def uncache(*names):
    """Uncache a module from sys.modules.

    A basic sanity check is performed to prevent uncaching modules that either
    cannot/shouldn't be uncached.

    """
    for name in names:
        if name in ('sys', 'marshal', 'imp'):
            raise ValueError(
                "cannot uncache {0}".format(name))
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
def temp_module(name, content='', *, pkg=False):
    conflicts = [n for n in sys.modules if n.partition('.')[0] == name]
    with support.temp_cwd(None) as cwd:
        with uncache(name, *conflicts):
            with support.DirsOnSysPath(cwd):
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
                    with open(modpath, 'w') as modfile:
                        modfile.write(content)
                yield location


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


class mock_modules(_ImporterMock):

    """Importer mock using PEP 302 APIs."""

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
            if fullname in self.module_code:
                try:
                    self.module_code[fullname]()
                except Exception:
                    del sys.modules[fullname]
                    raise
            return self.modules[fullname]

class mock_spec(_ImporterMock):

    """Importer mock using PEP 451 APIs."""

    def find_spec(self, fullname, path=None, parent=None):
        try:
            module = self.modules[fullname]
        except KeyError:
            return None
        is_package = hasattr(module, '__path__')
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
