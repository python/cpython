"""browsepict - Display all "PICT" resources found"""

import FrameWork
import EasyDialogs
import Res
import Qd
import Win
import List
import sys
import struct

#
# Resource definitions
ID_MAIN=512
MAIN_LIST=1
MAIN_SHOW=2

# Where is the picture window?
LEFT=200
TOP=64

def main():
	try:
		dummy = Res.GetResource('DLOG', ID_MAIN)
	except Res.Error:
		try:
			Res.FSpOpenResFile("oldPICTbrowse.rsrc", 0)
		except Res.Error, arg:
			EasyDialogs.Message("Cannot open PICTbrowse.rsrc: "+arg[1])
			sys.exit(1)	
	PICTbrowse()

class PICTbrowse(FrameWork.Application):
	def __init__(self):
		# First init menus, etc.
		FrameWork.Application.__init__(self)
		# Next create our dialog
		self.main_dialog = MyDialog(self)
		# Now open the dialog
		contents = self.findPICTresources()
		self.main_dialog.open(ID_MAIN, contents)
		# Finally, go into the event loop
		self.mainloop()
	
	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "File")
		self.quititem = FrameWork.MenuItem(m, "Quit", "Q", self.quit)
	
	def quit(self, *args):
		self._quit()
		
	def showPICT(self, resid):
		w = PICTwindow(self)
		w.open(resid)
		#EasyDialogs.Message('Show PICT '+`resid`)
		
	def findPICTresources(self):
		num = Res.CountResources('PICT')
		rv = []
		for i in range(1, num+1):
			Res.SetResLoad(0)
			try:
				r = Res.GetIndResource('PICT', i)
			finally:
				Res.SetResLoad(1)
			id, type, name = r.GetResInfo()
			rv.append((id, name))
		return rv
		
class PICTwindow(FrameWork.Window):
	def open(self, (resid, resname)):
		if not resname:
			resname = '#'+`resid`
		self.resid = resid
		picture = Qd.GetPicture(self.resid)
		# Get rect for picture
		print `picture.data[:16]`
		sz, t, l, b, r = struct.unpack('hhhhh', picture.data[:10])
		print 'pict:', t, l, b, r
		width = r-l
		height = b-t
		if width < 64: width = 64
		elif width > 480: width = 480
		if height < 64: height = 64
		elif height > 320: height = 320
		bounds = (LEFT, TOP, LEFT+width, TOP+height)
		print 'bounds:', bounds
		
		self.wid = Win.NewWindow(bounds, resname, 1, 0, -1, 1, 0)
		self.wid.SetWindowPic(picture)
		self.do_postopen()
		
class MyDialog(FrameWork.DialogWindow):
	"Main dialog window for PICTbrowse"

	def open(self, id, contents):
		self.id = id
		FrameWork.DialogWindow.open(self, ID_MAIN)
		self.dlg.SetDialogDefaultItem(MAIN_SHOW)
		tp, h, rect = self.dlg.GetDialogItem(MAIN_LIST)
		rect2 = rect[0]+1, rect[1]+1, rect[2]-17, rect[3]-17	# Scroll bar space
		self.list = List.LNew(rect2, (0, 0, 1, len(contents)), (0,0), 0, self.wid,
				0, 1, 1, 1)
		self.contents = contents
		self.setlist()

	def setlist(self):
		self.list.LDelRow(0, 0)
		self.list.LSetDrawingMode(0)
		if self.contents:
			self.list.LAddRow(len(self.contents), 0)
			for i in range(len(self.contents)):
				v = `self.contents[i][0]`
				if self.contents[i][1]:
					v = v + '"' + self.contents[i][1] + '"'
				self.list.LSetCell(v, (0, i))
		self.list.LSetDrawingMode(1)
		self.list.LUpdate(self.wid.GetWindowPort().visRgn)
		
	def do_listhit(self, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		where = Qd.GlobalToLocal(where)
		print 'LISTHIT', where
		if self.list.LClick(where, modifiers):
			self.do_show()
		
	def getselection(self):
		items = []
		point = (0,0)
		while 1:
			ok, point = self.list.LGetSelect(1, point)
			if not ok:
				break
			items.append(point[1])
			point = point[0], point[1]+1
		values = []
		for i in items:
			values.append(self.contents[i])
		return values
		
	def do_show(self, *args):
		selection = self.getselection()
		for resid in selection:
			self.parent.showPICT(resid)
		
	def do_rawupdate(self, window, event):
		tp, h, rect = self.dlg.GetDialogItem(MAIN_LIST)
		Qd.SetPort(self.wid)
		Qd.FrameRect(rect)
		self.list.LUpdate(self.wid.GetWindowPort().visRgn)
		
	def do_activate(self, activate, event):
		self.list.LActivate(activate)
		
	def do_close(self):
		self.close()
		
	def do_itemhit(self, item, event):
		if item == MAIN_LIST:
			self.do_listhit(event)
		if item == MAIN_SHOW:
			self.do_show()

main()
