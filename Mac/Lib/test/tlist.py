# Test List module.
# Draw a window with all the files in the current folder.
# double-clicking will change folder.
#
# This test expects Win, Evt and FrameWork (and anything used by those)
# to work.

from FrameWork import *
import Win
import Qd
import List
import os

class TestList(Application):
	def __init__(self):
		os.chdir('Moes:')
		self.makemenubar()
		self.makewindow()
	
	def makewindow(self):
		r = (40, 40, 400, 300)
		w = Win.NewWindow(r, "List test", 1, 0, -1, 1, 0x55555555)
		r2 = (0, 0, 345, 245)
		self.list = List.LNew(r2, (0, 0, 1, 1), (0,0), 0, w, 0, 1, 1, 1)
		self.filllist()
		w.DrawGrowIcon()
		self.win = w
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
	
	def quit(self, *args):
		raise self

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("""Test the List Manager interface.
		Double-click on a folder to change directory""")
		
	def do_activateEvt(self, *args):
		self.list.LActivate(1)	# XXXX Wrong...
		
	def do_update(self, *args):
		print 'LUPDATE'
		self.list.LUpdate()

	def do_inContent(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(window)
		local = Qd.GlobalToLocal(where)
		print 'CLICK', where, '->', local
		dclick = self.list.LClick(local, modifiers)
		if dclick:
			h, v = self.list.LLastClick()
			file = self.list.LGetCell(1000, (h, v))
			os.chdir(file)
			self.filllist()

	def filllist(self):
		"""Fill the list with the contents of the current directory"""
		l = self.list
		l.LSetDrawingMode(0)
		l.LDelRow(0, 0)
		contents = os.listdir(':')
		print contents
		l.LAddRow(len(contents), 0)
		for i in range(len(contents)):
			l.LSetCell(contents[i], (0, i))
		l.LSetDrawingMode(1)
		l.LUpdate()


def main():
	App = TestList()
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
