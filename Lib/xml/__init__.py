"""Core XML support for Python.

This package contains three sub-packages:

dom -- The W3C Document Object Model.  This supports DOM Level 1 +
       Namespaces.

parsers -- Python wrappers for XML parsers (currently only supports Expat).

sax -- The Simple API for XML, developed by XML-Dev, led by David
       Megginson and ported to Python by Lars Marius Garshol.  This
       supports the SAX 2 API.
"""


__all__ = ["dom", "parsers", "sax"]

__version__ = "$Revision$".split()[1]


_MINIMUM_XMLPLUS_VERSION = (0, 6, 1)


try:
    import _xmlplus
except ImportError:
    pass
else:
    try:
        v = _xmlplus.version_info
    except AttributeError:
        # _xmlplue is too old; ignore it
        pass
    else:
        if v >= _MINIMUM_XMLPLUS_VERSION:
            import sys
            sys.modules[__name__] = _xmlplus
        else:
            del v
