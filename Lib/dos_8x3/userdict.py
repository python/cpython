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
	def keys(self): return self.data.keys()
	def items(self): return self.data.items()
	def values(self): return self.data.values()
	def has_key(self, key): return self.data.has_key(key)
