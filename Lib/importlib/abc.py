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
        a namespace package. The sequence may be empty and the loader may be
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

        Used by the module type when the method does not raise
        NotImplementedError.
        """
        raise NotImplementedError

    def init_module_attrs(self, module):
        """Set the module's __loader__ attribute."""
        module.__loader__ = self


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

    def get_code(self, fullname):
        """Method which returns the code object for the module.

        The fullname is a str.  Returns a types.CodeType if possible, else
        returns None if a code object does not make sense
        (e.g. built-in module). Raises ImportError if the module cannot be
        found.
        """
        source = self.get_source(fullname)
        if source is None:
            return None
        return self.source_to_code(source)

    @abc.abstractmethod
    def get_source(self, fullname):
        """Abstract method which should return the source code for the
        module.  The fullname is a str.  Returns a str.

        Raises ImportError if the module cannot be found.
        """
        raise ImportError

    def source_to_code(self, data, path='<string>'):
        """Compile 'data' into a code object.

        The 'data' argument can be anything that compile() can handle. The'path'
        argument should be where the data was retrieved (when applicable)."""
        return compile(data, path, 'exec', dont_inherit=True)

    def init_module_attrs(self, module):
        """Initialize the __loader__ and __package__ attributes of the module.

        The name of the module is gleaned from module.__name__. The __package__
        attribute is set based on self.is_package().
        """
        super().init_module_attrs(module)
        _bootstrap._init_package_attrs(self, module)

    load_module = _bootstrap._LoaderBasics.load_module

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

    def get_code(self, fullname):
        """Method to return the code object for fullname.

        Should return None if not applicable (e.g. built-in module).
        Raise ImportError if the module cannot be found.
        """
        source = self.get_source(fullname)
        if source is None:
            return None
        try:
            path = self.get_filename(fullname)
        except ImportError:
            return self.source_to_code(source)
        else:
            return self.source_to_code(source, path)

    def init_module_attrs(self, module):
        """Initialize the module's attributes.

        It is assumed that the module's name has been set on module.__name__.
        It is also assumed that any path returned by self.get_filename() uses
        (one of) the operating system's path separator(s) to separate filenames
        from directories in order to set __path__ intelligently.
        InspectLoader.init_module_attrs() sets __loader__ and __package__.
        """
        super().init_module_attrs(module)
        _bootstrap._init_file_attrs(self, module)


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
