"""Minimal W application."""

import Wapplication
import macresource
import os

class TestApp(Wapplication.Application):

    def __init__(self):
        from Carbon import Res
#               macresource.open_pathname("Widgets.rsrc")
        self._menustocheck = []
        self.preffilepath = os.path.join("Python", "PythonIDE preferences")
        Wapplication.Application.__init__(self, 'Pyth')
        # open a new text editor
        import PyEdit
        PyEdit.Editor()
        # start the mainloop
        self.mainloop()


TestApp()
