"""Read resources contained within a package."""

from ._common import (
    as_file,
    files,
    Package,
)

from .abc import ResourceReader


__all__ = [
    'Package',
    'ResourceReader',
    'as_file',
    'files',
]
