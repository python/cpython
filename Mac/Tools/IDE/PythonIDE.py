# copyright 1996-2001 Just van Rossum, Letterror. just@letterror.com

# keep this (__main__) as clean as possible, since we are using 
# it like the "normal" interpreter.

__version__ = '1.0.1'


def init():
	import MacOS
	MacOS.EnableAppswitch(-1)
	
	import Qd, QuickDraw
	Qd.SetCursor(Qd.GetCursor(QuickDraw.watchCursor).data)
	
	import Res, sys, os
	try:
		Res.GetResource('DITL', 468)
	except Res.Error:
		# we're not an applet
		Res.FSpOpenResFile(os.path.join(sys.exec_prefix, ":Mac:Tools:IDE:PythonIDE.rsrc"), 1)
		Res.FSpOpenResFile(os.path.join(sys.exec_prefix, ":Mac:Tools:IDE:Widgets.rsrc"), 1)
		ide_path = os.path.join(sys.exec_prefix, ":Mac:Tools:IDE")
	else:
		# we're an applet
		try:
			Res.GetResource('CURS', 468)
		except Res.Error:
			Res.FSpOpenResFile(os.path.join(sys.exec_prefix, ":Mac:Tools:IDE:Widgets.rsrc"), 1)
			ide_path = os.path.join(sys.exec_prefix, ":Mac:Tools:IDE")
		else:
			# we're a full blown applet
			ide_path = sys.argv[0]
	if ide_path not in sys.path:
		sys.path.insert(0, ide_path)


init()
del init

import PythonIDEMain
