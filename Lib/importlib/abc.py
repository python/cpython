"""Abstract base classes related to import."""
from . import _bootstrap
from . import machinery
import abc
import types


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders.

    See PEP 302 for details.

    """

    def load_module(self, fullname:str) -> types.ModuleType:
        raise NotImplementedError

Loader.register(machinery.BuiltinImporter)
Loader.register(machinery.FrozenImporter)


class Finder(metaclass=abc.ABCMeta):

    """Abstract base class for import finders.

    See PEP 302 for details.

    """

    @abc.abstractmethod
    def find_module(self, fullname:str, path:[str]=None) -> Loader:
        raise NotImplementedError

Finder.register(machinery.BuiltinImporter)
Finder.register(machinery.FrozenImporter)
Finder.register(machinery.PathFinder)


class Importer(Finder, Loader):

    """Abstract base class for importers."""



class ResourceLoader(Loader):

    """Abstract base class for loaders which can return data from the back-end
    storage.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def get_data(self, path:str) -> bytes:
        raise NotImplementedError


class InspectLoader(Loader):

    """Abstract base class for loaders which supports introspection.

    This ABC represents one of the optional protocols specified by PEP 302.

    """

    @abc.abstractmethod
    def is_package(self, fullname:str) -> bool:
        return NotImplementedError

    @abc.abstractmethod
    def get_code(self, fullname:str) -> types.CodeType:
        return NotImplementedError

    @abc.abstractmethod
    def get_source(self, fullname:str) -> str:
        return NotImplementedError


class PyLoader(_bootstrap.PyLoader, InspectLoader):

    """Abstract base class that implements the core parts needed to load Python
    source code."""

    # load_module and get_code are implemented.

    @abc.abstractmethod
    def source_path(self, fullname:str) -> object:
        raise NotImplementedError


class PyPycLoader(_bootstrap.PyPycLoader, PyLoader):

    """Abstract base class that implements the core parts needed to load Python
    source and bytecode."""

    # Implements load_module and get_code.

    @abc.abstractmethod
    def source_mtime(self, fullname:str) -> int:
        raise NotImplementedError

    @abc.abstractmethod
    def bytecode_path(self, fullname:str) -> object:
        raise NotImplementedError

    @abc.abstractmethod
    def write_bytecode(self, fullname:str, bytecode:bytes):
        raise NotImplementedError
