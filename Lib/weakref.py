"""Weak reference support for Python.

This module is an implementation of PEP 205:

http://python.sourceforge.net/peps/pep-0205.html
"""

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

__all__ = ["ref", "mapping", "proxy", "getweakrefcount", "getweakrefs",
           "WeakKeyDictionary", "ReferenceType", "ProxyType",
           "CallableProxyType", "ProxyTypes", "WeakValueDictionary"]

def mapping(dict=None,weakkeys=0):
    if weakkeys:
        return WeakKeyDictionary(dict)
    else:
        return WeakValueDictionary(dict)


class WeakValueDictionary(UserDict.UserDict):

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
        for key, ref in self.data.items():
            o = ref()
            if o is not None:
                new[key] = o
        return new

    def get(self, key, default):
        try:
            ref = self.data[key]
        except KeyError:
            return default
        else:
            o = ref()
            if o is None:
                # This should only happen
                return default
            else:
                return o

    def items(self):
        L = []
        for key, ref in self.data.items():
            o = ref()
            if o is not None:
                L.append((key, o))
        return L

    def popitem(self):
        while 1:
            key, ref = self.data.popitem()
            o = ref()
            if o is not None:
                return key, o

    def setdefault(self, key, default):
        try:
            ref = self.data[key]
        except KeyError:
            def remove(o, data=self.data, key=key):
                del data[key]
            ref = ref(default, remove)
            self.data[key] = ref
            return default
        else:
            return ref()

    def update(self, dict):
        d = self.data
        L = []
        for key, o in dict.items():
            def remove(o, data=d, key=key):
                del data[key]
            L.append(key, ref(o, remove))
        for key, r in L:
            d[key] = r

    def values(self):
        L = []
        for ref in self.data.values():
            o = ref()
            if o is not None:
                L.append(o)
        return L


class WeakKeyDictionary(UserDict.UserDict):

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

    def get(self, key, default):
        return self.data.get(ref(key),default)

    def items(self):
        L = []
        for key, value in self.data.items():
            o = key()
            if o is not None:
                L.append((o, value))
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
        L = []
        for key, value in dict.items():
            L.append(ref(key, self._remove), value)
        for key, r in L:
            d[key] = r

# no longer needed
del UserDict
