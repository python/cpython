"""twit - The Window-Independent Tracer.

Interface:
twit.main()						Enter debugger in inactive interactive state
twit.run(stmt, globals, locals)	Enter debugger and start running stmt
twit.post_mortem(traceback)		Enter debugger in post-mortem mode on traceback
twit.pm()						Enter debugger in pm-mode on sys.last_traceback

main program: nothing but a bit of glue to put it all together.

Jack Jansen, CWI, August 1996."""

import os
import sys

# Add our directory to path, if needed
dirname = os.path.split(__file__)[0]
if not dirname in sys.path:
	sys.path.append(dirname)

if os.name == 'mac':
	import MacOS
	MacOS.splash(502)	# Try to show the splash screen
	import mactwit_app; twit_app = mactwit_app
else:
	try:
		import _tkinter
		have_tk = 1
	except ImportError:
		have_tk = 0
	if have_tk:
		import tktwit_app; twit_app = tktwit_app
	else:
		print 'Please implementent machine-dependent code and try again:-)'
		sys.exit(1)
	
import sys
	
def main():
	twit_app.Initialize()
	if os.name == 'mac':
		MacOS.splash()
	twit_app.Twit('none', None)
	
def run(statement, globals=None, locals=None):
	twit_app.Initialize()
	twit_app.Twit('run', (statement, globals, locals))

def post_mortem(t):
	Initialize()
	twit_app.Twit('pm', t)
	
def pm():
	post_mortem(sys.last_traceback)
	
if __name__ == '__main__':
	main()
	
	
