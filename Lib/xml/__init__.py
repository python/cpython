"""Core XML support for Python.

This package contains three sub-packages:

dom -- The W3C Document Object Model.  This supports DOM Level 1 +
       Namespaces.

parsers -- Python wrappers for XML parsers (currently only supports Expat).

sax -- The Simple API for XML, developed by XML-Dev, led by David
       Megginson and ported to Python by Lars Marius Garshol.  This 
       supports the SAX 2 API.
"""


if __name__ == "xml":
    try:
        import _xmlplus
    except ImportError:
        pass
    else:
        import sys
        sys.modules[__name__] = _xmlplus
