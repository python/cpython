"""
Infinite C recursion involving PyObject_GetAttr in slot_tp_new.
"""

class X(object):
    class __metaclass__(type):
        pass
    __new__ = 5

X.__metaclass__.__new__ = property(X)
print X()
