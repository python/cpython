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


def mapping(dict=None):
    return WeakDictionary(dict)


class WeakDictionary(UserDict.UserDict):

    # We inherit the constructor without worrying about the input
    # dictionary; since it uses our .update() method, we get the right
    # checks (if the other dictionary is a WeakDictionary, objects are
    # unwrapped on the way out, and we always wrap on the way in).

    def __getitem__(self, key):
        o = self.data.get(key)()
        if o is None:
            raise KeyError, key
        else:
            return o

    def __repr__(self):
        return "<WeakDictionary at %s>" % id(self)

    def __setitem__(self, key, value):
        def remove(o, data=self.data, key=key):
            del data[key]
        self.data[key] = ref(value, remove)

    def copy(self):
        new = WeakDictionary()
        for key, ref in self.data.items():
            o = ref()
            if o is not None:
                new[key] = o

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


# no longer needed
del UserDict
