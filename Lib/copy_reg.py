"""Helper to provide extensibility for pickle/cPickle.

This is only useful to add pickle support for extension types defined in
C, not for instances of user-defined classes.
"""

from types import ClassType as _ClassType

__all__ = ["pickle","constructor"]

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

_dummy_classes = {}

def _reconstructor(cls, base, state):
    dummy = _dummy_classes.get(base)
    if dummy is None:
        class dummy(base): pass
        _dummy_classes[base] = dummy
    obj = dummy(state)
    obj._foo = 1; del obj._foo # hack to create __dict__
    obj.__class__ = cls
    return obj
_reconstructor.__safe_for_unpickling__ = 1

_HEAPTYPE = 1<<9

def _reduce(self):
    for base in self.__class__.__mro__:
        if not base.__flags__ & _HEAPTYPE:
            break
    else:
        base = object # not really reachable
    if base is object:
        state = None
    else:
        state = base(self)
    return _reconstructor, (self.__class__, base, state), self.__dict__
