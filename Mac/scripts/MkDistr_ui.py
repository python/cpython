#
# MkDistr - User Interface.
#
# Jack Jansen, CWI, August 1995
#
# XXXX To be done (requires mods of FrameWork and toolbox interfaces too):
# - Give dialogs titles (need dlg->win conversion)
# - Place dialogs better (???)
# - <return> as <ok>
# - big box around ok button
# - window-close crashes on reopen (why?)
# - Box around lists (???)
# - Change cursor while busy (need cursor support in Qd)
#
import Res
import Dlg
import Ctl
import List
import Win
import Qd
from FrameWork import *
import EasyDialogs
import macfs
import os
import sys

# Resource IDs
ID_MAIN = 514
MAIN_LIST=1
MAIN_MKDISTR=2
MAIN_CHECK=3
MAIN_INCLUDE=4
MAIN_EXCLUDE=5

ID_INCWINDOW=515
ID_EXCWINDOW=517
INCEXC_DELETE=2
INCEXC_CHANGE=3
INCEXC_ADD=4

ID_INCLUDE=512
ID_EXCLUDE=513
DLG_OK=1 # Include for include, exclude for exclude
DLG_CANCEL=2
DLG_SRCPATH=3
DLG_DSTPATH=4 # include dialog only
DLG_EXCLUDE=5 # Exclude, include dialog only

ID_DTYPE=516
DTYPE_EXIST=1
DTYPE_NEW=2
DTYPE_CANCEL=3

class EditDialogWindow(DialogWindow):
	"""Include/exclude editor (modeless dialog window)"""
	
	def open(self, id, (src, dst), callback, cancelrv):
		self.id = id
		self.callback = callback
		self.cancelrv = cancelrv
		DialogWindow.open(self, id)
		tp, h, rect = self.dlg.GetDialogItem(DLG_SRCPATH)
		Dlg.SetDialogItemText(h, src)
		self.dlg.SetDialogDefaultItem(DLG_OK)
		self.dlg.SetDialogCancelItem(DLG_CANCEL)
		if id == ID_INCLUDE:
			tp, h, rect = self.dlg.GetDialogItem(DLG_DSTPATH)
			if dst == None:
				dst = ''
			Dlg.SetDialogItemText(h, dst)
		self.dlg.DrawDialog()
	
	def do_itemhit(self, item, event):
		if item in (DLG_OK, DLG_CANCEL, DLG_EXCLUDE):
			self.done(item)
		# else it is not interesting
		
	def done(self, item):
		tp, h, rect = self.dlg.GetDialogItem(DLG_SRCPATH)
		src = Dlg.GetDialogItemText(h)
		if item == DLG_OK:
			if self.id == ID_INCLUDE:
				tp, h, rect = self.dlg.GetDialogItem(DLG_DSTPATH)
				dst = Dlg.GetDialogItemText(h)
				rv = (src, dst)
			else:
				rv = (src, None)
		elif item == DLG_EXCLUDE:
			rv = (src, None)
		else:
			rv = self.cancelrv
		self.close()
		self.callback((item in (DLG_OK, DLG_EXCLUDE)), rv)
		
class ListWindow(DialogWindow):
	"""A dialog window containing a list as its main item"""
	
	def open(self, id, contents):
		self.id = id
		DialogWindow.open(self, id)
		Qd.SetPort(self.wid)
		tp, h, rect = self.dlg.GetDialogItem(MAIN_LIST)
		self.listrect = rect
		rect2 = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-16	# Scroll bar space
		self.list = List.LNew(rect2, (0, 0, 1, len(contents)), (0,0), 0, self.wid,
				0, 1, 1, 1)
		self.setlist(contents)

	def setlist(self, contents):
		self.list.LDelRow(0, 0)
		self.list.LSetDrawingMode(0)
		if contents:
			self.list.LAddRow(len(contents), 0)
			for i in range(len(contents)):
				self.list.LSetCell(contents[i], (0, i))
		self.list.LSetDrawingMode(1)
		##self.list.LUpdate(self.wid.GetWindowPort().visRgn)
		self.wid.InvalWindowRect(self.listrect)
		
	def additem(self, item):
		where = self.list.LAddRow(1, 0)
		self.list.LSetCell(item, (0, where))
		
	def delgetitem(self, item):
		data = self.list.LGetCell(1000, (0, item))
		self.list.LDelRow(1, item)
		return data
		
	def do_listhit(self, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		where = Qd.GlobalToLocal(where)
		if self.list.LClick(where, modifiers):
			self.do_dclick(self.delgetselection())
		
	def delgetselection(self):
		items = []
		point = (0,0)
		while 1:
			ok, point = self.list.LGetSelect(1, point)
			if not ok:
				break
			items.append(point[1])
			point = point[0], point[1]+1
		values = []
		items.reverse()
		for i in items:
			values.append(self.delgetitem(i))
		return values
		
	def do_rawupdate(self, window, event):
		Qd.SetPort(window)
		Qd.FrameRect(self.listrect)
		self.list.LUpdate(self.wid.GetWindowPort().visRgn)
		
	def do_close(self):
		self.close()
		
	def close(self):
		del self.list
		DialogWindow.close(self)
		
	def mycb_add(self, ok, item):
		if item:
			self.additem(item[0])
			self.cb_add(item)
		
class MainListWindow(ListWindow):
	"""The main window"""

	def open(self, id, cb_check, cb_run, cb_add):
		ListWindow.open(self, id, [])
		self.dlg.SetDialogDefaultItem(MAIN_INCLUDE)
		self.cb_run = cb_run
		self.cb_check = cb_check
		self.cb_add = cb_add
		setwatchcursor()
		list = self.cb_check()
		self.setlist(list)
		setarrowcursor()

	def do_itemhit(self, item, event):
		if item == MAIN_LIST:
			self.do_listhit(event)
		if item == MAIN_MKDISTR:
			setwatchcursor()
			self.cb_run()
			setarrowcursor()
		if item == MAIN_CHECK:
			setwatchcursor()
			list = self.cb_check()
			self.setlist(list)
			setarrowcursor()
		if item == MAIN_INCLUDE:
			self.do_dclick(self.delgetselection())
		if item == MAIN_EXCLUDE:
			for i in self.delgetselection():
				self.cb_add((i, None))
			
	def do_dclick(self, list):
		if not list:
			list = ['']
		for l in list:
			w = EditDialogWindow(self.parent)
			w.open(ID_INCLUDE, (l, None), self.mycb_add, None)

	def mycb_add(self, ok, item):
		if item:
			self.cb_add(item)

class IncListWindow(ListWindow):
	"""An include/exclude window"""
	def open(self, id, editid, contents, cb_add, cb_del, cb_get):
		ListWindow.open(self, id, contents)
		self.dlg.SetDialogDefaultItem(INCEXC_CHANGE)
		self.editid = editid
		self.cb_add = cb_add
		self.cb_del = cb_del
		self.cb_get = cb_get

	def do_itemhit(self, item, event):
		if item == MAIN_LIST:
			self.do_listhit(event)
		if item == INCEXC_DELETE:
			old = self.delgetselection()
			for i in old:
				self.cb_del(i)
		if item == INCEXC_CHANGE:
			self.do_dclick(self.delgetselection())
		if item == INCEXC_ADD:
			w = EditDialogWindow(self.parent)
			w.open(self.editid, ('', None), self.mycb_add, None)
			
	def do_dclick(self, list):
		if not list:
			list = ['']
		for l in list:
			old = self.cb_get(l)
			self.cb_del(l)
			w = EditDialogWindow(self.parent)
			w.open(self.editid, old, self.mycb_add, old)

class MkDistrUI(Application):
	def __init__(self, main):
		self.main = main
		Application.__init__(self)
		self.mwin = MainListWindow(self)
		self.mwin.open(ID_MAIN, self.main.check, self.main.run, self.main.inc.add)
		self.iwin = None
		self.ewin = None	
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.includeitem = MenuItem(m, "Show Include window", "", self.showinc)
		self.excludeitem = MenuItem(m, "Show Exclude window", "", self.showexc)
		self.saveitem = MenuItem(m, "Save databases", "S", self.save)
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
		
	def quit(self, *args):
		if self.main.is_modified():
			rv = EasyDialogs.AskYesNoCancel('Database modified. Save?', -1)
			if rv == -1:
				return
			if rv == 1:
				self.main.save()
		self._quit()
		
	def save(self, *args):
		self.main.save()
		
	def showinc(self, *args):
		if self.iwin:
			if self._windows.has_key(self.iwin):
				self.iwin.close()
			del self.iwin
		self.iwin = IncListWindow(self)
		self.iwin.open(ID_INCWINDOW, ID_INCLUDE, self.main.inc.getall(), self.main.inc.add,
			self.main.inc.delete, self.main.inc.get)
		
	def showexc(self, *args):
		if self.ewin:
			if self._windows.has_key(self.ewin):
				self.ewin.close()
			del self.ewin
		self.ewin = IncListWindow(self)
		self.ewin.open(ID_EXCWINDOW, ID_EXCLUDE, self.main.exc.getall(), self.main.exc.add,
			self.main.exc.delete, self.main.exc.get)

	def do_about(self, id, item, window, event):
		EasyDialogs.Message("Test the MkDistr user interface.")
		
def GetType():
	"""Ask user for distribution type"""
	while 1:
		d = Dlg.GetNewDialog(ID_DTYPE, -1)
		d.SetDialogDefaultItem(DTYPE_EXIST)
		d.SetDialogCancelItem(DTYPE_CANCEL)
		while 1:
			rv = ModalDialog(None)
			if rv in (DTYPE_EXIST, DTYPE_NEW, DTYPE_CANCEL):
				break
		del d
		if rv == DTYPE_CANCEL:
			sys.exit(0)
		if rv == DTYPE_EXIST:
##			macfs.SetFolder(':(MkDistr)')
			fss, ok = macfs.StandardGetFile('TEXT')
			if not ok:
				sys.exit(0)
			path = fss.as_pathname()
			basename = os.path.split(path)[-1]
			if basename[-8:] <> '.include':
				EasyDialogs.Message('That is not a distribution include file')
			else:
				return basename[:-8]
		else:
			name = EasyDialogs.AskString('Distribution name:')
			if name:
				return name
			sys.exit(0)
			
def InitUI():
	"""Initialize stuff needed by UI (a resource file)"""
	Res.FSpOpenResFile('MkDistr.rsrc', 1)

class _testerhelp:
	def __init__(self, which):
		self.which = which
		
	def get(self):
		return [self.which+'-one', self.which+'-two']
		
	def add(self, value):
		if value:
			print 'ADD', self.which, value
			
	def delete(self, value):
		print 'DEL', self.which, value
		
class _test:
	def __init__(self):
		import sys
		Res.FSpOpenResFile('MkDistr.rsrc', 1)
		self.inc = _testerhelp('include')
		self.exc = _testerhelp('exclude')
		self.ui = MkDistrUI(self)
		self.ui.mainloop()
		sys.exit(1)
		
	def check(self):
		print 'CHECK'
		return ['rv1', 'rv2']
		
	def run(self):
		print 'RUN'
		
if __name__ == '__main__':
	_test()
