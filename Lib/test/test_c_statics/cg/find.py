import os
import os.path

from . import info, scan
from .supported import is_supported


def _iter_files(dirnames):
    for dirname in dirnames:
        for parent, _, names in os.walk(dirname):
            for name in names:
                yield os.path.join(parent, name)


def _iter_statics(dirnames):
    for filename in _iter_files(dirnames):
        yield from scan.iter_statics(filename)


def statics(dirnames, ignored, known, *,
            _iter_statics=_iter_statics,
            _is_supported=is_supported,
            ):
    """Return a list of (StaticVar, <supported>) for each found static var."""
    if not dirnames:
        return []

    found = []
    for static in _iter_statics(dirnames):
        found.append(
                (static, _is_supported(static)))
    return found
