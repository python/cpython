# copyright 1996-2001 Just van Rossum, Letterror. just@letterror.com

# keep this (__main__) as clean as possible, since we are using 
# it like the "normal" interpreter.

__version__ = '1.0.1'
import sys
import os

def init():
	import MacOS
	if hasattr(MacOS, 'EnableAppswitch'):
		MacOS.EnableAppswitch(-1)
	
	from Carbon import Qd, QuickDraw
	Qd.SetCursor(Qd.GetCursor(QuickDraw.watchCursor).data)
	
	import macresource
	import sys, os
	macresource.need('DITL', 468, "PythonIDE.rsrc")
	widgetrespathsegs = [sys.exec_prefix, "Mac", "Tools", "IDE", "Widgets.rsrc"]
	widgetresfile = os.path.join(*widgetrespathsegs)
	refno = macresource.need('CURS', 468, widgetresfile)
	if refno:
		# We're not a fullblown application
		idepathsegs = [sys.exec_prefix, "Mac", "Tools", "IDE"]
		ide_path = os.path.join(*idepathsegs)
	else:
		# We are a fully frozen application
		ide_path = sys.argv[0]
	if ide_path not in sys.path:
		sys.path.insert(0, ide_path)


init()
del init

import PythonIDEMain
