# Test waste module.
# Draw a window in which the user can type.
#
# This test expects Win, Evt and FrameWork (and anything used by those)
# to work.
#
# Actually, it is more a test of FrameWork by now....

from FrameWork import *
import Win
import Qd
import waste
import WASTEconst
import os

class WasteWindow(Window):
	def open(self, name):
		r = (40, 40, 400, 300)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0x55555555)
		r2 = (0, 0, 400-40-16, 300-40-16)
		Qd.SetPort(w)
		flags = WASTEconst.weDoAutoScroll | WASTEconst.weDoOutlineHilite | \
			WASTEconst.weDoMonoStyled 
		self.ted = waste.WENew(r2, r2, flags)
		w.DrawGrowIcon()
		self.wid = w
		self.do_postopen()
		
	def do_idle(self):
		self.ted.WEIdle()
		
	def do_activate(self, onoff, evt):
		if onoff:
			self.ted.WEActivate()
		else:
			self.ted.WEDeactivate()

	def do_update(self, wid, event):
		Qd.EraseRect(wid.GetWindowPort().portRect)
		self.ted.WEUpdate(wid.GetWindowPort().visRgn)
		
	def do_contentclick(self, local, modifiers, evt):
		(what, message, when, where, modifiers) = evt
		self.ted.WEClick(local, modifiers, when)

	def do_char(self, ch, event):
		(what, message, when, where, modifiers) = event
		self.ted.WEKey(ord(ch), modifiers)

class TestWaste(Application):
	def __init__(self):
		Application.__init__(self)
		self.num = 0
		self.listoflists = []
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.newitem = MenuItem(m, "New window...", "O", self.open)
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
	
	def open(self, *args):
		w = WasteWindow(self)
		w.open('Window %d'%self.num)
		self.num = self.num + 1
		self.listoflists.append(w)
		
	def quit(self, *args):
		raise self

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("""Test the WASTE interface.
		Simple window in which you can type""")
		
	def do_idle(self, *args):
		for l in self.listoflists:
			l.do_idle()

def main():
	print 'Open app'
	App = TestWaste()
	print 'run'
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
