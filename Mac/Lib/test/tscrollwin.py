# Test FrameWork scrollbars
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

class MyWindow(ScrolledWindow):
	def open(self, name):
		r = (40, 40, 400, 300)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0x55555555)
		self.ourrect = 0, 0, 360-SCROLLBARWIDTH-1, 260-SCROLLBARWIDTH-1
		Qd.SetPort(w)
		w.DrawGrowIcon()
		self.wid = w
		self.do_postopen()
		self.vx = self.vy = 0
		self.scrollbars()
		
	def getscrollbarvalues(self):
		return self.vx, self.vy
		
	def scrollbar_callback(self, which, what, value):
		if what == '-':
			delta = -1
		elif what == '--':
			delta = -100
		elif what == '+':
			delta = 1
		elif what == '++':
			delta = 100
			
		if which == 'x':
			if value:
				self.vx = value
			else:
				self.vx = self.vx + delta
		else:
			if value:
				self.vy = value
			else:
				self.vy = self.vy + delta
		self.wid.InvalWindowRect(self.ourrect)

	def do_update(self, wid, event):
		Qd.EraseRect(self.ourrect)
		Qd.MoveTo(40, 40)
		Qd.DrawString("x=%d, y=%d"%(self.vx, self.vy))

class TestSW(Application):
	def __init__(self):
		Application.__init__(self)
		self.num = 0
		self.listoflists = []
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.newitem = MenuItem(m, "New window...", "O", self.open)
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
	
	def open(self, *args):
		w = MyWindow(self)
		w.open('Window %d'%self.num)
		self.num = self.num + 1
		self.listoflists.append(w)
		
	def quit(self, *args):
		raise self

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("""Test scrolling FrameWork windows""")

def main():
	App = TestSW()
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
