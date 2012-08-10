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
        Returns a Loader object.
        """
        raise NotImplementedError


class MetaPathFinder(Finder):

    """Abstract base class for import finders on sys.meta_path."""

    @abc.abstractmethod
    def find_module(self, fullname, path):
        """Abstract method which, when implemented, should find a module.
        The fullname is a str and the path is a str or None.
        Returns a Loader object.
        """
        raise NotImplementedError

    def invalidate_caches(self):
        """An optional method for clearing the finder's cache, if any.
        This method is used by importlib.invalidate_caches().
        """
        return NotImplemented

_register(MetaPathFinder, machinery.BuiltinImporter, machinery.FrozenImporter,
          machinery.PathFinder, machinery.WindowsRegistryFinder)


class PathEntryFinder(Finder):

    """Abstract base class for path entry finders used by PathFinder."""

    @abc.abstractmethod
    def find_loader(self, fullname):
        """Abstract method which, when implemented, returns a module loader.
        The fullname is a str.  Returns a 2-tuple of (Loader, portion) where
        portion is a sequence of file system locations contributing to part of
        a namespace package.  The sequence may be empty and the loader may be
        None.
        """
        raise NotImplementedError

    find_module = _bootstrap._find_module_shim

    def invalidate_caches(self):
        """An optional method for clearing the finder's cache, if any.
        This method is used by PathFinder.invalidate_caches().
        """
        return NotImplemented

_register(PathEntryFinder, machinery.FileFinder)


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders."""

    @abc.abstractmethod
    def load_module(self, fullname):
        """Abstract method which when implemented should load a module.
        The fullname is a str."""
        raise NotImplementedError

    @abc.abstractmethod
    def module_repr(self, module):
        """Abstract method which when implemented calculates and returns the
        given module's repr."""
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
        raise NotImplementedError


class InspectLoader(Loader):

    """Abstract base class for loaders which support inspection about the
    modules they can load.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def is_package(self, fullname):
        """Abstract method which when implemented should return whether the
        module is a package.  The fullname is a str.  Returns a bool."""
        raise NotImplementedError

    @abc.abstractmethod
    def get_code(self, fullname):
        """Abstract method which when implemented should return the code object
        for the module.  The fullname is a str.  Returns a types.CodeType."""
        raise NotImplementedError

    @abc.abstractmethod
    def get_source(self, fullname):
        """Abstract method which should return the source code for the
        module.  The fullname is a str.  Returns a str."""
        raise NotImplementedError

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
        set to."""
        raise NotImplementedError


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
            raise NotImplementedError
        return int(self.path_stats(path)['mtime'])

    def path_stats(self, path):
        """Return a metadata dict for the source pointed to by the path (str).
        Possible keys:
        - 'mtime' (mandatory) is the numeric timestamp of last source
          code modification;
        - 'size' (optional) is the size in bytes of the source code.
        """
        if self.path_mtime.__func__ is SourceLoader.path_mtime:
            raise NotImplementedError
        return {'mtime': self.path_mtime(path)}

    def set_data(self, path, data):
        """Write the bytes to the path (if possible).

        Accepts a str path and data as bytes.

        Any needed intermediary directories are to be created. If for some
        reason the file cannot be written because of permissions, fail
        silently.

        """
        raise NotImplementedError

_register(SourceLoader, machinery.SourceFileLoader)

class PyLoader(SourceLoader):

    """Implement the deprecated PyLoader ABC in terms of SourceLoader.

    This class has been deprecated! It is slated for removal in Python 3.4.
    If compatibility with Python 3.1 is not needed then implement the
    SourceLoader ABC instead of this class. If Python 3.1 compatibility is
    needed, then use the following idiom to have a single class that is
    compatible with Python 3.1 onwards::

        try:
            from importlib.abc import SourceLoader
        except ImportError:
            from importlib.abc import PyLoader as SourceLoader


        class CustomLoader(SourceLoader):
            def get_filename(self, fullname):
                # Implement ...

            def source_path(self, fullname):
                '''Implement source_path in terms of get_filename.'''
                try:
                    return self.get_filename(fullname)
                except ImportError:
                    return None

            def is_package(self, fullname):
                filename = os.path.basename(self.get_filename(fullname))
                return os.path.splitext(filename)[0] == '__init__'

    """

    @abc.abstractmethod
    def is_package(self, fullname):
        raise NotImplementedError

    @abc.abstractmethod
    def source_path(self, fullname):
        """Abstract method.  Accepts a str module name and returns the path to
        the source code for the module."""
        raise NotImplementedError

    def get_filename(self, fullname):
        """Implement get_filename in terms of source_path.

        As get_filename should only return a source file path there is no
        chance of the path not existing but loading still being possible, so
        ImportError should propagate instead of being turned into returning
        None.

        """
        warnings.warn("importlib.abc.PyLoader is deprecated and is "
                            "slated for removal in Python 3.4; "
                            "use SourceLoader instead. "
                            "See the importlib documentation on how to be "
                            "compatible with Python 3.1 onwards.",
                        DeprecationWarning)
        path = self.source_path(fullname)
        if path is None:
            raise ImportError(name=fullname)
        else:
            return path


class PyPycLoader(PyLoader):

    """Abstract base class to assist in loading source and bytecode by
    requiring only back-end storage methods to be implemented.

    This class has been deprecated! Removal is slated for Python 3.4. Implement
    the SourceLoader ABC instead. If Python 3.1 compatibility is needed, see
    PyLoader.

    The methods get_code, get_source, and load_module are implemented for the
    user.

    """

    def get_filename(self, fullname):
        """Return the source or bytecode file path."""
        path = self.source_path(fullname)
        if path is not None:
            return path
        path = self.bytecode_path(fullname)
        if path is not None:
            return path
        raise ImportError("no source or bytecode path available for "
                            "{0!r}".format(fullname), name=fullname)

    def get_code(self, fullname):
        """Get a code object from source or bytecode."""
        warnings.warn("importlib.abc.PyPycLoader is deprecated and slated for "
                            "removal in Python 3.4; use SourceLoader instead. "
                            "If Python 3.1 compatibility is required, see the "
                            "latest documentation for PyLoader.",
                        DeprecationWarning)
        source_timestamp = self.source_mtime(fullname)
        # Try to use bytecode if it is available.
        bytecode_path = self.bytecode_path(fullname)
        if bytecode_path:
            data = self.get_data(bytecode_path)
            try:
                magic = data[:4]
                if len(magic) < 4:
                    raise ImportError(
                        "bad magic number in {}".format(fullname),
                        name=fullname, path=bytecode_path)
                raw_timestamp = data[4:8]
                if len(raw_timestamp) < 4:
                    raise EOFError("bad timestamp in {}".format(fullname))
                pyc_timestamp = _bootstrap._r_long(raw_timestamp)
                raw_source_size = data[8:12]
                if len(raw_source_size) != 4:
                    raise EOFError("bad file size in {}".format(fullname))
                # Source size is unused as the ABC does not provide a way to
                # get the size of the source ahead of reading it.
                bytecode = data[12:]
                # Verify that the magic number is valid.
                if imp.get_magic() != magic:
                    raise ImportError(
                        "bad magic number in {}".format(fullname),
                        name=fullname, path=bytecode_path)
                # Verify that the bytecode is not stale (only matters when
                # there is source to fall back on.
                if source_timestamp:
                    if pyc_timestamp < source_timestamp:
                        raise ImportError("bytecode is stale", name=fullname,
                                          path=bytecode_path)
            except (ImportError, EOFError):
                # If source is available give it a shot.
                if source_timestamp is not None:
                    pass
                else:
                    raise
            else:
                # Bytecode seems fine, so try to use it.
                return marshal.loads(bytecode)
        elif source_timestamp is None:
            raise ImportError("no source or bytecode available to create code "
                              "object for {0!r}".format(fullname),
                              name=fullname)
        # Use the source.
        source_path = self.source_path(fullname)
        if source_path is None:
            message = "a source path must exist to load {0}".format(fullname)
            raise ImportError(message, name=fullname)
        source = self.get_data(source_path)
        code_object = compile(source, source_path, 'exec', dont_inherit=True)
        # Generate bytecode and write it out.
        if not sys.dont_write_bytecode:
            data = bytearray(imp.get_magic())
            data.extend(_bootstrap._w_long(source_timestamp))
            data.extend(_bootstrap._w_long(len(source) & 0xFFFFFFFF))
            data.extend(marshal.dumps(code_object))
            self.write_bytecode(fullname, data)
        return code_object

    @abc.abstractmethod
    def source_mtime(self, fullname):
        """Abstract method. Accepts a str filename and returns an int
        modification time for the source of the module."""
        raise NotImplementedError

    @abc.abstractmethod
    def bytecode_path(self, fullname):
        """Abstract method. Accepts a str filename and returns the str pathname
        to the bytecode for the module."""
        raise NotImplementedError

    @abc.abstractmethod
    def write_bytecode(self, fullname, bytecode):
        """Abstract method.  Accepts a str filename and bytes object
        representing the bytecode for the module.  Returns a boolean
        representing whether the bytecode was written or not."""
        raise NotImplementedError
