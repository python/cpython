# Fill-out form window

import stdwin
from stdwinevents import *


class Form:

	def __init__(self, title):
		self.title = title
		self.window = None
		self.fields = {}
		self.fieldnames = []
		self.formwidth = self.formheight = 0
		self.focusname = None
		self.tefocus = None

	def define_field(self, name, label, lines, chars):
		self.fieldnames.append(name)
		lh = stdwin.lineheight()
		cw = stdwin.textwidth('m')
		left = 20*cw
		top = self.formheight + 4
		right = left + chars*cw
		bottom = top + lines*lh
		te = None
		self.fields[name] = (label, left, top, right, bottom, te)
		self.formheight = bottom + 2
		self.formwidth = max(self.formwidth, right + 4)

	def open(self):
		if self.window: return
		self.formwidth = max(100, self.formwidth)
		self.formheight = max(50, self.formheight)
		stdwin.setdefwinsize(self.formwidth, self.formheight)
		stdwin.setdefscrollbars(0, 0)
		self.window = stdwin.open(self.title)
		self.window.setdocsize(self.formwidth, self.formheight)
		for name in self.fieldnames:
			label, left, top, right, bottom, te = \
				  self.fields[name]
			rect = (left, top), (right, bottom)
			te = self.window.textcreate(rect)
			te.setactive(0)
			te.setview(rect)
			self.fields[name] = \
				  label, left, top, right, bottom, te
		if self.fieldnames:
			self.setfocus(self.fieldnames[0])

	def setfocus(self, name):
		if name <> self.focusname and self.tefocus:
			self.tefocus.setactive(0)
		self.focusname = name
		if self.focusname:
			self.tefocus = self.fields[self.focusname][-1]
			self.tefocus.setactive(1)
		else:
			self.tefocus = None

	def dispatch(self, type, detail):
		event = type, self.window, detail
		if type == WE_NULL:
			pass
		elif type == WE_DRAW:
			self.draw(detail)
		elif type == WE_MOUSE_DOWN:
			x, y = detail[0]
			for name in self.fieldnames:
				label, left, top, right, bottom, te = \
					  self.fields[name]
				if left <= x < right and \
					  top <= y < bottom:
					self.setfocus(name)
					break
			else:
				stdwin.fleep()
				return
			if self.tefocus:
				(left, top), (right, bottom) = \
					  self.tefocus.getrect()
				if x < left: x = left
				if x >= right: x = right-1
				if y < top: y = top
				if y >= bottom:
					y = bottom-1
					x = right-1
				event = type, self.window, ((x,y),)+detail[1:]
				if not self.tefocus.event(event):
					stdwin.fleep()
		elif type in (WE_MOUSE_MOVE, WE_MOUSE_UP, WE_CHAR):
			if not self.tefocus or not self.tefocus.event(event):
				stdwin.fleep()
			elif type == WE_MOUSE_UP:
				button = detail[2]
				if button == 2:
					self.paste_selection()
				else:
					self.make_selection()
		elif type == WE_COMMAND:
			if detail in (WC_BACKSPACE, WC_UP, WC_DOWN,
				      WC_LEFT, WC_RIGHT):
				if not self.tefocus or \
					  not self.tefocus.event(event):
					stdwin.fleep()
			elif detail == WC_RETURN:
				print '*** Submit query'
			elif detail == WC_TAB:
				if not self.fields:
					stdwin.fleep()
					return
				if not self.focusname:
					i = 0
				else:
					i = self.fieldnames.index(
						  self.focusname)
					i = (i+1) % len(self.fieldnames)
				self.setfocus(self.fieldnames[i])
				self.tefocus.setfocus(0, 0x7fff)
				self.make_selection()
		elif type in (WE_ACTIVATE, WE_DEACTIVATE):
			pass
		elif type == WE_LOST_SEL:
			if self.tefocus:
				a, b = self.tefocus.getfocus()
				self.tefocus.setfocus(a, a)
		else:
			print 'Form.dispatch(%d, %s)' % (type, `detail`)

	def draw(self, detail):
		d = self.window.begindrawing()
		d.cliprect(detail)
		d.erase(detail)
		self.drawform(d, detail)
		d.noclip()
		d.close()
		# Stupid textedit objects can't draw with open draw object...
		self.drawtextedit(detail)

	def drawform(self, d, detail):
		for name in self.fieldnames:
			label, left, top, right, bottom, te = self.fields[name]
			d.text((0, top), label)
			d.box((left-3, top-2), (right+4, bottom+2))

	def drawtextedit(self, detail):
		for name in self.fieldnames:
			label, left, top, right, bottom, te = self.fields[name]
			te.draw(detail)

	def make_selection(self):
		s = self.tefocus.getfocustext()
		if not s:
			return
		stdwin.rotatecutbuffers(1)
		stdwin.setcutbuffer(0, s)
		if not self.window.setselection(WS_PRIMARY, s):
			stdwin.fleep()

	def paste_selection(self):
		if not self.tefocus:
			stdwin.fleep()
			return
		s = stdwin.getselection(WS_PRIMARY)
		if not s:
			s = stdwin.getcutbuffer(0)
			if not s:
				stdwin.fleep()
				return
		self.tefocus.replace(s)
