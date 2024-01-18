"""Read resources contained within a package."""

from ._common import (
    as_file,
    files,
    Package,
    Anchor,
)

from .abc import ResourceReader


__all__ = [
    'Package',
    'Anchor',
    'ResourceReader',
    'as_file',
    'files',
]
