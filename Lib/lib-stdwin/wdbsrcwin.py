# wdbsrcwin.py -- source window for wdb

import stdwin
from stdwinevents import *
import srcwin


class DebuggerSourceWindow(srcwin.SourceWindow):
	
	def init(self, debugger, filename):
		self.debugger = debugger
		self.curlineno = 0
		self.focus = 0
		return srcwin.SourceWindow.init(self, filename)
	
	def close(self):
		del self.debugger.sourcewindows[self.filename]
		del self.debugger
		srcwin.SourceWindow.close(self)
	
	def dispatch(self, event):
		type, win, detail = event
		if type == WE_CHAR:
			self.char(detail)
		elif type == WE_COMMAND:
			self.command(detail)
		elif type == WE_MOUSE_DOWN:
			self.mouse_down(detail)
		else:
			srcwin.SourceWindow.dispatch(self, event)
	
	def char(self, detail):
		self.debugger.char(detail)
	
	def command(self, detail):
		self.debugger.command(detail)
	
	def mouse_down(self, detail):
		(h, v), clicks, button, mask = detail
		if h >= self.leftmargin:
			srcwin.SourceWindow.dispatch(self, \
				(WE_MOUSE_DOWN, self.win, detail))
			return
		lineno = v/self.lineheight + 1
		if 1 <= lineno <= self.linecount:
			if self.debugger.get_break(self.filename, lineno):
				f = self.debugger.clear_break
			else:
				f = self.debugger.set_break
			err = f(self.filename, lineno)
			if err: stdwin.message(err)
			else: self.changemark(lineno)
		else:
			stdwin.fleep()
	
	def getmark(self, lineno):
		s = `lineno`
		if lineno == self.focus:
			s = '[' + s + ']'
		else:
			s = ' ' + s + ' '
		if lineno == self.curlineno:
			s = s + '->'
		else:
			s = s + '  '
		br = self.debugger.breaks
		if br.has_key(self.filename) and lineno in br[self.filename]:
			s = s + 'B'
		else:
			s = s + ' '
		return s
	
	def getmargin(self):
		return stdwin.textwidth('[' + `self.linecount+1` + ']->B ')
	
	def setlineno(self, newlineno):
		if newlineno != self.curlineno:
			oldlineno = self.curlineno
			self.curlineno = newlineno
			self.changemark(oldlineno)
			self.changemark(newlineno)
		if newlineno != 0:
			self.showline(newlineno)
	
	def resetlineno(self):
		self.setlineno(0)
	
	def setfocus(self, newfocus):
		if newfocus != self.focus:
			oldfocus = self.focus
			self.focus = newfocus
			self.changemark(oldfocus)
			self.changemark(newfocus)
		if newfocus != 0:
			self.showline(newfocus)
	
	def resetfocus(self):
		self.setfocus(0)

# XXX Should get rid of focus stuff again
