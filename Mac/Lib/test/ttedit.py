# Test TE module.
# Draw a window in which the user can type.
#
# This test expects Win, Evt and FrameWork (and anything used by those)
# to work.
#
# Actually, it is more a test of FrameWork by now....

from FrameWork import *
import Win
import Qd
import TE
import os

class TEWindow(Window):
	def open(self, name):
		r = (40, 40, 400, 300)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0x55555555)
		r2 = (0, 0, 345, 245)
		Qd.SetPort(w)
		self.ted = TE.TENew(r2, r2)
		self.ted.TEAutoView(1)
		w.DrawGrowIcon()
		self.wid = w
		self.do_postopen()
		
	def do_idle(self):
		self.ted.TEIdle()
		
	def do_activate(self, onoff, evt):
		if onoff:
			self.ted.TEActivate()
		else:
			self.ted.TEDeactivate()

	def do_update(self, wid, event):
		Qd.EraseRect(wid.GetWindowPort().portRect)
		self.ted.TEUpdate(wid.GetWindowPort().portRect)
		
	def do_contentclick(self, local, modifiers, evt):
		shifted = (modifiers & 0x200)
		self.ted.TEClick(local, shifted)

	def do_char(self, ch, event):
		self.ted.TEKey(ord(ch))

class TestList(Application):
	def __init__(self):
		Application.__init__(self)
		self.num = 0
		self.listoflists = []
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.newitem = MenuItem(m, "New window...", "O", self.open)
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
	
	def open(self, *args):
		w = TEWindow(self)
		w.open('Window %d'%self.num)
		self.num = self.num + 1
		self.listoflists.append(w)
		
	def quit(self, *args):
		raise self

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("""Test the TextEdit interface.
		Simple window in which you can type""")
		
	def do_idle(self, *args):
		for l in self.listoflists:
			l.do_idle()

def main():
	App = TestList()
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
