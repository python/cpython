from . import scan
from .supported import is_supported, ignored_from_file, known_from_file


def statics(dirnames, ignored, known, *,
            _load_ignored=ignored_from_file,
            _load_known=known_from_file,
            _iter_statics=scan.iter_statics,
            _is_supported=is_supported,
            ):
    """Return a list of (StaticVar, <supported>) for each found static var."""
    if not dirnames:
        return []

    ignored = set(_load_ignored(ignored)) if ignored else ()
    known = set(_load_known(known)) if known else ()
    found = []
    for static in _iter_statics(dirnames):
        found.append(
                (static, _is_supported(static, ignored, known)))
    return found
