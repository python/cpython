"""twit - The Window-Independent Tracer.

Interface:
twit.main()						Enter debugger in inactive interactive state
twit.run(stmt, globals, locals)	Enter debugger and start running stmt
twit.post_mortem(traceback)		Enter debugger in post-mortem mode on traceback
twit.pm()						Enter debugger in pm-mode on sys.last_traceback

main program: nothing but a bit of glue to put it all together.

Jack Jansen, CWI, August 1996."""

import os
if os.name == 'mac':
# Not supported in distributed 1.4b3:
##	import MacOS
##	MacOS.splash(515)	# Try to show the splash screen
	import mactwit_mod; twit_mod = mactwit_mod
	import mactwit_stack; twit_stack = mactwit_stack
	import mactwit_app; twit_app = mactwit_app
	import mactwit_browser; twit_browser = mactwit_browser
	import mactwit_edit; twit_edit = mactwit_edit
else:
	try:
		import _tkinter
		have_tk = 1
	except ImportError:
		have_tk = 0
	if have_tk:
		import tktwit_mod; twit_mod = tktwit_mod
		import tktwit_stack; twit_stack = tktwit_stack
		import tktwit_app; twit_app = tktwit_app
	else:
		print 'Please implementent twit_mod, twit_stack and twit_app and try again:-)'
		sys.exit(1)
	
import TwitCore
import sys

class Twit(twit_app.Application, TwitCore.Application):

	def new_module_browser(self, *args):
		return apply(TWIT_ModuleBrowser, args)
		
	def new_stack_browser(self, *args):
		return apply(TWIT_StackBrowser, args)
		
	def new_var_browser(self, *args):
		return apply(TWIT_VarBrowser, args)
	
	def edit(self, *args):
		return apply(twit_edit.edit, args)
		
class TWIT_ModuleBrowser(twit_mod.ModuleBrowser, TwitCore.ModuleBrowser):
	pass
	
class TWIT_StackBrowser(twit_stack.StackBrowser, TwitCore.StackBrowser):
	pass
	
def TWIT_VarBrowser(parent, var):
	return twit_browser.VarBrowser(parent).open(var)
	
def Initialize():
	# Gross...
	TwitCore.AskString = twit_app.AskString
	TwitCore.ShowMessage = twit_app.ShowMessage
	TwitCore.SetWatch = twit_app.SetWatch
	TwitCore.SetCursor = twit_app.SetCursor
	
def main():
	twit_app.Initialize()
	TwitCore.Initialize()
	Initialize()
##	if os.name == 'mac':
##		MacOS.splash()
	Twit(None, None)
	
def run(statement, globals=None, locals=None):
	twit_app.Initialize()
	TwitCore.Initialize()
	Initialize()
	Twit((statement, globals, locals), None)

def post_mortem(t):
	twit_app.Initialize()
	TwitCore.Initialize()
	Initialize()
	Twit(None, t)
	
def pm():
	post_mortem(sys.last_traceback)
	
if __name__ == '__main__':
	main()
	
	
