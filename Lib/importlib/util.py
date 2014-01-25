"""Utility code for constructing importers, etc."""

from ._bootstrap import MAGIC_NUMBER
from ._bootstrap import cache_from_source
from ._bootstrap import decode_source
from ._bootstrap import source_from_cache
from ._bootstrap import spec_from_loader
from ._bootstrap import spec_from_file_location
from ._bootstrap import _resolve_name
from ._bootstrap import _find_spec

from contextlib import contextmanager
import functools
import sys
import warnings


def resolve_name(name, package):
    """Resolve a relative module name to an absolute one."""
    if not name.startswith('.'):
        return name
    elif not package:
        raise ValueError('{!r} is not a relative name '
                         '(no leading dot)'.format(name))
    level = 0
    for character in name:
        if character != '.':
            break
        level += 1
    return _resolve_name(name[level:], package, level)


def _find_spec_from_path(name, path=None):
    """Return the spec for the specified module.

    First, sys.modules is checked to see if the module was already imported. If
    so, then sys.modules[name].__spec__ is returned. If that happens to be
    set to None, then ValueError is raised. If the module is not in
    sys.modules, then sys.meta_path is searched for a suitable spec with the
    value of 'path' given to the finders. None is returned if no spec could
    be found.

    Dotted names do not have their parent packages implicitly imported. You will
    most likely need to explicitly import all parent packages in the proper
    order for a submodule to get the correct spec.

    """
    if name not in sys.modules:
        return _find_spec(name, path)
    else:
        module = sys.modules[name]
        if module is None:
            return None
        try:
            spec = module.__spec__
        except AttributeError:
            raise ValueError('{}.__spec__ is not set'.format(name))
        else:
            if spec is None:
                raise ValueError('{}.__spec__ is None'.format(name))
            return spec


def find_spec(name, package=None):
    """Return the spec for the specified module.

    First, sys.modules is checked to see if the module was already imported. If
    so, then sys.modules[name].__spec__ is returned. If that happens to be
    set to None, then ValueError is raised. If the module is not in
    sys.modules, then sys.meta_path is searched for a suitable spec with the
    value of 'path' given to the finders. None is returned if no spec could
    be found.

    If the name is for submodule (contains a dot), the parent module is
    automatically imported.

    The name and package arguments work the same as importlib.import_module().
    In other words, relative module names (with leading dots) work.

    """
    fullname = resolve_name(name, package) if name.startswith('.') else name
    if fullname not in sys.modules:
        parent_name = fullname.rpartition('.')[0]
        if parent_name:
            # Use builtins.__import__() in case someone replaced it.
            parent = __import__(parent_name, fromlist=['__path__'])
            return _find_spec(fullname, parent.__path__)
        else:
            return _find_spec(fullname, None)
    else:
        module = sys.modules[fullname]
        if module is None:
            return None
        try:
            spec = module.__spec__
        except AttributeError:
            raise ValueError('{}.__spec__ is not set'.format(name))
        else:
            if spec is None:
                raise ValueError('{}.__spec__ is None'.format(name))
            return spec


@contextmanager
def _module_to_load(name):
    is_reload = name in sys.modules

    module = sys.modules.get(name)
    if not is_reload:
        # This must be done before open() is called as the 'io' module
        # implicitly imports 'locale' and would otherwise trigger an
        # infinite loop.
        module = type(sys)(name)
        # This must be done before putting the module in sys.modules
        # (otherwise an optimization shortcut in import.c becomes wrong)
        module.__initializing__ = True
        sys.modules[name] = module
    try:
        yield module
    except Exception:
        if not is_reload:
            try:
                del sys.modules[name]
            except KeyError:
                pass
    finally:
        module.__initializing__ = False


def set_package(fxn):
    """Set __package__ on the returned module.

    This function is deprecated.

    """
    @functools.wraps(fxn)
    def set_package_wrapper(*args, **kwargs):
        warnings.warn('The import system now takes care of this automatically.',
                      DeprecationWarning, stacklevel=2)
        module = fxn(*args, **kwargs)
        if getattr(module, '__package__', None) is None:
            module.__package__ = module.__name__
            if not hasattr(module, '__path__'):
                module.__package__ = module.__package__.rpartition('.')[0]
        return module
    return set_package_wrapper


def set_loader(fxn):
    """Set __loader__ on the returned module.

    This function is deprecated.

    """
    @functools.wraps(fxn)
    def set_loader_wrapper(self, *args, **kwargs):
        warnings.warn('The import system now takes care of this automatically.',
                      DeprecationWarning, stacklevel=2)
        module = fxn(self, *args, **kwargs)
        if getattr(module, '__loader__', None) is None:
            module.__loader__ = self
        return module
    return set_loader_wrapper


def module_for_loader(fxn):
    """Decorator to handle selecting the proper module for loaders.

    The decorated function is passed the module to use instead of the module
    name. The module passed in to the function is either from sys.modules if
    it already exists or is a new module. If the module is new, then __name__
    is set the first argument to the method, __loader__ is set to self, and
    __package__ is set accordingly (if self.is_package() is defined) will be set
    before it is passed to the decorated function (if self.is_package() does
    not work for the module it will be set post-load).

    If an exception is raised and the decorator created the module it is
    subsequently removed from sys.modules.

    The decorator assumes that the decorated function takes the module name as
    the second argument.

    """
    warnings.warn('The import system now takes care of this automatically.',
                  DeprecationWarning, stacklevel=2)
    @functools.wraps(fxn)
    def module_for_loader_wrapper(self, fullname, *args, **kwargs):
        with _module_to_load(fullname) as module:
            module.__loader__ = self
            try:
                is_package = self.is_package(fullname)
            except (ImportError, AttributeError):
                pass
            else:
                if is_package:
                    module.__package__ = fullname
                else:
                    module.__package__ = fullname.rpartition('.')[0]
            # If __package__ was not set above, __import__() will do it later.
            return fxn(self, module, *args, **kwargs)

    return module_for_loader_wrapper
