"""Abstract base classes related to import."""
from . import _bootstrap
from . import machinery
import abc
import types


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders."""

    @abc.abstractmethod
    def load_module(self, fullname:str) -> types.ModuleType:
        """Abstract method which when implemented should load a module."""
        raise NotImplementedError

Loader.register(machinery.FrozenImporter)


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
        return NotImplementedError

    @abc.abstractmethod
    def get_code(self, fullname:str) -> types.CodeType:
        """Abstract method which when implemented should return the code object
        for the module"""
        return NotImplementedError

    @abc.abstractmethod
    def get_source(self, fullname:str) -> str:
        """Abstract method which should return the source code for the
        module."""
        return NotImplementedError

InspectLoader.register(machinery.BuiltinImporter)


class PyLoader(_bootstrap.PyLoader, InspectLoader):

    """Abstract base class to assist in loading source code by requiring only
    back-end storage methods to be implemented.

    The methods get_code, get_source, and load_module are implemented for the
    user.

    """

    @abc.abstractmethod
    def source_path(self, fullname:str) -> object:
        """Abstract method which when implemented should return the path to the
        sourced code for the module."""
        raise NotImplementedError


class PyPycLoader(_bootstrap.PyPycLoader, PyLoader):

    """Abstract base class to assist in loading source and bytecode by
    requiring only back-end storage methods to be implemented.

    The methods get_code, get_source, and load_module are implemented for the
    user.

    """

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
    def write_bytecode(self, fullname:str, bytecode:bytes):
        """Abstract method which when implemented should attempt to write the
        bytecode for the module."""
        raise NotImplementedError
