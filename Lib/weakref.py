"""Weak reference support for Python.

This module is an implementation of PEP 205:

http://python.sourceforge.net/peps/pep-0205.html
"""

# Naming convention: Variables named "wr" are weak reference objects;
# they are called this instead of "ref" to avoid name collisions with
# the module-global ref() function imported from _weakref.

import UserDict

from _weakref import \
     getweakrefcount, \
     getweakrefs, \
     ref, \
     proxy, \
     ReferenceError, \
     CallableProxyType, \
     ProxyType, \
     ReferenceType

ProxyTypes = (ProxyType, CallableProxyType)

__all__ = ["ref", "proxy", "getweakrefcount", "getweakrefs",
           "WeakKeyDictionary", "ReferenceType", "ProxyType",
           "CallableProxyType", "ProxyTypes", "WeakValueDictionary"]


class WeakValueDictionary(UserDict.UserDict):
    """Mapping class that references values weakly.

    Entries in the dictionary will be discarded when no strong
    reference to the value exists anymore
    """
    # We inherit the constructor without worrying about the input
    # dictionary; since it uses our .update() method, we get the right
    # checks (if the other dictionary is a WeakValueDictionary,
    # objects are unwrapped on the way out, and we always wrap on the
    # way in).

    def __getitem__(self, key):
        o = self.data.get(key)()
        if o is None:
            raise KeyError, key
        else:
            return o

    def __repr__(self):
        return "<WeakValueDictionary at %s>" % id(self)

    def __setitem__(self, key, value):
        def remove(o, data=self.data, key=key):
            del data[key]
        self.data[key] = ref(value, remove)

    def copy(self):
        new = WeakValueDictionary()
        for key, wr in self.data.items():
            o = wr()
            if o is not None:
                new[key] = o
        return new

    def get(self, key, default=None):
        try:
            wr = self.data[key]
        except KeyError:
            return default
        else:
            o = wr()
            if o is None:
                # This should only happen
                return default
            else:
                return o

    def items(self):
        L = []
        for key, wr in self.data.items():
            o = wr()
            if o is not None:
                L.append((key, o))
        return L

    def popitem(self):
        while 1:
            key, wr = self.data.popitem()
            o = wr()
            if o is not None:
                return key, o

    def setdefault(self, key, default):
        try:
            wr = self.data[key]
        except KeyError:
            def remove(o, data=self.data, key=key):
                del data[key]
            self.data[key] = ref(default, remove)
            return default
        else:
            return wr()

    def update(self, dict):
        d = self.data
        for key, o in dict.items():
            def remove(o, data=d, key=key):
                del data[key]
            d[key] = ref(o, remove)

    def values(self):
        L = []
        for wr in self.data.values():
            o = wr()
            if o is not None:
                L.append(o)
        return L


class WeakKeyDictionary(UserDict.UserDict):
    """ Mapping class that references keys weakly.

    Entries in the dictionary will be discarded when there is no
    longer a strong reference to the key. This can be used to
    associate additional data with an object owned by other parts of
    an application without adding attributes to those objects. This
    can be especially useful with objects that override attribute
    accesses.
    """

    def __init__(self, dict=None):
        self.data = {}
        if dict is not None: self.update(dict)
        def remove(k, data=self.data):
            del data[k]
        self._remove = remove

    def __getitem__(self, key):
        return self.data[ref(key)]

    def __repr__(self):
        return "<WeakKeyDictionary at %s>" % id(self)

    def __setitem__(self, key, value):
        self.data[ref(key, self._remove)] = value

    def copy(self):
        new = WeakKeyDictionary()
        for key, value in self.data.items():
            o = key()
            if o is not None:
                new[o] = value
        return new

    def get(self, key, default=None):
        return self.data.get(ref(key),default)

    def has_key(self, key):
        return self.data.has_key(ref(key))

    def items(self):
        L = []
        for key, value in self.data.items():
            o = key()
            if o is not None:
                L.append((o, value))
        return L

    def keys(self):
        L = []
        for wr in self.data.keys():
            o = wr()
            if o is not None:
                L.append(o)
        return L

    def popitem(self):
        while 1:
            key, value = self.data.popitem()
            o = key()
            if o is not None:
                return o, value

    def setdefault(self, key, default):
        return self.data.setdefault(ref(key, self._remove),default)

    def update(self, dict):
        d = self.data
        for key, value in dict.items():
            d[ref(key, self._remove)] = value


# no longer needed
del UserDict
