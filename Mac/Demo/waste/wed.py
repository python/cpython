# A minimal text editor.
#
# To be done:
# - Functionality: find, etc.

from Menu import DrawMenuBar
from FrameWork import *
import Win
import Qd
import Res
import waste
import WASTEconst
import Scrap
import os
import macfs

UNDOLABELS = [ # Indexed by WEGetUndoInfo() value
	None, "", "typing", "Cut", "Paste", "Clear", "Drag", "Style"]
	
BIGREGION=Qd.NewRgn()
Qd.SetRectRgn(BIGREGION, -16000, -16000, 16000, 16000)

class WasteWindow(ScrolledWindow):
	def open(self, path, name, data):
		self.path = path
		self.name = name
		r = windowbounds(400, 400)
		w = Win.NewWindow(r, name, 1, 0, -1, 1, 0)
		self.wid = w
		vr = 0, 0, r[2]-r[0]-15, r[3]-r[1]-15
		dr = (0, 0, 10240, 0)
		Qd.SetPort(w)
		Qd.TextFont(4)
		Qd.TextSize(9)
		flags = WASTEconst.weDoAutoScroll | WASTEconst.weDoOutlineHilite | \
			WASTEconst.weDoMonoStyled | WASTEconst.weDoUndo
		self.ted = waste.WENew(dr, vr, flags)
		self.tedtexthandle = Res.Resource(data)
		self.ted.WEUseText(self.tedtexthandle)
		self.ted.WECalText()
		w.DrawGrowIcon()
		self.scrollbars()
		self.changed = 0
		self.do_postopen()
		self.do_activate(1, None)
		
	def do_idle(self, event):
		(what, message, when, where, modifiers) = event
		Qd.SetPort(self.wid)
		self.ted.WEIdle()	
		if self.ted.WEAdjustCursor(where, BIGREGION):
			return
		Qd.SetCursor(Qd.qd.arrow)
		
	def getscrollbarvalues(self):
		dr = self.ted.WEGetDestRect()
		vr = self.ted.WEGetViewRect()
		vx = self.scalebarvalue(dr[0], dr[2], vr[0], vr[2])
		vy = self.scalebarvalue(dr[1], dr[3], vr[1], vr[3])
##		print dr, vr, vx, vy
		return vx, vy
		
	def scrollbar_callback(self, which, what, value):
		if which == 'y':
			if what == 'set':
				height = self.ted.WEGetHeight(0, 0x3fffffff)
				cur = self.getscrollbarvalues()[1]
				delta = (cur-value)*height/32767
			if what == '-':
				topline_off,dummy = self.ted.WEGetOffset((1,1))
				topline_num = self.ted.WEOffsetToLine(topline_off)
				delta = self.ted.WEGetHeight(topline_num, topline_num+1)
			elif what == '--':
				delta = (self.ted.WEGetViewRect()[3]-10)
				if delta <= 0:
					delta = 10 # Random value
			elif what == '+':
				# XXXX Wrong: should be bottom line size
				topline_off,dummy = self.ted.WEGetOffset((1,1))
				topline_num = self.ted.WEOffsetToLine(topline_off)
				delta = -self.ted.WEGetHeight(topline_num, topline_num+1)
			elif what == '++':
				delta = -(self.ted.WEGetViewRect()[3]-10)
				if delta >= 0:
					delta = -10
			self.ted.WEScroll(0, delta)
##			print 'SCROLL Y', delta
		else:
			if what == 'set':
				return # XXXX
			vr = self.ted.WEGetViewRect()
			winwidth = vr[2]-vr[0]
			if what == '-':
				delta = winwidth/10
			elif what == '--':
				delta = winwidth/2
			elif what == '+':
				delta = -winwidth/10
			elif what == '++':
				delta = -winwidth/2
			self.ted.WEScroll(delta, 0)
		# Pin the scroll
		l, t, r, b = self.ted.WEGetDestRect()
		vl, vt, vr, vb = self.ted.WEGetViewRect()
		if t > 0 or l > 0:
			dx = dy = 0
			if t > 0: dy = -t
			if l > 0: dx = -l
##			print 'Extra scroll', dx, dy
			self.ted.WEScroll(dx, dy)
		elif b < vb:
##			print 'Extra downscroll', b-vb
			self.ted.WEScroll(0, b-vb)

		
	def do_activate(self, onoff, evt):
##		print "ACTIVATE", onoff
		Qd.SetPort(self.wid)
		ScrolledWindow.do_activate(self, onoff, evt)
		if onoff:
			self.ted.WEActivate()
			self.parent.active = self
			self.parent.updatemenubar()
		else:
			self.ted.WEDeactivate()

	def do_update(self, wid, event):
		region = wid.GetWindowPort().visRgn
		if Qd.EmptyRgn(region):
			return
		Qd.EraseRgn(region)
		self.ted.WEUpdate(region)
		self.updatescrollbars()
		
	def do_postresize(self, width, height, window):
		l, t, r, b = self.ted.WEGetViewRect()
		vr = (l, t, l+width-15, t+height-15)
		self.ted.WESetViewRect(vr)
		self.wid.InvalWindowRect(vr)
		ScrolledWindow.do_postresize(self, width, height, window)
		
	def do_contentclick(self, local, modifiers, evt):
		(what, message, when, where, modifiers) = evt
		self.ted.WEClick(local, modifiers, when)
		self.updatescrollbars()
		self.parent.updatemenubar()

	def do_char(self, ch, event):
		self.ted.WESelView()
		(what, message, when, where, modifiers) = event
		self.ted.WEKey(ord(ch), modifiers)
		self.changed = 1
		self.updatescrollbars()
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
		self.parent.updatemenubar()
		del self.ted
		del self.tedtexthandle
		self.do_postclose()
		
	def menu_save(self):
		if not self.path:
			self.menu_save_as()
			return # Will call us recursively
##		print 'Saving to ', self.path
		dhandle = self.ted.WEGetText()
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
		self.ted.WESelView()
		self.ted.WECut()
		Scrap.ZeroScrap()
		self.ted.WECut()
		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1
		
	def menu_copy(self):
		Scrap.ZeroScrap()
		self.ted.WECopy()
		self.updatescrollbars()
		self.parent.updatemenubar()
		
	def menu_paste(self):
		self.ted.WESelView()
		self.ted.WEPaste()
		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1
		
	def menu_clear(self):
		self.ted.WESelView()
		self.ted.WEDelete()
		self.updatescrollbars()
		self.parent.updatemenubar()
		self.changed = 1

	def menu_undo(self):
		self.ted.WEUndo()
		self.updatescrollbars()
		self.parent.updatemenubar()
				
	def have_selection(self):
		start, stop = self.ted.WEGetSelection()
		return start < stop
		
	def can_paste(self):
		return self.ted.WECanPaste()
		
	def can_undo(self):
		which, redo = self.ted.WEGetUndoInfo()
		which = UNDOLABELS[which]
		if which == None: return None
		if redo:
			return "Redo "+which
		else:
			return "Undo "+which

class Wed(Application):
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
		if changed:
			DrawMenuBar()

	#
	# Apple menu
	#
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message("A simple single-font text editor based on WASTE")
			
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
		w = WasteWindow(self)
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
	App = Wed()
	App.mainloop()
	
if __name__ == '__main__':
	main()
	
