# Test List module.
# Draw a window with all the files in the current folder.
# double-clicking will change folder.
#
# This test expects Win, Evt and FrameWork (and anything used by those)
# to work.
#
# Actually, it is more a test of FrameWork by now....

from FrameWork import *
import Win
import Qd
import List
import os

class ListWindow(Window):
	def open(self, name, where):
		self.where = where
		r = (40, 40, 400, 300)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0x55555555)
		r2 = (0, 0, 345, 245)
		self.list = List.LNew(r2, (0, 0, 1, 1), (0,0), 0, w, 0, 1, 1, 1)
		self.filllist()
		w.DrawGrowIcon()
		self.wid = w
		self.do_postopen()
		
	def do_activate(self, onoff, evt):
		self.list.LActivate(onoff)

	def do_rawupdate(self, window, event):
		window.BeginUpdate()
		self.do_update(window, event)
		window.EndUpdate()
		
	def do_update(self, *args):
		self.list.LUpdate()
		
	def do_contentclick(self, local, modifiers, evt):
		dclick = self.list.LClick(local, modifiers)
		if dclick:
			h, v = self.list.LLastClick()
			file = self.list.LGetCell(1000, (h, v))
			self.where = os.path.join(self.where, file)
			self.filllist()

	def filllist(self):
		"""Fill the list with the contents of the current directory"""
		l = self.list
		l.LSetDrawingMode(0)
		l.LDelRow(0, 0)
		contents = os.listdir(self.where)
		l.LAddRow(len(contents), 0)
		for i in range(len(contents)):
			l.LSetCell(contents[i], (0, i))
		l.LSetDrawingMode(1)
		l.LUpdate()


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
		import macfs
		fss, ok = macfs.GetDirectory()
		if not ok:
			return
		w = ListWindow(self)
		w.open('Window %d'%self.num, fss.as_pathname())
		self.num = self.num + 1
		self.listoflists.append(w)
		
	def quit(self, *args):
		raise self

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("""Test the List Manager interface.
		Simple inward-only folder browser""")

def main():
	App = TestList()
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
