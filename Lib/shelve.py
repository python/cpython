"""Manage shelves of pickled objects."""

import pickle
import StringIO

class Shelf:

	def __init__(self, dict):
		self.dict = dict
	
	def keys(self):
		return self.dict.keys()
	
	def __len__(self):
		return self.dict.len()
	
	def has_key(self, key):
		return self.dict.has_key(key)
	
	def __getitem__(self, key):
		return pickle.Unpickler(StringIO.StringIO(self.dict[key])).load()
	
	def __setitem__(self, key, value):
		f = StringIO.StringIO()
		p = pickle.Pickler(f)
		p.dump(value)
		self.dict[key] = f.getvalue()
	
	def __delitem__(self, key):
		del self.dict[key]
	
	def close(self):
		self.db.close()
		self.db = None

class DbShelf(Shelf):
	
	def __init__(self, file):
		import anydbm
		Shelf.__init__(self, anydbm.open(file))

def open(file):
	return DbShelf(file)
