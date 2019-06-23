from . import scan
from .supported import is_supported


def statics(dirnames, ignored, known, *,
            _iter_statics=scan.iter_statics,
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
