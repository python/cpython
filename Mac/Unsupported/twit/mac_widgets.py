from FrameWork import *
import Win
import Qd
import Controls
import Ctl
import TE
import List
import os
import string
import macfs

SCROLLBAR=16
MARGIN=2
ICONSIZE=16
TEXTWIDTH=4096 # More-or-less random value

TEXTFONT=4
TEXTSIZE=9

# Resource numbers
PIC_CURRENT=500
PIC_BREAK=501

picture_cache={}

class MT_TextWidget:
	def __init__(self, wid, r):
		self.wid = wid
		self.rect = r
		left, top, right, bottom = r
		self.terect = left+MARGIN+ICONSIZE, top+MARGIN, \
				right-(MARGIN+SCROLLBAR), bottom-(MARGIN+SCROLLBAR)
		dr = self.terect[0], self.terect[1], TEXTWIDTH, self.terect[3]
		Qd.SetPort(wid)
		Qd.TextFont(TEXTFONT)
		Qd.TextSize(TEXTSIZE)
		self.ted = TE.TENew(dr, self.terect)
		self.ted.TEAutoView(1)
		self.activate(1)
		
		rect = right-SCROLLBAR, top, right, bottom-SCROLLBAR+1
		self.bary = Ctl.NewControl(self.wid, rect, "", 1, 0, 0, 0, 16, 0)
		rect = left, bottom-SCROLLBAR, right-SCROLLBAR+1, bottom
		self.barx = Ctl.NewControl(self.wid, rect, "", 1, 0, 0, 0, 16, 0)
		
		self.have_data = 0
		self.line_index = []
		
	def close(self):
		del self.barx
		del self.bary
		del self.ted
		
	def scrollbars(self):
		pass
		
	def setcontent(self, file):
		self.line_index = []
		if file == None:
			data = ''
			self.have_data = 0
		else:
			try:
				fp = open(file, 'rb') # NOTE the binary
				data = fp.read()
				self.have_data = 1
			except IOError, arg:
				data = 'Cannot open file:\r'+`arg`
				self.have_data = 0
		if len(data) > 32767:
			self.have_data = 0
			data = 'File too big'
		self.ted.TESetText(data)
		if self.have_data:
			cur = 0
			while 1:
				self.line_index.append(cur)
				try:
					cur = string.index(data, '\r', cur+1)
				except ValueError:
					break
			self.line_index.append(len(data))
		self.wid.InvalWindowRect(self.rect)
		self.ted.TESetSelect(0,0)
		self.ted.TECalText()
		self.ted.TESelView()
		self.setscrollbars()
		
	def setscrollbars(self):
		docleft, doctop, docright, docbot = self.ted.destRect
		winleft, wintop, winright, winbot = self.ted.viewRect
		docbot = self.ted.nLines*self.ted.lineHeight + doctop
		self.setbar(self.barx, docleft, docright, winleft, winright)
		self.setbar(self.bary, doctop, docbot, wintop, winbot)
		
	def setbar(self, bar, minmin, maxmax, curmin, curmax):
		if maxmax-minmin > 32767 or (curmin <= minmin and curmax >= maxmax):
			bar.SetControlMinimum(0)
			bar.SetControlMaximum(0)
			bar.SetControlValue(0)
			return
		bar.SetControlMinimum(minmin)
		bar.SetControlMaximum(maxmax-(curmax-curmin))
		bar.SetControlValue(curmin)

	def update(self, rgn):
		Qd.EraseRect(self.terect)
		Qd.FrameRect(self.rect)
		self.ted.TEUpdate(self.terect)
		
	def activate(self, onoff):
		if onoff:
			self.ted.TEActivate()
		else:
			self.ted.TEDeactivate()

	def select(self, line):
		if line == None or line <= 0 or not self.have_data:
			self.ted.TESetSelect(0,0)
		else:
			line = line - 1
			if line > len(self.line_index)-1: line = len(self.line_index)-1
			if line == 1:
				self.ted.TESetSelect(0, self.line_index[1])
			else:
				self.ted.TESetSelect(self.line_index[line]+1, self.line_index[line+1])
		self.setscrollbars()
		
	def click(self, where, modifiers):
		# First check scrollbars
		ctltype, control = Ctl.FindControl(where, self.wid)
		if ctltype and control:
			partcode = control.TrackControl(where)
			if partcode:
				self.controlhit(control, partcode)
			return None, 0
		off = self.ted.TEGetOffset(where)
		inborder = where[0] < self.terect[0]
		l, t, r, b = self.terect
		if l <= where[0] <= r and t <= where[1] <= b or inborder:
			return self.offsettoline(off), inborder
		return None, 0	# In the grow box or something.
		
	def offsettoline(self, offset):
		for i in range(len(self.line_index)):
			if offset < self.line_index[i]:
				return i   # Not i-1: 1-based line numbers in files
		return None

	def controlhit(self, control, partcode):
		if partcode <> Controls.inThumb:
			if control == self.barx:
				if partcode == Controls.inUpButton:
					delta = -10
				if partcode == Controls.inDownButton:
					delta = 10
				if partcode == Controls.inPageUp:
					delta = 10-(self.terect[2]-self.terect[0])
				if partcode == Controls.inPageDown:
					delta = (self.terect[2]-self.terect[0])-10
				old = control.GetControlValue()
				control.SetControlValue(old+delta)
			if control == self.bary:
				if partcode == Controls.inUpButton:
					delta = -self.ted.lineHeight
				if partcode == Controls.inDownButton:
					delta = self.ted.lineHeight
				if partcode == Controls.inPageUp:
					delta = self.ted.lineHeight-(self.terect[3]-self.terect[1])
				if partcode == Controls.inPageDown:
					delta = (self.terect[3]-self.terect[1])-self.ted.lineHeight
				old = control.GetControlValue()
				control.SetControlValue(old+delta)
		newx = self.barx.GetControlValue()
		newy = self.bary.GetControlValue()
		oldx = self.ted.viewRect[0]
		oldy = self.ted.viewRect[1]
		self.ted.TEPinScroll(oldx-newx, oldy-newy)
		self.setscrollbars() # XXXX Bibbert, maar hoe anders?
			
class MT_IconTextWidget(MT_TextWidget):
	def __init__(self, wid, r):
		MT_TextWidget.__init__(self, wid, r)
		self.breakpointlist = []
		self.curline = None
		self.iconrect = (self.rect[0]+1, self.rect[1]+1, 
				self.terect[0]-1, self.rect[3]-SCROLLBAR)
		self.curlinerange = (self.terect[1]+self.ted.lineHeight,
				self.terect[3]-2*self.ted.lineHeight)
		self.piccurrent = PIC_CURRENT
		
	def setbreaks(self, list):
		self.breakpointlist = list[:]
		Qd.SetPort(self.wid)
		self.wid.InvalWindowRect(self.iconrect)
		
	def setcurline(self, line, pic=PIC_CURRENT):
		self.curline = line
		self.piccurrent = pic
		Qd.SetPort(self.wid)
		self.showline(line)

	def showline(self, line):
		if line <= 0: line = 1
		if line >= len(self.line_index): line = len(self.line_index)-1
		if line < 0: return
		off = self.line_index[line]
		x, y = self.ted.TEGetPoint(off)
		if self.curlinerange[0] <= y <= self.curlinerange[1]:
			return # It is in view
		middle = (self.curlinerange[0]+self.curlinerange[1])/2
		self.ted.TEPinScroll(0, middle-y) # Of andersom?
		self.setscrollbars()
		
	def setscrollbars(self):
		MT_TextWidget.setscrollbars(self)
		self.wid.InvalWindowRect(self.iconrect)
				
	def update(self, rgn):
		MT_TextWidget.update(self, rgn)
		self.drawallicons()
		
	def drawallicons(self):
		Qd.EraseRect(self.iconrect)
		Qd.MoveTo(self.iconrect[2], self.iconrect[1])
		Qd.LineTo(self.iconrect[2], self.iconrect[3])
		topoffset = self.ted.TEGetOffset((self.terect[0], self.terect[1]))
		botoffset = self.ted.TEGetOffset((self.terect[0], self.terect[3]))
		topline = self.offsettoline(topoffset)
		botline = self.offsettoline(botoffset)
		if topline == None: topline = 1 # ???
		if botline == None: botline = len(self.line_index)
		for i in self.breakpointlist:
			if topline <= i <= botline:
				self.draw1icon(i, PIC_BREAK)
		if self.curline <> None and topline <= self.curline <= botline:
			self.draw1icon(self.curline, self.piccurrent)
			
	def draw1icon(self, line, which):
		offset = self.line_index[line]
		botx, boty = self.ted.TEGetPoint(offset)
		rect = self.rect[0]+2, boty-self.ted.lineHeight, \
			self.rect[0]+ICONSIZE-2, boty
		if not picture_cache.has_key(which):
			picture_cache[which] = Qd.GetPicture(which)
		self.drawicon(rect, picture_cache[which])
		
	def drawicon(self, rect, which):
		Qd.DrawPicture(which, rect)

class MT_IndexList:
	def __init__(self, wid, rect, width):
		# wid is the window (dialog) where our list is going to be in
		# rect is it's item rectangle (as in dialog item)
		self.rect = rect
		rect2 = rect[0]+1, rect[1]+1, rect[2]-16, rect[3]-1
		self.list = List.LNew(rect2, (0, 0, width, 0), (0,0), 0, wid,
					0, 1, 0, 1)
		self.wid = wid
		self.width = width
	
	def setcontent(self, *content):
		self.list.LDelRow(0, 1)
		self.list.LSetDrawingMode(0)
		self.list.LAddRow(len(content[0]), 0)
		for x in range(len(content)):
			column = content[x]
			for y in range(len(column)):
				self.list.LSetCell(column[y], (x, y))
		self.list.LSetDrawingMode(1)
		self.wid.InvalWindowRect(self.rect)

	def deselectall(self):
		while 1:
			ok, pt = self.list.LGetSelect(1, (0,0))
			if not ok: return
			self.list.LSetSelect(0, pt)
			
	def select(self, num):
		self.deselectall()
		if num < 0:
			return
		for i in range(self.width):
			self.list.LSetSelect(1, (i, num))
			
	def click(self, where, modifiers):
		is_double = self.list.LClick(where, modifiers)
		ok, (x, y) = self.list.LGetSelect(1, (0, 0))
		if ok:
			return y, is_double
		else:
			return None, is_double
			
	# draw a frame around the list, List Manager doesn't do that
	def drawframe(self):
		Qd.SetPort(self.wid)
		Qd.FrameRect(self.rect)
		
	def update(self, rgn):
		self.drawframe()
		self.list.LUpdate(rgn)
		
	def activate(self, onoff):
		self.list.LActivate(onoff)
		
class MT_AnyList(MT_IndexList):

	def click(self, where, modifiers):
		is_double = self.list.LClick(where, modifiers)
		ok, (x, y) = self.list.LGetSelect(1, (0, 0))
		if ok:
			self.select(y)
			field0 = self.list.LGetCell(1000,(0,y))
		else:
			field0 = None
		return field0, is_double
	
