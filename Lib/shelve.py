"""Manage shelves of pickled objects.

A "shelf" is a persistent, dictionary-like object.  The difference
with dbm databases is that the values (not the keys!) in a shelf can
be essentially arbitrary Python objects -- anything that the "pickle"
module can handle.  This includes most class instances, recursive data
types, and objects containing lots of shared sub-objects.  The keys
are ordinary strings.

To summarize the interface (key is a string, data is an arbitrary
object):

	import shelve
	d = shelve.open(filename) # open, with (g)dbm filename -- no suffix

	d[key] = data	# store data at key (overwrites old data if
			# using an existing key)
	data = d[key]	# retrieve data at key (raise KeyError if no
			# such key)
	del d[key]	# delete data stored at key (raises KeyError
			# if no such key)
	flag = d.has_key(key)	# true if the key exists
	list = d.keys()	# a list of all existing keys (slow!)

	d.close()	# close it

Dependent on the implementation, closing a persistent dictionary may
or may not be necessary to flush changes to disk.
"""

import pickle
import StringIO


class Shelf:
	"""Base class for shelf implementations.

	This is initialized with a dictionary-like object.
	See the module's __doc__ string for an overview of the interface.
	"""

	def __init__(self, dict):
		self.dict = dict
	
	def keys(self):
		return self.dict.keys()
	
	def __len__(self):
		return self.dict.len()
	
	def has_key(self, key):
		return self.dict.has_key(key)
	
	def __getitem__(self, key):
		f = StringIO.StringIO(self.dict[key])
		return pickle.Unpickler(f).load()
	
	def __setitem__(self, key, value):
		f = StringIO.StringIO()
		p = pickle.Pickler(f)
		p.dump(value)
		self.dict[key] = f.getvalue()
	
	def __delitem__(self, key):
		del self.dict[key]
	
	def close(self):
		if hasattr(self.dict, 'close'):
			self.dict.close()
		self.dict = None

	def __del__(self):
		self.close()


class BsdDbShelf(Shelf):
	"""Shelf implementation using the "BSD" db interface.

	The actual database is opened using one of thethe "bsddb" modules
	"open" routines (i.e. bsddb.hashopen, bsddb.btopen or bsddb.rnopen.)

	This class is initialized with the the database object
	returned from one of the bsddb open functions.

	See the module's __doc__ string for an overview of the interface.
	"""

	def __init__(self, dict):
	    Shelf.__init__(self, dict)

	def set_location(self, key):
	     (key, value) = self.dict.set_location(key)
	     f = StringIO.StringIO(value)
	     return (key, pickle.Unpickler(f).load())

	def next(self):
	     (key, value) = self.dict.next()
	     f = StringIO.StringIO(value)
	     return (key, pickle.Unpickler(f).load())

	def previous(self):
	     (key, value) = self.dict.previous()
	     f = StringIO.StringIO(value)
	     return (key, pickle.Unpickler(f).load())

	def first(self):
	     (key, value) = self.dict.first()
	     f = StringIO.StringIO(value)
	     return (key, pickle.Unpickler(f).load())

	def last(self):
	     (key, value) = self.dict.last()
	     f = StringIO.StringIO(value)
	     return (key, pickle.Unpickler(f).load())


class DbfilenameShelf(Shelf):
	"""Shelf implementation using the "anydbm" generic dbm interface.

	This is initialized with the filename for the dbm database.
	See the module's __doc__ string for an overview of the interface.
	"""
	
	def __init__(self, filename, flag='c'):
		import anydbm
		Shelf.__init__(self, anydbm.open(filename, flag))


def open(filename, flag='c'):
	"""Open a persistent dictionary for reading and writing.

	Argument is the filename for the dbm database.
	See the module's __doc__ string for an overview of the interface.
	"""
	
	return DbfilenameShelf(filename, flag)
