"""
An example project.

@added: exampleproj NEXT
"""

from incremental import Version
from ._version import __version__

__all__ = ["__version__"]

if Version("exampleproj", "NEXT", 0, 0) > __version__:
    print("Unreleased!")
