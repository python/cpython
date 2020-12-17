from __future__ import absolute_import, unicode_literals

import abc

from six import add_metaclass

from virtualenv.create.describe import PosixSupports, Python3Supports, WindowsSupports
from virtualenv.create.via_global_ref.builtin.ref import PathRefToDest
from virtualenv.util.path import Path

from .common import PyPy


@add_metaclass(abc.ABCMeta)
class PyPy3(PyPy, Python3Supports):
    @classmethod
    def exe_stem(cls):
        return "pypy3"

    @property
    def stdlib(self):
        """
        PyPy3 seems to respect sysconfig only for the host python...
        virtual environments purelib is instead lib/pythonx.y
        """
        return self.dest / "lib" / "python{}".format(self.interpreter.version_release_str) / "site-packages"

    @classmethod
    def exe_names(cls, interpreter):
        return super(PyPy3, cls).exe_names(interpreter) | {"pypy"}


class PyPy3Posix(PyPy3, PosixSupports):
    """PyPy 2 on POSIX"""

    @classmethod
    def _shared_libs(cls):
        return ["libpypy3-c.so", "libpypy3-c.dylib"]

    def to_lib(self, src):
        return self.dest / "lib" / src.name

    @classmethod
    def sources(cls, interpreter):
        for src in super(PyPy3Posix, cls).sources(interpreter):
            yield src
        host_lib = Path(interpreter.system_prefix) / "lib"
        if host_lib.exists() and host_lib.is_dir():
            for path in host_lib.iterdir():
                yield PathRefToDest(path, dest=cls.to_lib)


class Pypy3Windows(PyPy3, WindowsSupports):
    """PyPy 2 on Windows"""

    @property
    def bin_dir(self):
        """PyPy3 needs to fallback to pypy definition"""
        return self.dest / "Scripts"

    @classmethod
    def _shared_libs(cls):
        return ["libpypy3-c.dll"]
