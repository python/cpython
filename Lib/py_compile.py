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
	codestring = f.read()
	f.close()
	timestamp = os.stat(file)[8]
	codeobject = __builtin__.compile(codestring, file, 'exec')
	if not cfile:
		cfile = file + 'c'
	fc = open(cfile, 'wb')
	fc.write(MAGIC)
	wr_long(fc, timestamp)
	marshal.dump(codeobject, fc)
	fc.close()
	if os.name == 'mac':
		import macfs
		macfs.FSSpec(cfile).SetCreatorType('PYTH', 'PYC ')
		macfs.FSSpec(file).SetCreatorType('PYTH', 'TEXT')
