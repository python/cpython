# A more or less complete user-defined wrapper around dictionary objects

class UserDict:
    def __init__(self): self.data = {}
    def __repr__(self): return repr(self.data)
    def __cmp__(self, dict):
	if type(dict) == type(self.data):
	    return cmp(self.data, dict)
	else:
	    return cmp(self.data, dict.data)
    def __len__(self): return len(self.data)
    def __getitem__(self, key): return self.data[key]
    def __setitem__(self, key, item): self.data[key] = item
    def __delitem__(self, key): del self.data[key]
    def clear(self): return self.data.clear()
    def copy(self):
	import copy
	return copy.copy(self)
    def keys(self): return self.data.keys()
    def items(self): return self.data.items()
    def values(self): return self.data.values()
    def has_key(self, key): return self.data.has_key(key)
    def update(self, other):
	if type(other) is type(self.data):
	    self.data.update(other)
	else:
	    for k, v in other.items():
		self.data[k] = v
    def get(self, key, failobj=None):
	if self.data.has_key(key):
	    return self.data[key]
	else:
	    return failobj
