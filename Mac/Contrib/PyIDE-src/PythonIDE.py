# copyright 1997 Just van Rossum, Letterror. just@knoware.nl

# keep this (__main__) as clean as possible, since we are using 
# it like the "normal" interpreter.

__version__ = '0.9b1'

def init():
	import sys
	import MacOS
	
	if sys.version[:5] == '1.5a3':
		def MyEnableAppswitch(yield, 
				table = {-1:0, 0:-1, 1:1}, 
				EnableAppswitch = MacOS.EnableAppswitch):
			return table[EnableAppswitch(table[yield])]
		MacOS.EnableAppswitch = MyEnableAppswitch
	
	MacOS.EnableAppswitch(-1)
	
	import Qd, QuickDraw
	Qd.SetCursor(Qd.GetCursor(QuickDraw.watchCursor).data)
	
	import os
	
	# kludge to keep stdwin's TextEdit.py out the door...
	import string
	for i in range(len(sys.path)):
		path = sys.path[i]
		if string.find(path, 'stdwin') > 0:
			del sys.path[i]
			break
	
	try:
		import SpecialKeys	# if this succeeds, we should have everything we need inside the applet.
		del SpecialKeys
	except ImportError:
		# personal hack for me
		wherearewe = os.getcwd()
		import Res, macfs
		if os.path.exists(os.path.join(wherearewe, 'IDELib')):
			sys.path.append(os.path.join(wherearewe, ':IDELib'))
			sys.path.append(os.path.join(wherearewe, ':IDELib:Widgets'))
			Res.FSpOpenResFile(macfs.FSSpec(os.path.join(wherearewe, ':IDELib:Resources:Widgets.rsrc')), 1)
			Res.FSpOpenResFile(macfs.FSSpec(os.path.join(wherearewe, 'PythonIDE.rsrc')), 1)
		else:
			oneback = os.path.split(wherearewe)[0]
			sys.path.append(os.path.join(oneback, ':Fog:Widgets'))
			Res.FSpOpenResFile(macfs.FSSpec(os.path.join(oneback, ':Fog:Resources:Widgets.rsrc')), 1)
			Res.FSpOpenResFile(macfs.FSSpec(os.path.join(wherearewe, 'PythonIDE.rsrc')), 1)
	
init()
del init

##import trace
##trace.set_trace()
import PythonIDEMain
