"""Helper to provide extensibility for pickle/cPickle.

This is only useful to add pickle support for extension types defined in
C, not for instances of user-defined classes.
"""

from types import ClassType as _ClassType

__all__ = ["pickle", "constructor",
           "add_extension", "remove_extension", "clear_extension_cache"]

dispatch_table = {}
safe_constructors = {}

def pickle(ob_type, pickle_function, constructor_ob=None):
    if type(ob_type) is _ClassType:
        raise TypeError("copy_reg is not intended for use with classes")

    if not callable(pickle_function):
        raise TypeError("reduction functions must be callable")
    dispatch_table[ob_type] = pickle_function

    if constructor_ob is not None:
        constructor(constructor_ob)

def constructor(object):
    if not callable(object):
        raise TypeError("constructors must be callable")
    safe_constructors[object] = 1

# Example: provide pickling support for complex numbers.

def pickle_complex(c):
    return complex, (c.real, c.imag)

pickle(type(1j), pickle_complex, complex)

# Support for picking new-style objects

def _reconstructor(cls, base, state):
    obj = base.__new__(cls, state)
    base.__init__(obj, state)
    return obj
_reconstructor.__safe_for_unpickling__ = 1

_HEAPTYPE = 1<<9

def _reduce(self):
    for base in self.__class__.__mro__:
        if hasattr(base, '__flags__') and not base.__flags__ & _HEAPTYPE:
            break
    else:
        base = object # not really reachable
    if base is object:
        state = None
    else:
        if base is self.__class__:
            raise TypeError, "can't pickle %s objects" % base.__name__
        state = base(self)
    args = (self.__class__, base, state)
    try:
        getstate = self.__getstate__
    except AttributeError:
        try:
            dict = self.__dict__
        except AttributeError:
            dict = None
    else:
        dict = getstate()
    if dict:
        return _reconstructor, args, dict
    else:
        return _reconstructor, args

# A registry of extension codes.  This is an ad-hoc compression
# mechanism.  Whenever a global reference to <module>, <name> is about
# to be pickled, the (<module>, <name>) tuple is looked up here to see
# if it is a registered extension code for it.  Extension codes are
# universal, so that the meaning of a pickle does not depend on
# context.  (There are also some codes reserved for local use that
# don't have this restriction.)  Codes are positive ints; 0 is
# reserved.

extension_registry = {}                 # key -> code
inverted_registry = {}                  # code -> key
extension_cache = {}                    # code -> object

def add_extension(module, name, code):
    """Register an extension code."""
    code = int(code)
    if not 1 <= code < 0x7fffffff:
        raise ValueError, "code out of range"
    key = (module, name)
    if (extension_registry.get(key) == code and
        inverted_registry.get(code) == key):
        return # Redundant registrations are benign
    if key in extension_registry:
        raise ValueError("key %s is already registered with code %s" %
                         (key, extension_registry[key]))
    if code in inverted_registry:
        raise ValueError("code %s is already in use for key %s" %
                         (code, inverted_registry[code]))
    extension_registry[key] = code
    inverted_registry[code] = key

def remove_extension(module, name, code):
    """Unregister an extension code.  For testing only."""
    key = (module, name)
    if (extension_registry.get(key) != code or
        inverted_registry.get(code) != key):
        raise ValueError("key %s is not registered with code %s" %
                         (key, code))
    del extension_registry[key]
    del inverted_registry[code]
    if code in extension_cache:
        del extension_cache[code]

def clear_extension_cache():
    extension_cache.clear()

# Standard extension code assignments

# Reserved ranges

# First  Last Count  Purpose
#     1   127   127  Reserved for Python standard library
#   128   191    64  Reserved for Zope 3
#   192   239    48  Reserved for 3rd parties
#   240   255    16  Reserved for private use (will never be assigned)
#   256   Inf   Inf  Reserved for future assignment

# Extension codes are assigned by the Python Software Foundation.
