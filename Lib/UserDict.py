"""A more or less complete user-defined wrapper around dictionary objects."""

class UserDict:
    def __init__(self, dict=None):
        self.data = {}
        if dict is not None: self.update(dict)
    def __repr__(self): return repr(self.data)
    def __cmp__(self, dict):
        if isinstance(dict, UserDict):
            return cmp(self.data, dict.data)
        else:
            return cmp(self.data, dict)
    def __len__(self): return len(self.data)
    def __getitem__(self, key): return self.data[key]
    def __setitem__(self, key, item): self.data[key] = item
    def __delitem__(self, key): del self.data[key]
    def clear(self): self.data.clear()
    def copy(self):
        if self.__class__ is UserDict:
            return UserDict(self.data)
        import copy
        data = self.data
        try:
            self.data = {}
            c = copy.copy(self)
        finally:
            self.data = data
        c.update(self)
        return c
    def keys(self): return self.data.keys()
    def items(self): return self.data.items()
    def iteritems(self): return self.data.iteritems()
    def iterkeys(self): return self.data.iterkeys()
    def itervalues(self): return self.data.itervalues()
    def values(self): return self.data.values()
    def has_key(self, key): return self.data.has_key(key)
    def update(self, dict):
        if isinstance(dict, UserDict):
            self.data.update(dict.data)
        elif isinstance(dict, type(self.data)):
            self.data.update(dict)
        else:
            for k, v in dict.items():
                self[k] = v
    def get(self, key, failobj=None):
        if not self.has_key(key):
            return failobj
        return self[key]
    def setdefault(self, key, failobj=None):
        if not self.has_key(key):
            self[key] = failobj
        return self[key]
    def pop(self, key):
        return self.data.pop(key)
    def popitem(self):
        return self.data.popitem()
    def __contains__(self, key):
        return key in self.data

class IterableUserDict(UserDict):
    def __iter__(self):
        return iter(self.data)

class DictMixin:
    '''Mixin defining all dictionary methods for classes that already have
       a minimum dictionary interface including getitem, setitem, delitem,
       and keys '''

    # first level provided by subclass: getitem, setitem, delitem, and keys

    # second level definitions which assume only getitem and keys
    def has_key(self, key):
        try:
            value = self[key]
        except KeyError:
            return False
        return True
    __contains__ = has_key
    def __iter__(self):
        for k in self.keys():
            yield k
    def __len__(self):
        return len(self.keys())

    # third level uses second level instead of first
    def iteritems(self):
        for k in self:
            yield (k, self[k])
    iterkeys = __iter__

    # fourth level uses second and third levels instead of first
    def itervalues(self):
        for _, v in self.iteritems():
            yield v
    def values(self):
        return [self[key] for key in self.keys()]
    def items(self):
        return list(self.iteritems())
    def clear(self):
        for key in self.keys():
            del self[key]
    def setdefault(self, key, default):
        if key not in self:
            self[key] = default
            return default
        return self[key]
    def pop(self, key):
        value = self[key]
        del self[key]
        return value
    def popitem(self):
        try:
            k, v = self.iteritems().next()
        except StopIteration:
            raise KeyError, 'dictionary is empty'
        del self[k]
        return (k, v)
    def update(self, other):
        for key in other.keys():
            self[key] = other[key]
    def get(self, key, default=None):
        if key in self:
            return self[key]
        return default
    def __repr__(self):
        return repr(dict(self.items()))
