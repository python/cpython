"""A generic interface to all dbm clones.

Instead of

	import dbm
	d = dbm.open(file, 'rw', 0666)

use

	import anydbm
	d = anydbm.open(file)

The returned object is a dbm, gdbm or (on the Mac) dbmac object,
dependent on availability of the modules (tested in this order).

It has the following interface (key and data are strings):

	d[key] = data	# store data at key (may override data at
			# existing key)
	data = d[key]	# retrieve data at key (raise KeyError if no
			# such key)
	del d[key]	# delete data stored at key (raises KeyError
			# if no such key)
	flag = d.has_key(key)	# true if the key exists
	list = d.keys()	# return a list of all existing keys (slow!)

Future versions may change the order in which implementations are
tested for existence, add interfaces to other db-like implementations
(e.g. BSD Hash), and (in the presence of multiple implementations)
decide which module to use based upon the extension or contents of an
existing database file.

The open function has an optional second argument.  This can be set to
'r' to open the database for reading only.  Don't pas an explicit 'w'
or 'rw' to open it for writing, as the different interfaces have
different interpretation of their mode argument if it isn't 'r'.
"""

try:
	import dbm
	def open(filename, mode = 'rw'):
		return dbm.open(filename, mode, 0666)
except ImportError:
	try:
		import gdbm
		def open(filename, mode = 'w'):
			return gdbm.open(filename, mode, 0666)
	except ImportError:
		import dbmac
		open = dbmac.open
