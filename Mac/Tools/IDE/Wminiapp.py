"""Minimal W application."""

import Wapplication

class TestApp(Wapplication.Application):
	
	def __init__(self):
		import Res
		Res.FSpOpenResFile("Widgets.rsrc", 1)
		self._menustocheck = []
		self.preffilepath = ":Python:PythonIDE preferences"
		Wapplication.Application.__init__(self, 'Pyth')
		# open a new text editor
		import PyEdit
		PyEdit.Editor()
		# start the mainloop
		self.mainloop()
	

TestApp()
