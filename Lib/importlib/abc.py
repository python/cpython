"""Abstract base classes related to import."""
from . import _bootstrap
from . import machinery
from . import util
import abc
import imp
import io
import marshal
import os.path
import sys
import tokenize
import types
import warnings


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders."""

    @abc.abstractmethod
    def load_module(self, fullname:str) -> types.ModuleType:
        """Abstract method which when implemented should load a module."""
        raise NotImplementedError


class Finder(metaclass=abc.ABCMeta):

    """Abstract base class for import finders."""

    @abc.abstractmethod
    def find_module(self, fullname:str, path:[str]=None) -> Loader:
        """Abstract method which when implemented should find a module."""
        raise NotImplementedError

Finder.register(machinery.BuiltinImporter)
Finder.register(machinery.FrozenImporter)
Finder.register(machinery.PathFinder)


class ResourceLoader(Loader):

    """Abstract base class for loaders which can return data from their
    back-end storage.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def get_data(self, path:str) -> bytes:
        """Abstract method which when implemented should return the bytes for
        the specified path."""
        raise NotImplementedError


class InspectLoader(Loader):

    """Abstract base class for loaders which support inspection about the
    modules they can load.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def is_package(self, fullname:str) -> bool:
        """Abstract method which when implemented should return whether the
        module is a package."""
        raise NotImplementedError

    @abc.abstractmethod
    def get_code(self, fullname:str) -> types.CodeType:
        """Abstract method which when implemented should return the code object
        for the module"""
        raise NotImplementedError

    @abc.abstractmethod
    def get_source(self, fullname:str) -> str:
        """Abstract method which should return the source code for the
        module."""
        raise NotImplementedError

InspectLoader.register(machinery.BuiltinImporter)
InspectLoader.register(machinery.FrozenImporter)


class ExecutionLoader(InspectLoader):

    """Abstract base class for loaders that wish to support the execution of
    modules as scripts.

    This ABC represents one of the optional protocols specified in PEP 302.

    """

    @abc.abstractmethod
    def get_filename(self, fullname:str) -> str:
        """Abstract method which should return the value that __file__ is to be
        set to."""
        raise NotImplementedError


class SourceLoader(ResourceLoader, ExecutionLoader):

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

    def path_mtime(self, path:str) -> int:
        """Optional method that returns the modification time for the specified
        path.

        Implementing this method allows the loader to read bytecode files.

        """
        raise NotImplementedError

    def set_data(self, path:str, data:bytes) -> None:
        """Optional method which writes data to a file path.

        Implementing this method allows for the writing of bytecode files.

        """
        raise NotImplementedError

    def is_package(self, fullname):
        """Concrete implementation of InspectLoader.is_package by checking if
        the path returned by get_filename has a filename of '__init__.py'."""
        filename = os.path.basename(self.get_filename(fullname))
        return os.path.splitext(filename)[0] == '__init__'

    def get_source(self, fullname):
        """Concrete implementation of InspectLoader.get_source."""
        path = self.get_filename(fullname)
        try:
            source_bytes = self.get_data(path)
        except IOError:
            raise ImportError("source not available through get_data()")
        encoding = tokenize.detect_encoding(io.BytesIO(source_bytes).readline)
        return source_bytes.decode(encoding[0])

    def get_code(self, fullname):
        """Concrete implementation of InspectLoader.get_code.

        Reading of bytecode requires path_mtime to be implemented. To write
        bytecode, set_data must also be implemented.

        """
        source_path = self.get_filename(fullname)
        bytecode_path = imp.cache_from_source(source_path)
        source_mtime = None
        if bytecode_path is not None:
            try:
                source_mtime = self.path_mtime(source_path)
            except NotImplementedError:
                pass
            else:
                try:
                    data = self.get_data(bytecode_path)
                except IOError:
                    pass
                else:
                    magic = data[:4]
                    raw_timestamp = data[4:8]
                    if (len(magic) == 4 and len(raw_timestamp) == 4 and
                            magic == imp.get_magic() and
                            marshal._r_long(raw_timestamp) == source_mtime):
                        return marshal.loads(data[8:])
        source_bytes = self.get_data(source_path)
        code_object = compile(source_bytes, source_path, 'exec',
                                dont_inherit=True)
        if (not sys.dont_write_bytecode and bytecode_path is not None and
                source_mtime is not None):
            # If e.g. Jython ever implements imp.cache_from_source to have
            # their own cached file format, this block of code will most likely
            # throw an exception.
            data = bytearray(imp.get_magic())
            data.extend(marshal._w_long(source_mtime))
            data.extend(marshal.dumps(code_object))
            try:
                self.set_data(bytecode_path, data)
            except (NotImplementedError, IOError):
                pass
        return code_object

    @util.module_for_loader
    def load_module(self, module):
        """Concrete implementation of Loader.load_module.

        Requires ExecutionLoader.get_filename and ResourceLoader.get_data to be
        implemented to load source code. Use of bytecode is dictated by whether
        get_code uses/writes bytecode.

        """
        name = module.__name__
        code_object = self.get_code(name)
        module.__file__ = self.get_filename(name)
        module.__cached__ = imp.cache_from_source(module.__file__)
        module.__package__ = name
        is_package = self.is_package(name)
        if is_package:
            module.__path__  = [os.path.dirname(module.__file__)]
        else:
            module.__package__ = module.__package__.rpartition('.')[0]
        module.__loader__ = self
        exec(code_object, module.__dict__)
        return module


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
    def source_path(self, fullname:str) -> object:
        """Abstract method which when implemented should return the path to the
        source code for the module."""
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
                        PendingDeprecationWarning)
        path = self.source_path(fullname)
        if path is None:
            raise ImportError
        else:
            return path

PyLoader.register(_bootstrap.PyLoader)


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
                            "{0!r}".format(fullname))

    def get_code(self, fullname):
        """Get a code object from source or bytecode."""
        warnings.warn("importlib.abc.PyPycLoader is deprecated and slated for "
                            "removal in Python 3.4; use SourceLoader instead. "
                            "If Python 3.1 compatibility is required, see the "
                            "latest documentation for PyLoader.",
                        PendingDeprecationWarning)
        source_timestamp = self.source_mtime(fullname)
        # Try to use bytecode if it is available.
        bytecode_path = self.bytecode_path(fullname)
        if bytecode_path:
            data = self.get_data(bytecode_path)
            try:
                magic = data[:4]
                if len(magic) < 4:
                    raise ImportError("bad magic number in {}".format(fullname))
                raw_timestamp = data[4:8]
                if len(raw_timestamp) < 4:
                    raise EOFError("bad timestamp in {}".format(fullname))
                pyc_timestamp = marshal._r_long(raw_timestamp)
                bytecode = data[8:]
                # Verify that the magic number is valid.
                if imp.get_magic() != magic:
                    raise ImportError("bad magic number in {}".format(fullname))
                # Verify that the bytecode is not stale (only matters when
                # there is source to fall back on.
                if source_timestamp:
                    if pyc_timestamp < source_timestamp:
                        raise ImportError("bytecode is stale")
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
                                "object for {0!r}".format(fullname))
        # Use the source.
        source_path = self.source_path(fullname)
        if source_path is None:
            message = "a source path must exist to load {0}".format(fullname)
            raise ImportError(message)
        source = self.get_data(source_path)
        code_object = compile(source, source_path, 'exec', dont_inherit=True)
        # Generate bytecode and write it out.
        if not sys.dont_write_bytecode:
            data = bytearray(imp.get_magic())
            data.extend(marshal._w_long(source_timestamp))
            data.extend(marshal.dumps(code_object))
            self.write_bytecode(fullname, data)
        return code_object


    @abc.abstractmethod
    def source_mtime(self, fullname:str) -> int:
        """Abstract method which when implemented should return the
        modification time for the source of the module."""
        raise NotImplementedError

    @abc.abstractmethod
    def bytecode_path(self, fullname:str) -> object:
        """Abstract method which when implemented should return the path to the
        bytecode for the module."""
        raise NotImplementedError

    @abc.abstractmethod
    def write_bytecode(self, fullname:str, bytecode:bytes) -> bool:
        """Abstract method which when implemented should attempt to write the
        bytecode for the module, returning a boolean representing whether the
        bytecode was written or not."""
        raise NotImplementedError

PyPycLoader.register(_bootstrap.PyPycLoader)
