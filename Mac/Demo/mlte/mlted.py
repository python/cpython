# A minimal text editor using MLTE. Based on wed.py.
#
# To be done:
# - Functionality: find, etc.

from Menu import DrawMenuBar
from FrameWork import *
import Win
import Ctl
import Qd
import Res
import Scrap
import os
import macfs
import MacTextEditor
import Mlte

UNDOLABELS = [ # Indexed by MLTECanUndo() value
	"Typing", "Cut", "Paste", "Clear", "Font Change", "Color Change", "Size Change",
	"Style Change", "Align Left", "Align Center", "Align Right", "Drop", "Move"]
	
class MlteWindow(Window):
	def open(self, path, name, data):
		self.path = path
		self.name = name
		r = windowbounds(400, 400)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0x55555555)
		self.wid = w
		flags = MacTextEditor.kTXNDrawGrowIconMask|MacTextEditor.kTXNWantHScrollBarMask| \
				MacTextEditor.kTXNWantVScrollBarMask
		self.ted, self.frameid = Mlte.TXNNewObject(None, w, None, flags, MacTextEditor.kTXNTextEditStyleFrameType,
				MacTextEditor.kTXNTextFile, MacTextEditor.kTXNMacOSEncoding)
		self.ted.TXNSetData(MacTextEditor.kTXNTextData, data, 0, 0x7fffffff)
		self.changed = 0
		self.do_postopen()
		self.do_activate(1, None)
		
	def do_idle(self, event):
		self.ted.TXNIdle()	
		self.ted.TXNAdjustCursor(None)
		

		
	def do_activate(self, onoff, evt):
		if onoff:
##			self.ted.TXNActivate(self.frameid, 0)
			self.ted.TXNFocus(1)
			self.parent.active = self
		else:
			self.ted.TXNFocus(0)
			self.parent.active = None
		self.parent.updatemenubar()

	def do_update(self, wid, event):
		self.ted.TXNDraw(None)
		
	def do_postresize(self, width, height, window):
		self.ted.TXNResizeFrame(width, height, self.frameid)
		
	def do_contentclick(self, local, modifiers, evt):
		self.ted.TXNClick(evt)
		self.parent.updatemenubar()
		
	def do_char(self, ch, event):
		self.ted.TXNKeyDown(event)
		self.parent.updatemenubar()
		
	def close(self):
		if self.changed:
			save = EasyDialogs.AskYesNoCancel('Save window "%s" before closing?'%self.name, 1)
			if save > 0:
				self.menu_save()
			elif save < 0:
				return
		if self.parent.active == self:
			self.parent.active = None
		self.ted.TXNDeleteObject()
		del self.ted
##		del self.tedtexthandle
		self.do_postclose()
		
	def menu_save(self):
		if not self.path:
			self.menu_save_as()
			return # Will call us recursively
		dhandle = self.ted.TXNGetData(0, 0x7fffffff)
		data = dhandle.data
		fp = open(self.path, 'wb')  # NOTE: wb, because data has CR for end-of-line
		fp.write(data)
		if data[-1] <> '\r': fp.write('\r')
		fp.close()
		self.changed = 0
		
	def menu_save_as(self):
		fss, ok = macfs.StandardPutFile('Save as:')
		if not ok: return
		self.path = fss.as_pathname()
		self.name = os.path.split(self.path)[-1]
		self.wid.SetWTitle(self.name)
		self.menu_save()
		
	def menu_cut(self):
##		self.ted.WESelView()
		self.ted.TXNCut()
###		Mlte.ConvertToPublicScrap()
##		Scrap.ZeroScrap()
##		self.ted.WECut()
##		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1
		
	def menu_copy(self):
##		Scrap.ZeroScrap()
		self.ted.TXNCopy()
###		Mlte.ConvertToPublicScrap()
##		self.updatescrollbars()
		self.parent.updatemenubar()
		
	def menu_paste(self):
###		Mlte.ConvertFromPublicScrap()
		self.ted.TXNPaste()
##		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1
		
	def menu_clear(self):
##		self.ted.WESelView()
		self.ted.TXNClear()
##		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1

	def menu_undo(self):
		self.ted.TXNUndo()
##		self.updatescrollbars()
		self.parent.updatemenubar()
				
	def menu_redo(self):
		self.ted.TXNRedo()
##		self.updatescrollbars()
		self.parent.updatemenubar()
				
	def have_selection(self):
		start, stop = self.ted.TXNGetSelection()
		return start < stop
		
	def can_paste(self):
		return Mlte.TXNIsScrapPastable()
		
	def can_undo(self):
		can, which = self.ted.TXNCanUndo()
		if not can:
			return None
		if which >= len(UNDOLABELS):
			# Unspecified undo
			return "Undo"
		which = UNDOLABELS[which]
		
		return "Undo "+which

	def can_redo(self):
		can, which = self.ted.TXNCanRedo()
		if not can:
			return None
		if which >= len(UNDOLABELS):
			# Unspecified undo
			return "Redo"
		which = UNDOLABELS[which]
		
		return "Redo "+which

class Mlted(Application):
	def __init__(self):
		Application.__init__(self)
		self.num = 0
		self.active = None
		self.updatemenubar()
		
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.newitem = MenuItem(m, "New window", "N", self.open)
		self.openitem = MenuItem(m, "Open...", "O", self.openfile)
		self.closeitem = MenuItem(m, "Close", "W", self.closewin)
		m.addseparator()
		self.saveitem = MenuItem(m, "Save", "S", self.save)
		self.saveasitem = MenuItem(m, "Save as...", "", self.saveas)
		m.addseparator()
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
		
		self.editmenu = m = Menu(self.menubar, "Edit")
		self.undoitem = MenuItem(m, "Undo", "Z", self.undo)
		self.redoitem = MenuItem(m, "Redo", None, self.redo)
		m.addseparator()
		self.cutitem = MenuItem(m, "Cut", "X", self.cut)
		self.copyitem = MenuItem(m, "Copy", "C", self.copy)
		self.pasteitem = MenuItem(m, "Paste", "V", self.paste)
		self.clearitem = MenuItem(m, "Clear", "", self.clear)
		
		# Groups of items enabled together:
		self.windowgroup = [self.closeitem, self.saveitem, self.saveasitem, self.editmenu]
		self.focusgroup = [self.cutitem, self.copyitem, self.clearitem]
		self.windowgroup_on = -1
		self.focusgroup_on = -1
		self.pastegroup_on = -1
		self.undo_label = "never"
		self.redo_label = "never"
		
	def updatemenubar(self):
		changed = 0
		on = (self.active <> None)
		if on <> self.windowgroup_on:
			for m in self.windowgroup:
				m.enable(on)
			self.windowgroup_on = on
			changed = 1
		if on:
			# only if we have an edit menu
			on = self.active.have_selection()
			if on <> self.focusgroup_on:
				for m in self.focusgroup:
					m.enable(on)
				self.focusgroup_on = on
				changed = 1
			on = self.active.can_paste()
			if on <> self.pastegroup_on:
				self.pasteitem.enable(on)
				self.pastegroup_on = on
				changed = 1
			on = self.active.can_undo()
			if on <> self.undo_label:
				if on:
					self.undoitem.enable(1)
					self.undoitem.settext(on)
					self.undo_label = on
				else:
					self.undoitem.settext("Nothing to undo")
					self.undoitem.enable(0)
				changed = 1
			on = self.active.can_redo()
			if on <> self.redo_label:
				if on:
					self.redoitem.enable(1)
					self.redoitem.settext(on)
					self.redo_label = on
				else:
					self.redoitem.settext("Nothing to redo")
					self.redoitem.enable(0)
				changed = 1
		if changed:
			DrawMenuBar()

	#
	# Apple menu
	#
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message("A simple single-font text editor based on MacTextEditor")
			
	#
	# File menu
	#

	def open(self, *args):
		self._open(0)
		
	def openfile(self, *args):
		self._open(1)

	def _open(self, askfile):
		if askfile:
			fss, ok = macfs.StandardGetFile('TEXT')
			if not ok:
				return
			path = fss.as_pathname()
			name = os.path.split(path)[-1]
			try:
				fp = open(path, 'rb') # NOTE binary, we need cr as end-of-line
				data = fp.read()
				fp.close()
			except IOError, arg:
				EasyDialogs.Message("IOERROR: "+`arg`)
				return
		else:
			path = None
			name = "Untitled %d"%self.num
			data = ''
		w = MlteWindow(self)
		w.open(path, name, data)
		self.num = self.num + 1
		
	def closewin(self, *args):
		if self.active:
			self.active.close()
		else:
			EasyDialogs.Message("No active window?")
		
	def save(self, *args):
		if self.active:
			self.active.menu_save()
		else:
			EasyDialogs.Message("No active window?")
		
	def saveas(self, *args):
		if self.active:
			self.active.menu_save_as()
		else:
			EasyDialogs.Message("No active window?")
			
		
	def quit(self, *args):
		for w in self._windows.values():
			w.close()
		if self._windows:
			return
		self._quit()
		
	#
	# Edit menu
	#
	
	def undo(self, *args):
		if self.active:
			self.active.menu_undo()
		else:
			EasyDialogs.Message("No active window?")
		
	def redo(self, *args):
		if self.active:
			self.active.menu_redo()
		else:
			EasyDialogs.Message("No active window?")
		
	def cut(self, *args):
		if self.active:
			self.active.menu_cut()
		else:
			EasyDialogs.Message("No active window?")
		
	def copy(self, *args):
		if self.active:
			self.active.menu_copy()
		else:
			EasyDialogs.Message("No active window?")
		
	def paste(self, *args):
		if self.active:
			self.active.menu_paste()
		else:
			EasyDialogs.Message("No active window?")

	def clear(self, *args):
		if self.active:
			self.active.menu_clear()
		else:
			EasyDialogs.Message("No active window?")
		
	#
	# Other stuff
	#	

	def idle(self, event):
		if self.active:
			self.active.do_idle(event)

def main():
	Mlte.TXNInitTextension(0)
	try:
		App = Mlted()
		App.mainloop()
	finally:
		Mlte.TXNTerminateTextension()
	
if __name__ == '__main__':
	main()
	
