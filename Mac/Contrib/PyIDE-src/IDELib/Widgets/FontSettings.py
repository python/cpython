"""usage: 
newsettings = print FontDialog(('Chicago', 0, 12, (0, 0, 0)))	# font name or id, style flags, size, color (color is ignored)
if newsettings:
	font, style, size, color = newsettings	# 'font' is always the font name, not the id number
	# do something
"""

import W
import PyEdit
import TextEdit
import string
import types

_stylenames = ["Plain", "Bold", "Italic", "Underline", "Outline", "Shadow", "Condensed", "Extended"]


class _FontDialog:
	
	def __init__(self, fontsettings):
		leftmargin = 46
		self.w = W.ModalDialog((440, 180), 'Font settings')
		self.w.fonttitle = W.TextBox((10, 12, 30, 14), "Font:", TextEdit.teJustRight)
		self.w.pop = W.FontMenu((leftmargin, 10, 16, 16), self.setfont)
		self.w.fontname = W.TextBox((leftmargin + 20, 12, 150, 14))
		self.w.sizetitle = W.TextBox((10, 38, 30, 14), "Size:", TextEdit.teJustRight)
		self.w.sizeedit = W.EditText((leftmargin, 35, 40, 20), "", self.checksize)
		styletop = 64
		self.w.styletitle = W.TextBox((10, styletop + 2, 30, 14), "Style:", TextEdit.teJustRight)
		for i in range(len(_stylenames)):
			top = styletop + (i % 4) * 20
			left = leftmargin + 80 * (i > 3) - 2
			if i:
				self.w[i] = W.CheckBox((left, top, 76, 16), _stylenames[i], self.dostyle)
			else:
				self.w[i] = W.CheckBox((left, top, 70, 16), _stylenames[i], self.doplain)
		self.w.cancelbutton = W.Button((-180, -26, 80, 16), "Cancel", self.cancel)
		self.w.donebutton = W.Button((-90, -26, 80, 16), "Done", self.done)
		
		sampletext = "Sample text."
		self.w.sample = W.EditText((230, 10, -10, 130), sampletext, fontsettings = fontsettings)
		
		self.w.setdefaultbutton(self.w.donebutton)
		self.w.bind('cmd.', self.w.cancelbutton.push)
		self.w.bind('cmdw', self.w.donebutton.push)
		self.lastsize = fontsettings[2]
		self._rv = None
		self.set(fontsettings)
		self.w.open()
	
	def set(self, fontsettings):
		font, style, size, color = fontsettings
		if type(font) <> types.StringType:
			import Res
			res = Res.GetResource('FOND', font)
			font = res.GetResInfo()[2]
		self.w.fontname.set(font)
		self.w.sizeedit.set(str(size))
		if style:
			for i in range(1, len(_stylenames)):
				self.w[i].set(style & 0x01)
				style = style >> 1
		else:
			self.w[0].set(1)
	
	def get(self):
		font = self.w.fontname.get()
		style = 0
		if not self.w[0].get():
			flag = 0x01
			for i in range(1, len(_stylenames)):
				if self.w[i].get():
					style = style | flag
				flag = flag << 1
		size = self.lastsize
		return (font, style, size, (0, 0, 0))
	
	def doit(self):
		if self.w[0].get():
			style = 0
		else:
			style = 0
			for i in range(1, len(_stylenames)):
				if self.w[i].get():
					style = style | 2 ** (i - 1)
		#self.w.sample.set(`style`)
		self.w.sample.setfontsettings(self.get())
	
	def checksize(self):
		size = self.w.sizeedit.get()
		if not size:
			return
		try:
			size = string.atoi(size)
		except (ValueError, OverflowError):
			good = 0
		else:
			good = 1 <= size <= 500
		if good:
			if self.lastsize <> size:
				self.lastsize = size
				self.doit()
		else:
			# beep!
			self.w.sizeedit.set(`self.lastsize`)
			self.w.sizeedit.selectall()
	
	def doplain(self):
		for i in range(1, len(_stylenames)):
			self.w[i].set(0)
		self.w[0].set(1)
		self.doit()
	
	def dostyle(self):
		for i in range(1, len(_stylenames)):
			if self.w[i].get():
				self.w[0].set(0)
				break
		else:
			self.w[0].set(1)
		self.doit()
	
	def cancel(self):
		self.w.close()
	
	def done(self):
		self._rv = self.get()
		self.w.close()
	
	def setfont(self, fontname):
		self.w.fontname.set(fontname)
		self.doit()
	

def FontDialog(fontsettings):
	fd = _FontDialog(fontsettings)
	return fd._rv

def test():
	print FontDialog(('Zapata-Light', 0, 25, (0, 0, 0)))
