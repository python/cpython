"""Generic interface to all dbm clones.

Instead of

	import dbm
	d = dbm.open(file, 'w', 0666)

use

	import anydbm
	d = anydbm.open(file)

The returned object is a dbhash, gdbm, dbm or dumbdbm object,
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
tested for existence, add interfaces to other dbm-like
implementations, and (in the presence of multiple implementations)
decide which module to use based upon the extension or contents of an
existing database file.

The open function has an optional second argument.  This can be set to
'r' to open the database for reading only.  The default is 'r', like
the dbm default.

"""

_names = ['dbhash', 'gdbm', 'dbm', 'dumbdbm']

for _name in _names:
	try:
		exec "import %s; _mod = %s" % (_name, _name)
	except ImportError:
		continue
	else:
		break
else:
	raise ImportError, "no dbm clone found; tried %s" % _names
def open(file, flag = 'r', mode = 0666):
	return _mod.open(file, flag, mode)
