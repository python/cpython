from _collections_abc import *
from _collections_abc import __all__
from _collections_abc import _CallableGenericAlias

_deprecated_ByteString = globals().pop("ByteString")

def __getattr__(attr):
    if attr == "ByteString":
        import warnings
        warnings._deprecated("collections.abc.ByteString", remove=(3, 14))
        return _deprecated_ByteString
    raise AttributeError(f"module 'collections.abc' has no attribute {attr!r}")
