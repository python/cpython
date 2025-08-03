"""
Compatibility shim for .resources.readers as found on Python 3.10.

Consumers that can rely on Python 3.11 should use the other
module directly.
"""

from .resources.readers import (
    FileReader,
    MultiplexedPath,
    NamespaceReader,
    ZipReader,
)

__all__ = ['FileReader', 'ZipReader', 'MultiplexedPath', 'NamespaceReader']
