# copyright 1996-1999 Just van Rossum, Letterror. just@letterror.com

# keep this (__main__) as clean as possible, since we are using 
# it like the "normal" interpreter.

__version__ = '1.0b2'


def init():
	import MacOS
	MacOS.EnableAppswitch(-1)
	
	import Qd, QuickDraw
	Qd.SetCursor(Qd.GetCursor(QuickDraw.watchCursor).data)
	
	import Res
	try:
		Res.GetResource('DITL', 468)
	except Res.Error:
		# we're not an applet
		Res.OpenResFile('Widgets.rsrc')
		Res.OpenResFile('PythonIDE.rsrc')
	else:
		# we're an applet
		import sys
		if sys.argv[0] not in sys.path:
			sys.path[2:2] = [sys.argv[0]]


init()
del init

import PythonIDEMain
