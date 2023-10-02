

from sqlite3.dbapi2 import *
from sqlite3.dbapi2 import (_deprecated_names,
                            _deprecated_version_info,
                            _deprecated_version)


def __getattr__(name):
    if name in _deprecated_names:
        from warnings import warn

        warn(f"{name} is deprecated and will be removed in Python 3.14",
             DeprecationWarning, stacklevel=2)
        return globals()[f"_deprecated_{name}"]
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
