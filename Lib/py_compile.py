# Routine to "compile" a .py file to a .pyc file.
# This has intimate knowledge of how Python/import.c does it.
# By Sjoerd Mullender (I forced him to write it :-).

import imp
MAGIC = imp.get_magic()

def wr_long(f, x):
	f.write(chr( x        & 0xff))
	f.write(chr((x >> 8)  & 0xff))
	f.write(chr((x >> 16) & 0xff))
	f.write(chr((x >> 24) & 0xff))

def compile(file, cfile = None):
	import os, marshal, __builtin__
	f = open(file)
	try:
	    timestamp = os.fstat(file.fileno())
	except AttributeError:
	    timestamp = long(os.stat(file)[8])
	codestring = f.read()
	f.close()
	codeobject = __builtin__.compile(codestring, file, 'exec')
	if not cfile:
		cfile = file + (__debug__ and 'c' or 'o')
	fc = open(cfile, 'wb')
	fc.write('\0\0\0\0')
	wr_long(fc, timestamp)
	marshal.dump(codeobject, fc)
	fc.flush()
	fc.seek(0, 0)
	fc.write(MAGIC)
	fc.close()
	if os.name == 'mac':
		import macfs
		macfs.FSSpec(cfile).SetCreatorType('Pyth', 'PYC ')
		macfs.FSSpec(file).SetCreatorType('Pyth', 'TEXT')
