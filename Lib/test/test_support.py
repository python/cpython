# Python test set -- supporting definitions.

TestFailed = 'test_support -- test failed'	# Exception

def unload(name):
	import sys
	try:
		del sys.modules[name]
	except KeyError:
		pass

def forget(modname):
	unload(modname)
	import sys, os
	for dirname in sys.path:
		try:
			os.unlink(os.path.join(dirname, modname + '.pyc'))
		except os.error:
			pass

TESTFN = '@test' # Filename used for testing
from os import unlink
