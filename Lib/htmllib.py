# A parser for HTML documents


# HTML: HyperText Markup Language; an SGML-like syntax used by WWW to
# describe hypertext documents
#
# SGML: Standard Generalized Markup Language
#
# WWW: World-Wide Web; a distributed hypertext system develped at CERN
#
# CERN: European Particle Physics Laboratory in Geneva, Switzerland


# This file is only concerned with parsing and formatting HTML
# documents, not with the other (hypertext and networking) aspects of
# the WWW project.  (It does support highlighting of anchors.)


import os
import sys
import regex
import string
import sgmllib


class HTMLParser(sgmllib.SGMLParser):

	# Copy base class entities and add some
	entitydefs = {}
	for key in sgmllib.SGMLParser.entitydefs.keys():
		entitydefs[key] = sgmllib.SGMLParser.entitydefs[key]
	entitydefs['bullet'] = '*'

	# Provided -- handlers for tags introducing literal text
	
	def start_listing(self, attrs):
		self.setliteral('listing')
		self.literal_bgn('listing', attrs)

	def end_listing(self):
		self.literal_end('listing')

	def start_xmp(self, attrs):
		self.setliteral('xmp')
		self.literal_bgn('xmp', attrs)

	def end_xmp(self):
		self.literal_end('xmp')

	def do_plaintext(self, attrs):
		self.setnomoretags()
		self.literal_bgn('plaintext', attrs)

	# To be overridden -- begin/end literal mode
	def literal_bgn(self, tag, attrs): pass
	def literal_end(self, tag): pass


# Next level of sophistication -- collect anchors, title, nextid and isindex
class CollectingParser(HTMLParser):
	#
	def __init__(self):
		HTMLParser.__init__(self)
		self.savetext = None
		self.nextid = []
		self.isindex = 0
		self.title = ''
		self.inanchor = 0
		self.anchors = []
		self.anchornames = []
		self.anchortypes = []
	#
	def start_a(self, attrs):
		self.inanchor = 0
		href = ''
		name = ''
		type = ''
		for attrname, value in attrs:
			if attrname == 'href':
				href = value
			if attrname == 'name=':
				name = value
			if attrname == 'type=':
				type = string.lower(value)
		if not (href or name):
			return
		self.anchors.append(href)
		self.anchornames.append(name)
		self.anchortypes.append(type)
		self.inanchor = len(self.anchors)
		if not href:
			self.inanchor = -self.inanchor
	#
	def end_a(self):
		if self.inanchor > 0:
			# Don't show anchors pointing into the current document
			if self.anchors[self.inanchor-1][:1] <> '#':
				self.handle_data('[' + `self.inanchor` + ']')
		self.inanchor = 0
	#
	def start_html(self, attrs): pass
	def end_html(self): pass
	#
	def start_head(self, attrs): pass
	def end_head(self): pass
	#
	def start_body(self, attrs): pass
	def end_body(self): pass
	#
	def do_nextid(self, attrs):
		self.nextid = attrs
	#
	def do_isindex(self, attrs):
		self.isindex = 1
	#
	def start_title(self, attrs):
		self.savetext = ''
	#
	def end_title(self):
		if self.savetext <> None:
			self.title = self.savetext
			self.savetext = None
	#
	def handle_data(self, text):
		if self.savetext is not None:
			self.savetext = self.savetext + text


# Formatting parser -- takes a formatter and a style sheet as arguments

# XXX The use of style sheets should change: for each tag and end tag
# there should be a style definition, and a style definition should
# encompass many more parameters: font, justification, indentation,
# vspace before, vspace after, hanging tag...

wordprog = regex.compile('[^ \t\n]*')
spaceprog = regex.compile('[ \t\n]*')

class FormattingParser(CollectingParser):

	def __init__(self, formatter, stylesheet):
		CollectingParser.__init__(self)
		self.fmt = formatter
		self.stl = stylesheet
		self.savetext = None
		self.compact = 0
		self.nofill = 0
		self.resetfont()
		self.setindent(self.stl.stdindent)

	def resetfont(self):
		self.fontstack = []
		self.stylestack = []
		self.fontset = self.stl.stdfontset
		self.style = ROMAN
		self.passfont()

	def passfont(self):
		font = self.fontset[self.style]
		self.fmt.setfont(font)

	def pushstyle(self, style):
		self.stylestack.append(self.style)
		self.style = min(style, len(self.fontset)-1)
		self.passfont()

	def popstyle(self):
		self.style = self.stylestack[-1]
		del self.stylestack[-1]
		self.passfont()

	def pushfontset(self, fontset, style):
		self.fontstack.append(self.fontset)
		self.fontset = fontset
		self.pushstyle(style)

	def popfontset(self):
		self.fontset = self.fontstack[-1]
		del self.fontstack[-1]
		self.popstyle()

	def flush(self):
		self.fmt.flush()

	def setindent(self, n):
		self.fmt.setleftindent(n)

	def needvspace(self, n):
		self.fmt.needvspace(n)

	def close(self):
		HTMLParser.close(self)
		self.fmt.flush()

	def handle_literal(self, text):
		lines = string.splitfields(text, '\n')
		for i in range(1, len(lines)):
			lines[i] = string.expandtabs(lines[i], 8)
		for line in lines[:-1]:
			self.fmt.addword(line, 0)
			self.fmt.flush()
			self.fmt.nospace = 0
		for line in lines[-1:]:
			self.fmt.addword(line, 0)

	def handle_data(self, text):
		if self.savetext is not None:
			self.savetext = self.savetext + text
			return
		if self.literal:
			self.handle_literal(text)
			return
		i = 0
		n = len(text)
		while i < n:
			j = i + wordprog.match(text, i)
			word = text[i:j]
			i = j + spaceprog.match(text, j)
			self.fmt.addword(word, i-j)
			if self.nofill and '\n' in text[j:i]:
				self.fmt.flush()
				self.fmt.nospace = 0
				i = j+1
				while text[i-1] <> '\n': i = i+1

	def literal_bgn(self, tag, attrs):
		if tag == 'plaintext':
			self.flush()
		else:
			self.needvspace(1)
		self.pushfontset(self.stl.stdfontset, FIXED)
		self.setindent(self.stl.literalindent)

	def literal_end(self, tag):
		self.needvspace(1)
		self.popfontset()
		self.setindent(self.stl.stdindent)

	def start_title(self, attrs):
		self.flush()
		self.savetext = ''
	# NB end_title is unchanged

	def do_p(self, attrs):
		if self.compact:
			self.flush()
		else:
			self.needvspace(1)

	def start_h1(self, attrs):
		self.needvspace(2)
		self.setindent(self.stl.h1indent)
		self.pushfontset(self.stl.h1fontset, BOLD)
		self.fmt.setjust('c')

	def end_h1(self):
		self.popfontset()
		self.needvspace(2)
		self.setindent(self.stl.stdindent)
		self.fmt.setjust('l')

	def start_h2(self, attrs):
		self.needvspace(1)
		self.setindent(self.stl.h2indent)
		self.pushfontset(self.stl.h2fontset, BOLD)

	def end_h2(self):
		self.popfontset()
		self.needvspace(1)
		self.setindent(self.stl.stdindent)

	def start_h3(self, attrs):
		self.needvspace(1)
		self.setindent(self.stl.stdindent)
		self.pushfontset(self.stl.h3fontset, BOLD)

	def end_h3(self):
		self.popfontset()
		self.needvspace(1)
		self.setindent(self.stl.stdindent)

	def start_h4(self, attrs):
		self.needvspace(1)
		self.setindent(self.stl.stdindent)
		self.pushfontset(self.stl.stdfontset, BOLD)

	def end_h4(self):
		self.popfontset()
		self.needvspace(1)
		self.setindent(self.stl.stdindent)

	start_h5 = start_h4
	end_h5 = end_h4

	start_h6 = start_h5
	end_h6 = end_h5

	start_h7 = start_h6
	end_h7 = end_h6

	def start_ul(self, attrs):
		self.needvspace(1)
		for attrname, value in attrs:
			if attrname == 'compact':
				self.compact = 1
				self.setindent(0)
				break
		else:
			self.setindent(self.stl.ulindent)

	start_dir = start_menu = start_ol = start_ul

	do_li = do_p

	def end_ul(self):
		self.compact = 0
		self.needvspace(1)
		self.setindent(self.stl.stdindent)

	end_dir = end_menu = end_ol = end_ul

	def start_dl(self, attrs):
		for attrname, value in attrs:
			if attrname == 'compact':
				self.compact = 1
		self.needvspace(1)

	def end_dl(self):
		self.compact = 0
		self.needvspace(1)
		self.setindent(self.stl.stdindent)

	def do_dt(self, attrs):
		if self.compact:
			self.flush()
		else:
			self.needvspace(1)
		self.setindent(self.stl.stdindent)

	def do_dd(self, attrs):
		self.fmt.addword('', 1)
		self.setindent(self.stl.ddindent)

	def start_address(self, attrs):
		self.compact = 1
		self.needvspace(1)
		self.fmt.setjust('r')

	def end_address(self):
		self.compact = 0
		self.needvspace(1)
		self.setindent(self.stl.stdindent)
		self.fmt.setjust('l')

	def start_pre(self, attrs):
		self.needvspace(1)
		self.nofill = self.nofill + 1
		self.pushstyle(FIXED)

	def end_pre(self):
		self.popstyle()
		self.nofill = self.nofill - 1
		self.needvspace(1)

	start_typewriter = start_pre
	end_typewriter = end_pre

	def do_img(self, attrs):
		self.fmt.addword('(image)', 0)

	# Physical styles

	def start_tt(self, attrs): self.pushstyle(FIXED)
	def end_tt(self): self.popstyle()

	def start_b(self, attrs): self.pushstyle(BOLD)
	def end_b(self): self.popstyle()

	def start_i(self, attrs): self.pushstyle(ITALIC)
	def end_i(self): self.popstyle()

	def start_u(self, attrs): self.pushstyle(ITALIC) # Underline???
	def end_u(self): self.popstyle()

	def start_r(self, attrs): self.pushstyle(ROMAN) # Not official
	def end_r(self): self.popstyle()

	# Logical styles

	start_em = start_i
	end_em = end_i

	start_strong = start_b
	end_strong = end_b

	start_code = start_tt
	end_code = end_tt

	start_samp = start_tt
	end_samp = end_tt

	start_kbd = start_tt
	end_kbd = end_tt

	start_file = start_tt # unofficial
	end_file = end_tt

	start_var = start_i
	end_var = end_i

	start_dfn = start_i
	end_dfn = end_i

	start_cite = start_i
	end_cite = end_i

	start_hp1 = start_i
	end_hp1 = start_i

	start_hp2 = start_b
	end_hp2 = end_b

	def unknown_starttag(self, tag, attrs):
		print '*** unknown <' + tag + '>'

	def unknown_endtag(self, tag):
		print '*** unknown </' + tag + '>'


# An extension of the formatting parser which formats anchors differently.
class AnchoringParser(FormattingParser):

	def start_a(self, attrs):
		FormattingParser.start_a(self, attrs)
		if self.inanchor:
			self.fmt.bgn_anchor(self.inanchor)

	def end_a(self):
		if self.inanchor:
			self.fmt.end_anchor(self.inanchor)
			self.inanchor = 0


# Style sheet -- this is never instantiated, but the attributes
# of the class object itself are used to specify fonts to be used
# for various paragraph styles.
# A font set is a non-empty list of fonts, in the order:
# [roman, italic, bold, fixed].
# When a style is not available the nearest lower style is used

ROMAN = 0
ITALIC = 1
BOLD = 2
FIXED = 3

class NullStylesheet:
	# Fonts -- none
	stdfontset = [None]
	h1fontset = [None]
	h2fontset = [None]
	h3fontset = [None]
	# Indents
	stdindent = 2
	ddindent = 25
	ulindent = 4
	h1indent = 0
	h2indent = 0
	literalindent = 0


class X11Stylesheet(NullStylesheet):
	stdfontset = [
		'-*-helvetica-medium-r-normal-*-*-100-100-*-*-*-*-*',
		'-*-helvetica-medium-o-normal-*-*-100-100-*-*-*-*-*',
		'-*-helvetica-bold-r-normal-*-*-100-100-*-*-*-*-*',
		'-*-courier-medium-r-normal-*-*-100-100-*-*-*-*-*',
		]
	h1fontset = [
		'-*-helvetica-medium-r-normal-*-*-180-100-*-*-*-*-*',
		'-*-helvetica-medium-o-normal-*-*-180-100-*-*-*-*-*',
		'-*-helvetica-bold-r-normal-*-*-180-100-*-*-*-*-*',
		]
	h2fontset = [
		'-*-helvetica-medium-r-normal-*-*-140-100-*-*-*-*-*',
		'-*-helvetica-medium-o-normal-*-*-140-100-*-*-*-*-*',
		'-*-helvetica-bold-r-normal-*-*-140-100-*-*-*-*-*',
		]
	h3fontset = [
		'-*-helvetica-medium-r-normal-*-*-120-100-*-*-*-*-*',
		'-*-helvetica-medium-o-normal-*-*-120-100-*-*-*-*-*',
		'-*-helvetica-bold-r-normal-*-*-120-100-*-*-*-*-*',
		]
	ddindent = 40


class MacStylesheet(NullStylesheet):
	stdfontset = [
		('Geneva', 'p', 10),
		('Geneva', 'i', 10),
		('Geneva', 'b', 10),
		('Monaco', 'p', 10),
		]
	h1fontset = [
		('Geneva', 'p', 18),
		('Geneva', 'i', 18),
		('Geneva', 'b', 18),
		('Monaco', 'p', 18),
		]
	h3fontset = [
		('Geneva', 'p', 14),
		('Geneva', 'i', 14),
		('Geneva', 'b', 14),
		('Monaco', 'p', 14),
		]
	h3fontset = [
		('Geneva', 'p', 12),
		('Geneva', 'i', 12),
		('Geneva', 'b', 12),
		('Monaco', 'p', 12),
		]


if os.name == 'mac':
	StdwinStylesheet = MacStylesheet
else:
	StdwinStylesheet = X11Stylesheet


class GLStylesheet(NullStylesheet):
	stdfontset = [
		'Helvetica 10',
		'Helvetica-Italic 10',
		'Helvetica-Bold 10',
		'Courier 10',
		]
	h1fontset = [
		'Helvetica 18',
		'Helvetica-Italic 18',
		'Helvetica-Bold 18',
		'Courier 18',
		]
	h2fontset = [
		'Helvetica 14',
		'Helvetica-Italic 14',
		'Helvetica-Bold 14',
		'Courier 14',
		]
	h3fontset = [
		'Helvetica 12',
		'Helvetica-Italic 12',
		'Helvetica-Bold 12',
		'Courier 12',
		]


# Test program -- produces no output but times how long it takes
# to send a document to a null formatter, exclusive of I/O

def test():
	import fmt
	import time
	if sys.argv[1:]: file = sys.argv[1]
	else: file = 'test.html'
	data = open(file, 'r').read()
	t0 = time.time()
	fmtr = fmt.WritingFormatter(sys.stdout, 79)
	p = FormattingParser(fmtr, NullStylesheet)
	p.feed(data)
	p.close()
	t1 = time.time()
	print
	print '*** Formatting time:', round(t1-t0, 3), 'seconds.'


# Test program using stdwin

def testStdwin():
	import stdwin, fmt
	from stdwinevents import *
	if sys.argv[1:]: file = sys.argv[1]
	else: file = 'test.html'
	data = open(file, 'r').read()
	window = stdwin.open('testStdwin')
	b = None
	while 1:
		etype, ewin, edetail = stdwin.getevent()
		if etype == WE_CLOSE:
			break
		if etype == WE_SIZE:
			window.setdocsize(0, 0)
			window.setorigin(0, 0)
			window.change((0, 0), (10000, 30000)) # XXX
		if etype == WE_DRAW:
			if not b:
				b = fmt.StdwinBackEnd(window, 1)
				f = fmt.BaseFormatter(b.d, b)
				p = FormattingParser(f, MacStylesheet)
				p.feed(data)
				p.close()
				b.finish()
			else:
				b.redraw(edetail)
	window.close()


# Test program using GL

def testGL():
	import gl, GL, fmt
	if sys.argv[1:]: file = sys.argv[1]
	else: file = 'test.html'
	data = open(file, 'r').read()
	W, H = 600, 600
	gl.foreground()
	gl.prefsize(W, H)
	wid = gl.winopen('testGL')
	gl.ortho2(0, W, H, 0)
	gl.color(GL.WHITE)
	gl.clear()
	gl.color(GL.BLACK)
	b = fmt.GLBackEnd(wid)
	f = fmt.BaseFormatter(b.d, b)
	p = FormattingParser(f, GLStylesheet)
	p.feed(data)
	p.close()
	b.finish()
	#
	import time
	time.sleep(5)


if __name__ == '__main__':
	test()
