# A class to help applications that do fancy text formatting.
# You create an instance each time you must redraw the window.
# Set the initial left, top and right coordinates;
# then feed it words, font changes and vertical movements.
#
# This class should eventually be extended to support much fancier
# formatting, along the lines of TeX; for now, a very simple model
# is sufficient.
#
class formatter:
	#
	# Initialize a formatter instance.
	# Pass the window's drawing object, and left, top, right
	# coordinates of the drawing space as arguments.
	#
	def init(self, (d, left, top, right)):
		self.d = d		# Drawing object
		self.left = left	# Left margin
		self.right = right	# Right margin
		self.v = top		# Top of current line
		self.center = 0
		self.justify = 1
		self.setfont('')	# Default font
		self._reset()		# Prepare for new line
		return self
	#
	# Reset for start of fresh line.
	#
	def _reset(self):
		self.boxes = []		# Boxes and glue still to be output
		self.sum_width = 0	# Total width of boxes
		self.sum_space = 0	# Total space between boxes
		self.sum_stretch = 0	# Total stretch for space between boxes
		self.max_ascent = 0	# Max ascent of current line
		self.max_descent = 0	# Max descent of current line
		self.avail_width = self.right - self.left
		self.hang_indent = 0
	#
	# Set the current font, and compute some values from it.
	#
	def setfont(self, font):
		self.font = font
		self.d.setfont(font)
		self.font_space = self.d.textwidth(' ')
		self.font_ascent = self.d.baseline()
		self.font_descent = self.d.lineheight() - self.font_ascent
	#
	# Add a word to the list of boxes; first flush if line is full.
	# Space and stretch factors are expressed in fractions
	# of the current font's space width.
	# (Two variations: one without, one with explicit stretch factor.)
	#
	def addword(self, (word, spacefactor)):
		self.addwordstretch(word, spacefactor, spacefactor)
	#
	def addwordstretch(self, (word, spacefactor, stretchfactor)):
		width = self.d.textwidth(word)
		if width > self.avail_width:
			self._flush(1)
		space = int(float(self.font_space) * float(spacefactor))
		stretch = int(float(self.font_space) * float(stretchfactor))
		box = (self.font, word, width, space, stretch)
		self.boxes.append(box)
		self.sum_width = self.sum_width + width
		self.sum_space = self.sum_space + space
		self.sum_stretch = self.sum_stretch + stretch
		self.max_ascent = max(self.font_ascent, self.max_ascent)
		self.max_descent = max(self.font_descent, self.max_descent)
		self.avail_width = self.avail_width - width - space
	#
	# Flush current line and start a new one.
	# Flushing twice is harmless (i.e. does not introduce a blank line).
	# (Two versions: the internal one has a parameter for justification.)
	#
	def flush(self):
		self._flush(0)
	#
	def _flush(self, justify):
		if not self.boxes:
			return
		#
		# Compute amount of stretch needed.
		#
		if justify and self.justify or self.center:
			#
			# Compute extra space to fill;
			# this is avail_width plus glue from last box.
			# Also compute available stretch.
			#
			last_box = self.boxes[len(self.boxes)-1]
			font, word, width, space, stretch = last_box
			tot_extra = self.avail_width + space
			tot_stretch = self.sum_stretch - stretch
		else:
			tot_extra = tot_stretch = 0
		#
		# Output the boxes.
		#
		baseline = self.v + self.max_ascent
		h = self.left + self.hang_indent
		if self.center:
			h = h + tot_extra / 2
			tot_extra = tot_stretch = 0
		for font, word, width, space, stretch in self.boxes:
			self.d.setfont(font)
			v = baseline - self.d.baseline()
			self.d.text((h, v), word)
			h = h + width + space
			if tot_extra > 0 and tot_stretch > 0:
				extra = stretch * tot_extra / tot_stretch
				h = h + extra
				tot_extra = tot_extra - extra
				tot_stretch = tot_stretch - stretch
		#
		# Prepare for next line.
		#
		self.v = baseline + self.max_descent
		self.d.setfont(self.font)
		self._reset()
	#
	# Add vertical space; first flush.
	# Vertical space is expressed in fractions of the current
	# font's line height.
	#
	def vspace(self, lines):
		self.vspacepixels(int(lines * self.d.lineheight()))
	#
	# Add vertical space given in pixels.
	#
	def vspacepixels(self, dv):
		self.flush()
		self.v = self.v + dv
	#
	# Set temporary (hanging) indent, for paragraph start.
	# First flush.
	#
	def tempindent(self, space):
		self.flush()
		hang = int(float(self.font_space) * float(space))
		self.hang_indent = hang
		self.avail_width = self.avail_width - hang
	#
	# Add (permanent) left indentation.  First flush.
	#
	def addleftindent(self, space):
		self.flush()
		self.left = self.left \
			+ int(float(self.font_space) * float(space))
		self._reset()
	#


# Test procedure
#
def test():
	import stdwin, stdwinq
	from stdwinevents import *
	try:
		import mac
		# Mac font assignments:
		font1 = 'times', '', 12
		font2 = 'times', 'b', 14
	except ImportError:
		# X11R4 font assignments
		font1 = '*times-medium-r-*-120-*'
		font2 = '*times-bold-r-*-140-*'
	words = \
	    ['The','quick','brown','fox','jumps','over','the','lazy','dog.']
	words = words * 2
	stage = 0
	stages = [(0,0,'ragged'), (1,0,'justified'), (0,1,'centered')]
	justify, center, title = stages[stage]
	stdwin.setdefwinsize(300,200)
	w = stdwin.open(title)
	winsize = w.getwinsize()
	while 1:
		type, window, detail = stdwinq.getevent()
		if type = WE_CLOSE:
			break
		elif type = WE_SIZE:
			newsize = w.getwinsize()
			if newsize <> winsize:
				w.change((0,0), winsize)
				winsize = newsize
				w.change((0,0), winsize)
		elif type = WE_MOUSE_DOWN:
			stage = (stage + 1) % len(stages)
			justify, center, title = stages[stage]
			w.settitle(title)
			w.change((0, 0), (1000, 1000))
		elif type = WE_DRAW:
			width, height = winsize
			f = formatter().init(w.begindrawing(), 0, 0, width)
			f.center = center
			f.justify = justify
			if not center:
				f.tempindent(5)
			for font in font1, font2, font1:
				f.setfont(font)
				for word in words:
					space = 1 + (word[-1:] = '.')
					f.addword(word, space)
					if center and space > 1:
						f.flush()
			f.flush()
			height = f.v
			del f
			w.setdocsize(0, height)
