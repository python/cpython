"""A generic interface to all dbm clones."""

try:
	import dbm
	def open(file, mode = 'rw'):
		return dbm.open(file, mode, 0666)
except ImportError:
	import dbmac
	open = dbmac.open
