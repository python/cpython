# Text formatting abstractions
# Note -- this module is obsolete, it's too slow anyway


import string
import Para


# A formatter back-end object has one method that is called by the formatter:
# addpara(p), where p is a paragraph object.  For example:


# Formatter back-end to do nothing at all with the paragraphs
class NullBackEnd:
	#
	def __init__(self):
		pass
	#
	def addpara(self, p):
		pass
	#
	def bgn_anchor(self, id):
		pass
	#
	def end_anchor(self, id):
		pass


# Formatter back-end to collect the paragraphs in a list
class SavingBackEnd(NullBackEnd):
	#
	def __init__(self):
		self.paralist = []
	#
	def addpara(self, p):
		self.paralist.append(p)
	#
	def hitcheck(self, h, v):
		hits = []
		for p in self.paralist:
			if p.top <= v <= p.bottom:
				for id in p.hitcheck(h, v):
					if id not in hits:
						hits.append(id)
		return hits
	#
	def extract(self):
		text = ''
		for p in self.paralist:
			text = text + (p.extract())
		return text
	#
	def extractpart(self, long1, long2):
		if long1 > long2: long1, long2 = long2, long1
		para1, pos1 = long1
		para2, pos2 = long2
		text = ''
		while para1 < para2:
			ptext = self.paralist[para1].extract()
			text = text + ptext[pos1:]
			pos1 = 0
			para1 = para1 + 1
		ptext = self.paralist[para2].extract()
		return text + ptext[pos1:pos2]
	#
	def whereis(self, d, h, v):
		total = 0
		for i in range(len(self.paralist)):
			p = self.paralist[i]
			result = p.whereis(d, h, v)
			if result <> None:
				return i, result
		return None
	#
	def roundtowords(self, long1, long2):
		i, offset = long1
		text = self.paralist[i].extract()
		while offset > 0 and text[offset-1] <> ' ': offset = offset-1
		long1 = i, offset
		#
		i, offset = long2
		text = self.paralist[i].extract()
		n = len(text)
		while offset < n-1 and text[offset] <> ' ': offset = offset+1
		long2 = i, offset
		#
		return long1, long2
	#
	def roundtoparagraphs(self, long1, long2):
		long1 = long1[0], 0
		long2 = long2[0], len(self.paralist[long2[0]].extract())
		return long1, long2


# Formatter back-end to send the text directly to the drawing object
class WritingBackEnd(NullBackEnd):
	#
	def __init__(self, d, width):
		self.d = d
		self.width = width
		self.lineno = 0
	#
	def addpara(self, p):
		self.lineno = p.render(self.d, 0, self.lineno, self.width)


# A formatter receives a stream of formatting instructions and assembles
# these into a stream of paragraphs on to a back-end.  The assembly is
# parametrized by a text measurement object, which must match the output
# operations of the back-end.  The back-end is responsible for splitting
# paragraphs up in lines of a given maximum width.  (This is done because
# in a windowing environment, when the window size changes, there is no
# need to redo the assembly into paragraphs, but the splitting into lines
# must be done taking the new window size into account.)


# Formatter base class.  Initialize it with a text measurement object,
# which is used for text measurements, and a back-end object,
# which receives the completed paragraphs.  The formatting methods are:
# setfont(font)
# setleftindent(nspaces)
# setjust(type) where type is 'l', 'c', 'r', or 'lr'
# flush()
# vspace(nlines)
# needvspace(nlines)
# addword(word, nspaces)
class BaseFormatter:
	#
	def __init__(self, d, b):
		# Drawing object used for text measurements
		self.d = d
		#
		# BackEnd object receiving completed paragraphs
		self.b = b
		#
		# Parameters of the formatting model
		self.leftindent = 0
		self.just = 'l'
		self.font = None
		self.blanklines = 0
		#
		# Parameters derived from the current font
		self.space = d.textwidth(' ')
		self.line = d.lineheight()
		self.ascent = d.baseline()
		self.descent = self.line - self.ascent
		#
		# Parameter derived from the default font
		self.n_space = self.space
		#
		# Current paragraph being built
		self.para = None
		self.nospace = 1
		#
		# Font to set on the next word
		self.nextfont = None
	#
	def newpara(self):
		return Para.Para()
	#
	def setfont(self, font):
		if font == None: return
		self.font = self.nextfont = font
		d = self.d
		d.setfont(font)
		self.space = d.textwidth(' ')
		self.line = d.lineheight()
		self.ascent = d.baseline()
		self.descent = self.line - self.ascent
	#
	def setleftindent(self, nspaces):
		self.leftindent = int(self.n_space * nspaces)
		if self.para:
			hang = self.leftindent - self.para.indent_left
			if hang > 0 and self.para.getlength() <= hang:
				self.para.makehangingtag(hang)
				self.nospace = 1
			else:
				self.flush()
	#
	def setrightindent(self, nspaces):
		self.rightindent = int(self.n_space * nspaces)
		if self.para:
			self.para.indent_right = self.rightindent
			self.flush()
	#
	def setjust(self, just):
		self.just = just
		if self.para:
			self.para.just = self.just
	#
	def flush(self):
		if self.para:
			self.b.addpara(self.para)
			self.para = None
			if self.font <> None:
				self.d.setfont(self.font)
		self.nospace = 1
	#
	def vspace(self, nlines):
		self.flush()
		if nlines > 0:
			self.para = self.newpara()
			tuple = None, '', 0, 0, 0, int(nlines*self.line), 0
			self.para.words.append(tuple)
			self.flush()
			self.blanklines = self.blanklines + nlines
	#
	def needvspace(self, nlines):
		self.flush() # Just to be sure
		if nlines > self.blanklines:
			self.vspace(nlines - self.blanklines)
	#
	def addword(self, text, space):
		if self.nospace and not text:
			return
		self.nospace = 0
		self.blanklines = 0
		if not self.para:
			self.para = self.newpara()
			self.para.indent_left = self.leftindent
			self.para.just = self.just
			self.nextfont = self.font
		space = int(space * self.space)
		self.para.words.append((self.nextfont, text,
			self.d.textwidth(text), space, space,
			self.ascent, self.descent))
		self.nextfont = None
	#
	def bgn_anchor(self, id):
		if not self.para:
			self.nospace = 0
			self.addword('', 0)
		self.para.bgn_anchor(id)
	#
	def end_anchor(self, id):
		if not self.para:
			self.nospace = 0
			self.addword('', 0)
		self.para.end_anchor(id)


# Measuring object for measuring text as viewed on a tty
class NullMeasurer:
	#
	def __init__(self):
		pass
	#
	def setfont(self, font):
		pass
	#
	def textwidth(self, text):
		return len(text)
	#
	def lineheight(self):
		return 1
	#
	def baseline(self):
		return 0


# Drawing object for writing plain ASCII text to a file
class FileWriter:
	#
	def __init__(self, fp):
		self.fp = fp
		self.lineno, self.colno = 0, 0
	#
	def setfont(self, font):
		pass
	#
	def text(self, (h, v), str):
		if not str: return
		if '\n' in str:
			raise ValueError, 'can\'t write \\n'
		while self.lineno < v:
			self.fp.write('\n')
			self.colno, self.lineno = 0, self.lineno + 1
		while self.lineno > v:
			# XXX This should never happen...
			self.fp.write('\033[A') # ANSI up arrow
			self.lineno = self.lineno - 1
		if self.colno < h:
			self.fp.write(' ' * (h - self.colno))
		elif self.colno > h:
			self.fp.write('\b' * (self.colno - h))
		self.colno = h
		self.fp.write(str)
		self.colno = h + len(str)


# Formatting class to do nothing at all with the data
class NullFormatter(BaseFormatter):
	#
	def __init__(self):
		d = NullMeasurer()
		b = NullBackEnd()
		BaseFormatter.__init__(self, d, b)


# Formatting class to write directly to a file
class WritingFormatter(BaseFormatter):
	#
	def __init__(self, fp, width):
		dm = NullMeasurer()
		dw = FileWriter(fp)
		b = WritingBackEnd(dw, width)
		BaseFormatter.__init__(self, dm, b)
		self.blanklines = 1
	#
	# Suppress multiple blank lines
	def needvspace(self, nlines):
		BaseFormatter.needvspace(self, min(1, nlines))


# A "FunnyFormatter" writes ASCII text with a twist: *bold words*,
# _italic text_ and _underlined words_, and `quoted text'.
# It assumes that the fonts are 'r', 'i', 'b', 'u', 'q': (roman,
# italic, bold, underline, quote).
# Moreover, if the font is in upper case, the text is converted to
# UPPER CASE.
class FunnyFormatter(WritingFormatter):
	#
	def flush(self):
		if self.para: finalize(self.para)
		WritingFormatter.flush(self)


# Surrounds *bold words* and _italic text_ in a paragraph with
# appropriate markers, fixing the size (assuming these characters'
# width is 1).
openchar = \
    {'b':'*', 'i':'_', 'u':'_', 'q':'`', 'B':'*', 'I':'_', 'U':'_', 'Q':'`'}
closechar = \
    {'b':'*', 'i':'_', 'u':'_', 'q':'\'', 'B':'*', 'I':'_', 'U':'_', 'Q':'\''}
def finalize(para):
	oldfont = curfont = 'r'
	para.words.append(('r', '', 0, 0, 0, 0)) # temporary, deleted at end
	for i in range(len(para.words)):
		fo, te, wi = para.words[i][:3]
		if fo <> None: curfont = fo
		if curfont <> oldfont:
			if closechar.has_key(oldfont):
				c = closechar[oldfont]
				j = i-1
				while j > 0 and para.words[j][1] == '': j = j-1
				fo1, te1, wi1 = para.words[j][:3]
				te1 = te1 + c
				wi1 = wi1 + len(c)
				para.words[j] = (fo1, te1, wi1) + \
					para.words[j][3:]
			if openchar.has_key(curfont) and te:
				c = openchar[curfont]
				te = c + te
				wi = len(c) + wi
				para.words[i] = (fo, te, wi) + \
					para.words[i][3:]
			if te: oldfont = curfont
			else: oldfont = 'r'
		if curfont in string.uppercase:
			te = string.upper(te)
			para.words[i] = (fo, te, wi) + para.words[i][3:]
	del para.words[-1]


# Formatter back-end to draw the text in a window.
# This has an option to draw while the paragraphs are being added,
# to minimize the delay before the user sees anything.
# This manages the entire "document" of the window.
class StdwinBackEnd(SavingBackEnd):
	#
	def __init__(self, window, drawnow):
		self.window = window
		self.drawnow = drawnow
		self.width = window.getwinsize()[0]
		self.selection = None
		self.height = 0
		window.setorigin(0, 0)
		window.setdocsize(0, 0)
		self.d = window.begindrawing()
		SavingBackEnd.__init__(self)
	#
	def finish(self):
		self.d.close()
		self.d = None
		self.window.setdocsize(0, self.height)
	#
	def addpara(self, p):
		self.paralist.append(p)
		if self.drawnow:
			self.height = \
				p.render(self.d, 0, self.height, self.width)
		else:
			p.layout(self.width)
			p.left = 0
			p.top = self.height
			p.right = self.width
			p.bottom = self.height + p.height
			self.height = p.bottom
	#
	def resize(self):
		self.window.change((0, 0), (self.width, self.height))
		self.width = self.window.getwinsize()[0]
		self.height = 0
		for p in self.paralist:
			p.layout(self.width)
			p.left = 0
			p.top = self.height
			p.right = self.width
			p.bottom = self.height + p.height
			self.height = p.bottom
		self.window.change((0, 0), (self.width, self.height))
		self.window.setdocsize(0, self.height)
	#
	def redraw(self, area):
		d = self.window.begindrawing()
		(left, top), (right, bottom) = area
		d.erase(area)
		d.cliprect(area)
		for p in self.paralist:
			if top < p.bottom and p.top < bottom:
				v = p.render(d, p.left, p.top, p.right)
		if self.selection:
			self.invert(d, self.selection)
		d.close()
	#
	def setselection(self, new):
		if new:
			long1, long2 = new
			pos1 = long1[:3]
			pos2 = long2[:3]
			new = pos1, pos2
		if new <> self.selection:
			d = self.window.begindrawing()
			if self.selection:
				self.invert(d, self.selection)
			if new:
				self.invert(d, new)
			d.close()
			self.selection = new
	#
	def getselection(self):
		return self.selection
	#
	def extractselection(self):
		if self.selection:
			a, b = self.selection
			return self.extractpart(a, b)
		else:
			return None
	#
	def invert(self, d, region):
		long1, long2 = region
		if long1 > long2: long1, long2 = long2, long1
		para1, pos1 = long1
		para2, pos2 = long2
		while para1 < para2:
			self.paralist[para1].invert(d, pos1, None)
			pos1 = None
			para1 = para1 + 1
		self.paralist[para2].invert(d, pos1, pos2)
	#
	def search(self, prog):
		import re, string
		if type(prog) == type(''):
			prog = re.compile(string.lower(prog))
		if self.selection:
			iold = self.selection[0][0]
		else:
			iold = -1
		hit = None
		for i in range(len(self.paralist)):
			if i == iold or i < iold and hit:
				continue
			p = self.paralist[i]
			text = string.lower(p.extract())
			match = prog.search(text)
			if match:
				a, b = match.group(0)
				long1 = i, a
				long2 = i, b
				hit = long1, long2
				if i > iold:
					break
		if hit:
			self.setselection(hit)
			i = hit[0][0]
			p = self.paralist[i]
			self.window.show((p.left, p.top), (p.right, p.bottom))
			return 1
		else:
			return 0
	#
	def showanchor(self, id):
		for i in range(len(self.paralist)):
			p = self.paralist[i]
			if p.hasanchor(id):
				long1 = i, 0
				long2 = i, len(p.extract())
				hit = long1, long2
				self.setselection(hit)
				self.window.show(
					(p.left, p.top), (p.right, p.bottom))
				break


# GL extensions

class GLFontCache:
	#
	def __init__(self):
		self.reset()
		self.setfont('')
	#
	def reset(self):
		self.fontkey = None
		self.fonthandle = None
		self.fontinfo = None
		self.fontcache = {}
	#
	def close(self):
		self.reset()
	#
	def setfont(self, fontkey):
		if fontkey == '':
			fontkey = 'Times-Roman 12'
		elif ' ' not in fontkey:
			fontkey = fontkey + ' 12'
		if fontkey == self.fontkey:
			return
		if self.fontcache.has_key(fontkey):
			handle = self.fontcache[fontkey]
		else:
			import string
			i = string.index(fontkey, ' ')
			name, sizestr = fontkey[:i], fontkey[i:]
			size = eval(sizestr)
			key1 = name + ' 1'
			key = name + ' ' + `size`
			# NB key may differ from fontkey!
			if self.fontcache.has_key(key):
				handle = self.fontcache[key]
			else:
				if self.fontcache.has_key(key1):
					handle = self.fontcache[key1]
				else:
					import fm
					handle = fm.findfont(name)
					self.fontcache[key1] = handle
				handle = handle.scalefont(size)
				self.fontcache[fontkey] = \
					self.fontcache[key] = handle
		self.fontkey = fontkey
		if self.fonthandle <> handle:
			self.fonthandle = handle
			self.fontinfo = handle.getfontinfo()
			handle.setfont()


class GLMeasurer(GLFontCache):
	#
	def textwidth(self, text):
		return self.fonthandle.getstrwidth(text)
	#
	def baseline(self):
		return self.fontinfo[6] - self.fontinfo[3]
	#
	def lineheight(self):
		return self.fontinfo[6]


class GLWriter(GLFontCache):
	#
	# NOTES:
	# (1) Use gl.ortho2 to use X pixel coordinates!
	#
	def text(self, (h, v), text):
		import gl, fm
		gl.cmov2i(h, v + self.fontinfo[6] - self.fontinfo[3])
		fm.prstr(text)
	#
	def setfont(self, fontkey):
		oldhandle = self.fonthandle
		GLFontCache.setfont(fontkey)
		if self.fonthandle <> oldhandle:
			handle.setfont()


class GLMeasurerWriter(GLMeasurer, GLWriter):
	pass


class GLBackEnd(SavingBackEnd):
	#
	def __init__(self, wid):
		import gl
		gl.winset(wid)
		self.wid = wid
		self.width = gl.getsize()[1]
		self.height = 0
		self.d = GLMeasurerWriter()
		SavingBackEnd.__init__(self)
	#
	def finish(self):
		pass
	#
	def addpara(self, p):
		self.paralist.append(p)
		self.height = p.render(self.d, 0, self.height, self.width)
	#
	def redraw(self):
		import gl
		gl.winset(self.wid)
		width = gl.getsize()[1]
		if width <> self.width:
			setdocsize = 1
			self.width = width
			for p in self.paralist:
				p.top = p.bottom = None
		d = self.d
		v = 0
		for p in self.paralist:
			v = p.render(d, 0, v, width)
