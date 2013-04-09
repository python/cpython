"""Abstract base classes related to import."""
from . import _bootstrap
from . import machinery
try:
    import _frozen_importlib
except ImportError as exc:
    if exc.name != '_frozen_importlib':
        raise
    _frozen_importlib = None
import abc
import imp
import marshal
import sys
import tokenize
import warnings


def _register(abstract_cls, *classes):
    for cls in classes:
        abstract_cls.register(cls)
        if _frozen_importlib is not None:
            frozen_cls = getattr(_frozen_importlib, cls.__name__)
            abstract_cls.register(frozen_cls)


class Finder(metaclass=abc.ABCMeta):

    """Legacy abstract base class for import finders.

    It may be subclassed for compatibility with legacy third party
    reimplementations of the import system.  Otherwise, finder
    implementations should derive from the more specific MetaPathFinder
    or PathEntryFinder ABCs.
    """

    @abc.abstractmethod
    def find_module(self, fullname, path=None):
        """An abstract method that should find a module.
        The fullname is a str and the optional path is a str or None.
        Returns a Loader object or None.
        """


class MetaPathFinder(Finder):

    """Abstract base class for import finders on sys.meta_path."""

    @abc.abstractmethod
    def find_module(self, fullname, path):
        """Abstract method which, when implemented, should find a module.
        The fullname is a str and the path is a list of strings or None.
        Returns a Loader object or None.
        """

    def invalidate_caches(self):
        """An optional method for clearing the finder's cache, if any.
        This method is used by importlib.invalidate_caches().
        """

_register(MetaPathFinder, machinery.BuiltinImporter, machinery.FrozenImporter,
          machinery.PathFinder, machinery.WindowsRegistryFinder)


class PathEntryFinder(Finder):

    """Abstract base class for path entry finders used by PathFinder."""

    @abc.abstractmethod
    def find_loader(self, fullname):
        """Abstract method which, when implemented, returns a module loader or
        a possible part of a namespace.
        The fullname is a str.  Returns a 2-tuple of (Loader, portion) where
        portion is a sequence of file system locations contributing to part of
        a namespace package.  The sequence may be empty and the loader may be
        None.
        """
        return None, []

    find_module = _bootstrap._find_module_shim

    def invalidate_caches(self):
        """An optional method for clearing the finder's cache, if any.
        This method is used by PathFinder.invalidate_caches().
        """

_register(PathEntryFinder, machinery.FileFinder)


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders.

    The optional method module_repr(module) may be defined to provide a
    repr for a module when appropriate (see PEP 420). The __repr__() method on
    the module type will use the method as appropriate.

    """

    @abc.abstractmethod
    def load_module(self, fullname):
        """Abstract method which when implemented should load a module.
        The fullname is a str.

        ImportError is raised on failure.
        """
        raise ImportError

    def module_repr(self, module):
        """Return a module's repr.

        Used by the module type when implemented without raising an exception.
        """
        raise NotImplementedError


class ResourceLoader(Loader):

    """Abstract base class for loaders which can return data from their
    back-end storage.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def get_data(self, path):
        """Abstract method which when implemented should return the bytes for
        the specified path.  The path must be a str."""
        raise IOError


class InspectLoader(Loader):

    """Abstract base class for loaders which support inspection about the
    modules they can load.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def is_package(self, fullname):
        """Abstract method which when implemented should return whether the
        module is a package.  The fullname is a str.  Returns a bool.

        Raises ImportError is the module cannot be found.
        """
        raise ImportError

    @abc.abstractmethod
    def get_code(self, fullname):
        """Abstract method which when implemented should return the code object
        for the module.  The fullname is a str.  Returns a types.CodeType.

        Raises ImportError if the module cannot be found.
        """
        raise ImportError

    @abc.abstractmethod
    def get_source(self, fullname):
        """Abstract method which should return the source code for the
        module.  The fullname is a str.  Returns a str.

        Raises ImportError if the module cannot be found.
        """
        raise ImportError

_register(InspectLoader, machinery.BuiltinImporter, machinery.FrozenImporter,
            machinery.ExtensionFileLoader)


class ExecutionLoader(InspectLoader):

    """Abstract base class for loaders that wish to support the execution of
    modules as scripts.

    This ABC represents one of the optional protocols specified in PEP 302.

    """

    @abc.abstractmethod
    def get_filename(self, fullname):
        """Abstract method which should return the value that __file__ is to be
        set to.

        Raises ImportError if the module cannot be found.
        """
        raise ImportError


class FileLoader(_bootstrap.FileLoader, ResourceLoader, ExecutionLoader):

    """Abstract base class partially implementing the ResourceLoader and
    ExecutionLoader ABCs."""

_register(FileLoader, machinery.SourceFileLoader,
            machinery.SourcelessFileLoader)


class SourceLoader(_bootstrap.SourceLoader, ResourceLoader, ExecutionLoader):

    """Abstract base class for loading source code (and optionally any
    corresponding bytecode).

    To support loading from source code, the abstractmethods inherited from
    ResourceLoader and ExecutionLoader need to be implemented. To also support
    loading from bytecode, the optional methods specified directly by this ABC
    is required.

    Inherited abstractmethods not implemented in this ABC:

        * ResourceLoader.get_data
        * ExecutionLoader.get_filename

    """

    def path_mtime(self, path):
        """Return the (int) modification time for the path (str)."""
        if self.path_stats.__func__ is SourceLoader.path_stats:
            raise IOError
        return int(self.path_stats(path)['mtime'])

    def path_stats(self, path):
        """Return a metadata dict for the source pointed to by the path (str).
        Possible keys:
        - 'mtime' (mandatory) is the numeric timestamp of last source
          code modification;
        - 'size' (optional) is the size in bytes of the source code.
        """
        if self.path_mtime.__func__ is SourceLoader.path_mtime:
            raise IOError
        return {'mtime': self.path_mtime(path)}

    def set_data(self, path, data):
        """Write the bytes to the path (if possible).

        Accepts a str path and data as bytes.

        Any needed intermediary directories are to be created. If for some
        reason the file cannot be written because of permissions, fail
        silently.
        """

_register(SourceLoader, machinery.SourceFileLoader)
