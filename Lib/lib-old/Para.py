# Text formatting abstractions 
# Note -- this module is obsolete, it's too slow anyway


# Oft-used type object
Int = type(0)


# Represent a paragraph.  This is a list of words with associated
# font and size information, plus indents and justification for the
# entire paragraph.
# Once the words have been added to a paragraph, it can be laid out
# for different line widths.  Once laid out, it can be rendered at
# different screen locations.  Once rendered, it can be queried
# for mouse hits, and parts of the text can be highlighted
class Para:
	#
	def __init__(self):
		self.words = [] # The words
		self.just = 'l' # Justification: 'l', 'r', 'lr' or 'c'
		self.indent_left = self.indent_right = self.indent_hang = 0
		# Final lay-out parameters, may change
		self.left = self.top = self.right = self.bottom = \
			self.width = self.height = self.lines = None
	#
	# Add a word, computing size information for it.
	# Words may also be added manually by appending to self.words
	# Each word should be a 7-tuple:
	# (font, text, width, space, stretch, ascent, descent)
	def addword(self, d, font, text, space, stretch):
		if font <> None:
			d.setfont(font)
		width = d.textwidth(text)
		ascent = d.baseline()
		descent = d.lineheight() - ascent
		spw = d.textwidth(' ')
		space = space * spw
		stretch = stretch * spw
		tuple = (font, text, width, space, stretch, ascent, descent)
		self.words.append(tuple)
	#
	# Hooks to begin and end anchors -- insert numbers in the word list!
	def bgn_anchor(self, id):
		self.words.append(id)
	#
	def end_anchor(self, id):
		self.words.append(0)
	#
	# Return the total length (width) of the text added so far, in pixels
	def getlength(self):
		total = 0
		for word in self.words:
			if type(word) <> Int:
				total = total + word[2] + word[3]
		return total
	#
	# Tab to a given position (relative to the current left indent):
	# remove all stretch, add fixed space up to the new indent.
	# If the current position is already at the tab stop,
	# don't add any new space (but still remove the stretch)
	def tabto(self, tab):
		total = 0
		as, de = 1, 0
		for i in range(len(self.words)):
			word = self.words[i]
			if type(word) == Int: continue
			(fo, te, wi, sp, st, as, de) = word
			self.words[i] = (fo, te, wi, sp, 0, as, de)
			total = total + wi + sp
		if total < tab:
			self.words.append((None, '', 0, tab-total, 0, as, de))
	#
	# Make a hanging tag: tab to hang, increment indent_left by hang,
	# and reset indent_hang to -hang
	def makehangingtag(self, hang):
		self.tabto(hang)
		self.indent_left = self.indent_left + hang
		self.indent_hang = -hang
	#
	# Decide where the line breaks will be given some screen width
	def layout(self, linewidth):
		self.width = linewidth
		height = 0
		self.lines = lines = []
		avail1 = self.width - self.indent_left - self.indent_right
		avail = avail1 - self.indent_hang
		words = self.words
		i = 0
		n = len(words)
		lastfont = None
		while i < n:
			firstfont = lastfont
			charcount = 0
			width = 0
			stretch = 0
			ascent = 0
			descent = 0
			lsp = 0
			j = i
			while i < n:
				word = words[i]
				if type(word) == Int:
					if word > 0 and width >= avail:
						break
					i = i+1
					continue
				fo, te, wi, sp, st, as, de = word
				if width + wi > avail and width > 0 and wi > 0:
					break
				if fo <> None:
					lastfont = fo
					if width == 0:
						firstfont = fo
				charcount = charcount + len(te) + (sp > 0)
				width = width + wi + sp
				lsp = sp
				stretch = stretch + st
				lst = st
				ascent = max(ascent, as)
				descent = max(descent, de)
				i = i+1
			while i > j and type(words[i-1]) == Int and \
				words[i-1] > 0: i = i-1
			width = width - lsp
			if i < n:
				stretch = stretch - lst
			else:
				stretch = 0
			tuple = i-j, firstfont, charcount, width, stretch, \
				ascent, descent
			lines.append(tuple)
			height = height + ascent + descent
			avail = avail1
		self.height = height
	#
	# Call a function for all words in a line
	def visit(self, wordfunc, anchorfunc):
		avail1 = self.width - self.indent_left - self.indent_right
		avail = avail1 - self.indent_hang
		v = self.top
		i = 0
		for tuple in self.lines:
			wordcount, firstfont, charcount, width, stretch, \
				ascent, descent = tuple
			h = self.left + self.indent_left
			if i == 0: h = h + self.indent_hang
			extra = 0
			if self.just == 'r': h = h + avail - width
			elif self.just == 'c': h = h + (avail - width) / 2
			elif self.just == 'lr' and stretch > 0:
				extra = avail - width
			v2 = v + ascent + descent
			for j in range(i, i+wordcount):
				word = self.words[j]
				if type(word) == Int:
					ok = anchorfunc(self, tuple, word, \
							h, v)
					if ok <> None: return ok
					continue
				fo, te, wi, sp, st, as, de = word
				if extra > 0 and stretch > 0:
					ex = extra * st / stretch
					extra = extra - ex
					stretch = stretch - st
				else:
					ex = 0
				h2 = h + wi + sp + ex
				ok = wordfunc(self, tuple, word, h, v, \
					h2, v2, (j==i), (j==i+wordcount-1))
				if ok <> None: return ok
				h = h2
			v = v2
			i = i + wordcount
			avail = avail1
	#
	# Render a paragraph in "drawing object" d, using the rectangle
	# given by (left, top, right) with an unspecified bottom.
	# Return the computed bottom of the text.
	def render(self, d, left, top, right):
		if self.width <> right-left:
			self.layout(right-left)
		self.left = left
		self.top = top
		self.right = right
		self.bottom = self.top + self.height
		self.anchorid = 0
		try:
			self.d = d
			self.visit(self.__class__._renderword, \
				   self.__class__._renderanchor)
		finally:
			self.d = None
		return self.bottom
	#
	def _renderword(self, tuple, word, h, v, h2, v2, isfirst, islast):
		if word[0] <> None: self.d.setfont(word[0])
		baseline = v + tuple[5]
		self.d.text((h, baseline - word[5]), word[1])
		if self.anchorid > 0:
			self.d.line((h, baseline+2), (h2, baseline+2))
	#
	def _renderanchor(self, tuple, word, h, v):
		self.anchorid = word
	#
	# Return which anchor(s) was hit by the mouse
	def hitcheck(self, mouseh, mousev):
		self.mouseh = mouseh
		self.mousev = mousev
		self.anchorid = 0
		self.hits = []
		self.visit(self.__class__._hitcheckword, \
			   self.__class__._hitcheckanchor)
		return self.hits
	#
	def _hitcheckword(self, tuple, word, h, v, h2, v2, isfirst, islast):
		if self.anchorid > 0 and h <= self.mouseh <= h2 and \
			v <= self.mousev <= v2:
			self.hits.append(self.anchorid)
	#
	def _hitcheckanchor(self, tuple, word, h, v):
		self.anchorid = word
	#
	# Return whether the given anchor id is present
	def hasanchor(self, id):
		return id in self.words or -id in self.words
	#
	# Extract the raw text from the word list, substituting one space
	# for non-empty inter-word space, and terminating with '\n'
	def extract(self):
		text = ''
		for w in self.words:
			if type(w) <> Int:
				word = w[1]
				if w[3]: word = word + ' '
				text = text + word
		return text + '\n'
	#
	# Return which character position was hit by the mouse, as
	# an offset in the entire text as returned by extract().
	# Return None if the mouse was not in this paragraph
	def whereis(self, d, mouseh, mousev):
		if mousev < self.top or mousev > self.bottom:
			return None
		self.mouseh = mouseh
		self.mousev = mousev
		self.lastfont = None
		self.charcount = 0
		try:
			self.d = d
			return self.visit(self.__class__._whereisword, \
					  self.__class__._whereisanchor)
		finally:
			self.d = None
	#
	def _whereisword(self, tuple, word, h1, v1, h2, v2, isfirst, islast):
		fo, te, wi, sp, st, as, de = word
		if fo <> None: self.lastfont = fo
		h = h1
		if isfirst: h1 = 0
		if islast: h2 = 999999
		if not (v1 <= self.mousev <= v2 and h1 <= self.mouseh <= h2):
			self.charcount = self.charcount + len(te) + (sp > 0)
			return
		if self.lastfont <> None:
			self.d.setfont(self.lastfont)
		cc = 0
		for c in te:
			cw = self.d.textwidth(c)
			if self.mouseh <= h + cw/2:
				return self.charcount + cc
			cc = cc+1
			h = h+cw
		self.charcount = self.charcount + cc
		if self.mouseh <= (h+h2) / 2:
			return self.charcount
		else:
			return self.charcount + 1
	#
	def _whereisanchor(self, tuple, word, h, v):
		pass
	#
	# Return screen position corresponding to position in paragraph.
	# Return tuple (h, vtop, vbaseline, vbottom).
	# This is more or less the inverse of whereis()
	def screenpos(self, d, pos):
		if pos < 0:
			ascent, descent = self.lines[0][5:7]
			return self.left, self.top, self.top + ascent, \
				self.top + ascent + descent
		self.pos = pos
		self.lastfont = None
		try:
			self.d = d
			ok = self.visit(self.__class__._screenposword, \
					self.__class__._screenposanchor)
		finally:
			self.d = None
		if ok == None:
			ascent, descent = self.lines[-1][5:7]
			ok = self.right, self.bottom - ascent - descent, \
				self.bottom - descent, self.bottom
		return ok
	#
	def _screenposword(self, tuple, word, h1, v1, h2, v2, isfirst, islast):
		fo, te, wi, sp, st, as, de = word
		if fo <> None: self.lastfont = fo
		cc = len(te) + (sp > 0)
		if self.pos > cc:
			self.pos = self.pos - cc
			return
		if self.pos < cc:
			self.d.setfont(self.lastfont)
			h = h1 + self.d.textwidth(te[:self.pos])
		else:
			h = h2
		ascent, descent = tuple[5:7]
		return h, v1, v1+ascent, v2
	#
	def _screenposanchor(self, tuple, word, h, v):
		pass
	#
	# Invert the stretch of text between pos1 and pos2.
	# If pos1 is None, the beginning is implied;
	# if pos2 is None, the end is implied.
	# Undoes its own effect when called again with the same arguments
	def invert(self, d, pos1, pos2):
		if pos1 == None:
			pos1 = self.left, self.top, self.top, self.top
		else:
			pos1 = self.screenpos(d, pos1)
		if pos2 == None:
			pos2 = self.right, self.bottom,self.bottom,self.bottom
		else:
			pos2 = self.screenpos(d, pos2)
		h1, top1, baseline1, bottom1 = pos1
		h2, top2, baseline2, bottom2 = pos2
		if bottom1 <= top2:
			d.invert((h1, top1), (self.right, bottom1))
			h1 = self.left
			if bottom1 < top2:
				d.invert((h1, bottom1), (self.right, top2))
			top1, bottom1 = top2, bottom2
		d.invert((h1, top1), (h2, bottom2))
