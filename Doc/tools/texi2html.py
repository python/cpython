#! /usr/local/bin/python

# Convert GNU texinfo files into HTML, one file per node.
# Based on Texinfo 2.14.
# Usage: texi2html [-d] [-d] inputfile outputdirectory
# The input file must be a complete texinfo file, e.g. emacs.texi.
# This creates many files (one per info node) in the output directory,
# overwriting existing files of the same name.  All files created have
# ".html" as their extension.


# XXX To do:
# - handle @comment*** correctly
# - handle @xref {some words} correctly
# - handle @ftable correctly (items aren't indexed?)
# - handle @itemx properly
# - handle @exdent properly
# - add links directly to the proper line from indices
# - check against the definitive list of @-cmds; we still miss (among others):
# - @set, @clear, @ifset, @ifclear
# - @defindex (hard)
# - @c(omment) in the middle of a line (rarely used)
# - @this* (not really needed, only used in headers anyway)
# - @today{} (ever used outside title page?)


import os
import regex
import regsub
import string

MAGIC = '\\input texinfo'

cmprog = regex.compile('^@\([a-z]+\)\([ \t]\|$\)') # Command (line-oriented)
blprog = regex.compile('^[ \t]*$') # Blank line
kwprog = regex.compile('@[a-z]+') # Keyword (embedded, usually with {} args)
spprog = regex.compile('[\n@{}&<>]') # Special characters in running text
miprog = regex.compile( \
	'^\* \([^:]*\):\(:\|[ \t]*\([^\t,\n.]+\)\([^ \t\n]*\)\)[ \t\n]*')
					# menu item (Yuck!)

class TexinfoParser:

	# Initialize an instance
	def __init__(self):
		self.unknown = {}	# statistics about unknown @-commands
		self.filenames = {}	# Check for identical filenames
		self.debugging = 0	# larger values produce more output
		self.nodefp = None	# open file we're writing to
		self.nodelineno = 0	# Linenumber relative to node
		self.links = None	# Links from current node
		self.savetext = None	# If not None, save text head instead
		self.dirname = 'tmp'	# directory where files are created
		self.includedir = '.'	# directory to search @include files
		self.nodename = ''	# name of current node
		self.topname = ''	# name of top node (first node seen)
		self.title = ''		# title of this whole Texinfo tree
		self.resetindex()	# Reset all indices
		self.contents = []	# Reset table of contents
		self.numbering = []	# Reset section numbering counters
		self.nofill = 0		# Normal operation: fill paragraphs
		self.goodset=['html']	# Names that should be parsed in ifset
		self.stackinfo={}	# Keep track of state in the stack
		# XXX The following should be reset per node?!
		self.footnotes = []	# Reset list of footnotes
		self.itemarg = None	# Reset command used by @item
		self.itemnumber = None	# Reset number for @item in @enumerate
		self.itemindex = None	# Reset item index name

	# Set (output) directory name
	def setdirname(self, dirname):
		self.dirname = dirname

	# Set include directory name
	def setincludedir(self, includedir):
		self.includedir = includedir

	# Parse the contents of an entire file
	def parse(self, fp):
		line = fp.readline()
		lineno = 1
		while line and (line[0] == '%' or blprog.match(line) >= 0):
			line = fp.readline()
			lineno = lineno + 1
		if line[:len(MAGIC)] <> MAGIC:
			raise SyntaxError, 'file does not begin with '+`MAGIC`
		self.parserest(fp, lineno)

	# Parse the contents of a file, not expecting a MAGIC header
	def parserest(self, fp, initial_lineno):
		lineno = initial_lineno
		self.done = 0
		self.skip = 0
		self.stack = []
		accu = []
		while not self.done:
			line = fp.readline()
			self.nodelineno = self.nodelineno + 1
			if not line:
				if accu:
					if not self.skip: self.process(accu)
					accu = []
				if initial_lineno > 0:
					print '*** EOF before @bye'
				break
			lineno = lineno + 1
			if cmprog.match(line) >= 0:
				a, b = cmprog.regs[1]
				cmd = line[a:b]
				if cmd in ('noindent', 'refill'):
					accu.append(line)
				else:
					if accu:
						if not self.skip:
							self.process(accu)
						accu = []
					self.command(line)
			elif blprog.match(line) >= 0 and \
			     'format' not in self.stack and \
			     'example' not in self.stack:
				if accu:
					if not self.skip:
						self.process(accu)
						self.write('<P>\n')
						accu = []
			else:
				# Append the line including trailing \n!
				accu.append(line)
		#
		if self.skip:
			print '*** Still skipping at the end'
		if self.stack:
			print '*** Stack not empty at the end'
			print '***', self.stack

	# Start saving text in a buffer instead of writing it to a file
	def startsaving(self):
		if self.savetext <> None:
			print '*** Recursively saving text, expect trouble'
		self.savetext = ''

	# Return the text saved so far and start writing to file again
	def collectsavings(self):
		savetext = self.savetext
		self.savetext = None
		return savetext or ''

	# Write text to file, or save it in a buffer, or ignore it
	def write(self, *args):
		text = string.joinfields(args, '')
		if self.savetext <> None:
			self.savetext = self.savetext + text
		elif self.nodefp:
			self.nodefp.write(text)

	# Complete the current node -- write footnotes and close file
	def endnode(self):
		if self.savetext <> None:
			print '*** Still saving text at end of node'
			dummy = self.collectsavings()
		if self.footnotes:
			self.writefootnotes()
		if self.nodefp:
			if self.nodelineno > 20:
				self.write ('<HR>\n')
				[name, next, prev, up] = self.nodelinks[:4]
				self.link('Next', next)
				self.link('Prev', prev)
				self.link('Up', up)
				if self.nodename <> self.topname:
					self.link('Top', self.topname)
				self.write ('<HR>\n')
			self.write('</BODY>\n')
			self.nodefp.close()
		self.nodefp = None
		self.nodename = ''

	# Process a list of lines, expanding embedded @-commands
	# This mostly distinguishes between menus and normal text
	def process(self, accu):
		if self.debugging > 1:
			print self.skip, self.stack,
			if accu: print accu[0][:30],
			if accu[0][30:] or accu[1:]: print '...',
			print
		if self.stack and self.stack[-1] == 'menu':
			# XXX should be done differently
			for line in accu:
				if miprog.match(line) < 0:
					line = string.strip(line) + '\n'
					self.expand(line)
					continue
				(bgn, end), (a, b), (c, d), (e, f), (g, h) = \
					miprog.regs[:5]
				label = line[a:b]
				nodename = line[c:d]
				if nodename[0] == ':': nodename = label
				else: nodename = line[e:f]
				punct = line[g:h]
				self.write('<DT><A HREF="', \
					makefile(nodename), \
					'" TYPE=Menu>', nodename, \
					'</A>', punct, '\n<DD>')
				self.expand(line[end:])
		else:
			text = string.joinfields(accu, '')
			self.expand(text)

	# Write a string, expanding embedded @-commands
	def expand(self, text):
		stack = []
		i = 0
		n = len(text)
		while i < n:
			start = i
			i = spprog.search(text, i)
			if i < 0:
				self.write(text[start:])
				break
			self.write(text[start:i])
			c = text[i]
			i = i+1
			if c == '\n':
				if self.nofill > 0:
					self.write('<P>\n')
				else:
					self.write('\n')
				continue
			if c == '<':
				self.write('&lt;')
				continue
			if c == '>':
				self.write('&gt;')
				continue
			if c == '&':
				self.write('&amp;')
				continue
			if c == '{':
				stack.append('')
				continue
			if c == '}':
				if not stack:
					print '*** Unmatched }'
					self.write('}')
					continue
				cmd = stack[-1]
				del stack[-1]
				try:
					method = getattr(self, 'close_' + cmd)
				except AttributeError:
					self.unknown_close(cmd)
					continue
				method()
				continue
			if c <> '@':
				# Cannot happen unless spprog is changed
				raise RuntimeError, 'unexpected funny '+`c`
			start = i
			while i < n and text[i] in string.letters: i = i+1
			if i == start:
				# @ plus non-letter: literal next character
				i = i+1
				c = text[start:i]
				if c == ':':
					# `@:' means no extra space after
					# preceding `.', `?', `!' or `:'
					pass
				else:
					# `@.' means a sentence-ending period;
					# `@@', `@{', `@}' quote `@', `{', `}'
					self.write(c)
				continue
			cmd = text[start:i]
			if i < n and text[i] == '{':
				i = i+1
				stack.append(cmd)
				try:
					method = getattr(self, 'open_' + cmd)
				except AttributeError:
					self.unknown_open(cmd)
					continue
				method()
				continue
			try:
				method = getattr(self, 'handle_' + cmd)
			except AttributeError:
				self.unknown_handle(cmd)
				continue
			method()
		if stack:
			print '*** Stack not empty at para:', stack

	# --- Handle unknown embedded @-commands ---

	def unknown_open(self, cmd):
		print '*** No open func for @' + cmd + '{...}'
		cmd = cmd + '{'
		self.write('@', cmd)
		if not self.unknown.has_key(cmd):
			self.unknown[cmd] = 1
		else:
			self.unknown[cmd] = self.unknown[cmd] + 1

	def unknown_close(self, cmd):
		print '*** No close func for @' + cmd + '{...}'
		cmd = '}' + cmd
		self.write('}')
		if not self.unknown.has_key(cmd):
			self.unknown[cmd] = 1
		else:
			self.unknown[cmd] = self.unknown[cmd] + 1

	def unknown_handle(self, cmd):
		print '*** No handler for @' + cmd
		self.write('@', cmd)
		if not self.unknown.has_key(cmd):
			self.unknown[cmd] = 1
		else:
			self.unknown[cmd] = self.unknown[cmd] + 1

	# XXX The following sections should be ordered as the texinfo docs

	# --- Embedded @-commands without {} argument list --

	def handle_noindent(self): pass

	def handle_refill(self): pass

	# --- Include file handling ---

	def do_include(self, args):
		file = args
		file = os.path.join(self.includedir, file)
		try:
			fp = open(file, 'r')
		except IOError, msg:
			print '*** Can\'t open include file', `file`
			return
		if self.debugging:
			print '--> file', `file`
		save_done = self.done
		save_skip = self.skip
		save_stack = self.stack
		self.parserest(fp, 0)
		fp.close()
		self.done = save_done
		self.skip = save_skip
		self.stack = save_stack
		if self.debugging:
			print '<-- file', `file`

	# --- Special Insertions ---

	def open_dmn(self): pass
	def close_dmn(self): pass

	def open_dots(self): self.write('...')
	def close_dots(self): pass

	def open_bullet(self): pass
	def close_bullet(self): pass

	def open_TeX(self): self.write('TeX')
	def close_TeX(self): pass

	def handle_copyright(self): self.write('(C)')

	def open_minus(self): self.write('-')
	def close_minus(self): pass

	# --- Special Glyphs for Examples ---

	def open_result(self): self.write('=&gt;')
	def close_result(self): pass

	def open_expansion(self): self.write('==&gt;')
	def close_expansion(self): pass

	def open_print(self): self.write('-|')
	def close_print(self): pass

	def open_error(self): self.write('error--&gt;')
	def close_error(self): pass

	def open_equiv(self): self.write('==')
	def close_equiv(self): pass

	def open_point(self): self.write('-!-')
	def close_point(self): pass

	# --- Cross References ---

	def open_pxref(self):
		self.write('see ')
		self.startsaving()
	def close_pxref(self):
		self.makeref()

	def open_xref(self):
		self.write('See ')
		self.startsaving()
	def close_xref(self):
		self.makeref()

	def open_ref(self):
		self.startsaving()
	def close_ref(self):
		self.makeref()

	def open_inforef(self):
		self.write('See info file ')
		self.startsaving()
	def close_inforef(self):
		text = self.collectsavings()
		args = string.splitfields(text, ',')
		n = len(args)
		for i in range(n):
			args[i] = string.strip(args[i])
		while len(args) < 3: args.append('')
		node = args[0]
		file = args[2]
		self.write('`', file, '\', node `', node, '\'')

	def makeref(self):
		text = self.collectsavings()
		args = string.splitfields(text, ',')
		n = len(args)
		for i in range(n):
			args[i] = string.strip(args[i])
		while len(args) < 5: args.append('')
		nodename = label = args[0]
		if args[2]: label = args[2]
		file = args[3]
		title = args[4]
		href = makefile(nodename)
		if file:
			href = '../' + file + '/' + href
		self.write('<A HREF="', href, '">', label, '</A>')

	# --- Marking Words and Phrases ---

	# --- Other @xxx{...} commands ---

	def open_(self): pass # Used by {text enclosed in braces}
	def close_(self): pass

	open_asis = open_
	close_asis = close_

	def open_cite(self): self.write('<CITE>')
	def close_cite(self): self.write('</CITE>')

	def open_code(self): self.write('<CODE>')
	def close_code(self): self.write('</CODE>')

	open_t = open_code
	close_t = close_code

	def open_dfn(self): self.write('<DFN>')
	def close_dfn(self): self.write('</DFN>')

	def open_emph(self): self.write('<I>')
	def close_emph(self): self.write('</I>')

	open_i = open_emph
	close_i = close_emph

	def open_footnote(self):
		if self.savetext <> None:
			print '*** Recursive footnote -- expect weirdness'
		id = len(self.footnotes) + 1
		self.write('<A NAME="footnoteref', `id`, \
			'" HREF="#footnotetext', `id`, '">(', `id`, ')</A>')
		self.savetext = ''

	def close_footnote(self):
		id = len(self.footnotes) + 1
		self.footnotes.append(`id`, self.savetext)
		self.savetext = None

	def writefootnotes(self):
		self.write('<H2>---------- Footnotes ----------</H2>\n')
		for id, text in self.footnotes:
			self.write('<A NAME="footnotetext', id, \
				'" HREF="#footnoteref', id, '">(', \
				id, ')</A>\n', text, '<P>\n')
		self.footnotes = []

	def open_file(self): self.write('<FILE>')
	def close_file(self): self.write('</FILE>')

	def open_kbd(self): self.write('<KBD>')
	def close_kbd(self): self.write('</KBD>')

	def open_key(self): self.write('<KEY>')
	def close_key(self): self.write('</KEY>')

	def open_r(self): self.write('<R>')
	def close_r(self): self.write('</R>')

	def open_samp(self): self.write('`<SAMP>')
	def close_samp(self): self.write('</SAMP>\'')

	def open_sc(self): self.write('<SMALLCAPS>')
	def close_sc(self): self.write('</SMALLCAPS>')

	def open_strong(self): self.write('<B>')
	def close_strong(self): self.write('</B>')

	open_b = open_strong
	close_b = close_strong

	def open_var(self): self.write('<VAR>')
	def close_var(self): self.write('</VAR>')

	def open_w(self): self.write('<NOBREAK>')
	def close_w(self): self.write('</NOBREAK>')

	open_titlefont = open_
	close_titlefont = close_

	def open_small(self): pass
	def close_small(self): pass
	
	def command(self, line):
		a, b = cmprog.regs[1]
		cmd = line[a:b]
		args = string.strip(line[b:])
		if self.debugging > 1:
			print self.skip, self.stack, '@' + cmd, args
		try:
			func = getattr(self, 'do_' + cmd)
		except AttributeError:
			try:
				func = getattr(self, 'bgn_' + cmd)
			except AttributeError:
				self.unknown_cmd(cmd, args)
				return
			self.stack.append(cmd)
			func(args)
			return
		if not self.skip or cmd == 'end':
			func(args)

	def unknown_cmd(self, cmd, args):
		print '*** unknown', '@' + cmd, args
		if not self.unknown.has_key(cmd):
			self.unknown[cmd] = 1
		else:
			self.unknown[cmd] = self.unknown[cmd] + 1

	def do_end(self, args):
		words = string.split(args)
		if not words:
			print '*** @end w/o args'
		else:
			cmd = words[0]
			if not self.stack or self.stack[-1] <> cmd:
				print '*** @end', cmd, 'unexpected'
			else:
				del self.stack[-1]
			try:
				func = getattr(self, 'end_' + cmd)
			except AttributeError:
				self.unknown_end(cmd)
				return
			func()

	def unknown_end(self, cmd):
		cmd = 'end ' + cmd
		print '*** unknown', '@' + cmd
		if not self.unknown.has_key(cmd):
			self.unknown[cmd] = 1
		else:
			self.unknown[cmd] = self.unknown[cmd] + 1

	# --- Comments ---

	def do_comment(self, args): pass
	do_c = do_comment

	# --- Conditional processing ---

	def bgn_ifinfo(self, args): pass
	def end_ifinfo(self): pass

	def bgn_iftex(self, args): self.skip = self.skip + 1
	def end_iftex(self): self.skip = self.skip - 1

	def bgn_ignore(self, args): self.skip = self.skip + 1
	def end_ignore(self): self.skip = self.skip - 1

	def bgn_tex(self, args): self.skip = self.skip + 1
	def end_tex(self): self.skip = self.skip - 1

	def bgn_set(self, args):
		if args not in self.goodset:
			self.gooset.append(args)
			
	def bgn_clear(self, args):
		if args in self.goodset:
			self.gooset.remove(args)
			
	def bgn_ifset(self, args):
		if args not in self.goodset:
			self.skip = self.skip + 1
			self.stackinfo[len(self.stack)] = 1
		else:
			self.stackinfo[len(self.stack)] = 0
	def end_ifset(self):
		print self.stack
		print self.stackinfo
		if self.stackinfo[len(self.stack) + 1]:
			self.skip = self.skip - 1
		del self.stackinfo[len(self.stack) + 1]

	def bgn_ifclear(self, args):
		if args in self.goodset:
			self.skip = self.skip + 1
			self.stackinfo[len(self.stack)] = 1
		else:
			self.stackinfo[len(self.stack)] = 0
		
	end_ifclear = end_ifset
	
	# --- Beginning a file ---

	do_finalout = do_comment
	do_setchapternewpage = do_comment
	do_setfilename = do_comment

	def do_settitle(self, args):
		self.title = args

	def do_parskip(self, args): pass
	
	# --- Ending a file ---

	def do_bye(self, args):
		self.done = 1

	# --- Title page ---

	def bgn_titlepage(self, args): self.skip = self.skip + 1
	def end_titlepage(self): self.skip = self.skip - 1

	def do_center(self, args):
		# Actually not used outside title page...
		self.write('<H1>')
		self.expand (args)
		self.write ('</H1>\n')
	do_title = do_center
	do_subtitle = do_center
	do_author = do_center

	do_vskip = do_comment
	do_vfill = do_comment
	do_smallbook = do_comment

	do_paragraphindent = do_comment
	do_setchapternewpage = do_comment
	do_headings = do_comment
	do_footnotestyle = do_comment

	do_evenheading = do_comment
	do_evenfooting = do_comment
	do_oddheading = do_comment
	do_oddfooting = do_comment
	do_everyheading = do_comment
	do_everyfooting = do_comment

	# --- Nodes ---

	def do_node(self, args):
		self.endnode()
		self.nodelineno = 0
		parts = string.splitfields(args, ',')
		while len(parts) < 4: parts.append('')
		for i in range(4): parts[i] = string.strip(parts[i])
		self.nodelinks = parts
		[name, next, prev, up] = parts[:4]
		file = self.dirname + '/' + makefile(name)
		if self.filenames.has_key(file):
			print '*** Filename already in use: ', file
		else:
			if self.debugging: print '--- writing', file
		self.filenames[file] = 1
		self.nodefp = open(file, 'w')
		self.nodename = name
		if not self.topname: self.topname = name
		title = name
		if self.title: title = title + ' -- ' + self.title
		# No idea what this means, but this is what latex2html writes
		self.write('<!DOCTYPE HTML PUBLIC "-//W3O//DTD W3 HTML 2.0//EN">\n')
		self.write('<!- Converted with texi2html and Python>\n')
		self.write ('<P>\n<HEAD>\n')
		self.write('<TITLE>', title, '</TITLE>\n')
		self.write ('</HEAD>\n<BODY>\n<P>\n<BR> <HR>\n')
		self.link('Next', next)
		self.link('Prev', prev)
		self.link('Up', up)
		if self.nodename <> self.topname:
			self.link('Top', self.topname)
		self.write ('<BR> <HR> <P>\n')

	def link(self, label, nodename):
		if nodename:
			if string.lower(nodename) == '(dir)':
				addr = '../dir.html'
			else:
				addr = makefile(nodename)
			self.write(label, ': <A HREF="', addr, '" TYPE="', \
				label, '">', nodename, '</A>  \n')

	# --- Sectioning commands ---

	def do_chapter(self, args):
		self.heading('H1', args, 0)
	def do_unnumbered(self, args):
		self.heading('H1', args, -1)
	def do_appendix(self, args):
		self.heading('H1', args, -1)
	def do_top(self, args):
		self.heading('H1', args, -1)
	def do_chapheading(self, args):
		self.heading('H1', args, -1)
	def do_majorheading(self, args):
		self.heading('H1', args, -1)

	def do_section(self, args):
		self.heading('H1', args, 1)
	def do_unnumberedsec(self, args):
		self.heading('H1', args, -1)
	def do_appendixsec(self, args):
		self.heading('H1', args, -1)
	do_appendixsection = do_appendixsec
	def do_heading(self, args):
		self.heading('H1', args, -1)

	def do_subsection(self, args):
		self.heading('H2', args, 2)
	def do_unnumberedsubsec(self, args):
		self.heading('H2', args, -1)
	def do_appendixsubsec(self, args):
		self.heading('H2', args, -1)
	def do_subheading(self, args):
		self.heading('H2', args, -1)

	def do_subsubsection(self, args):
		self.heading('H3', args, 3)
	def do_unnumberedsubsubsec(self, args):
		self.heading('H3', args, -1)
	def do_appendixsubsubsec(self, args):
		self.heading('H3', args, -1)
	def do_subsubheading(self, args):
		self.heading('H3', args, -1)

	def heading(self, type, args, level):
		if level >= 0:
			while len(self.numbering) <= level:
				self.numbering.append(0)
			del self.numbering[level+1:]
			self.numbering[level] = self.numbering[level] + 1
			x = ''
			for i in self.numbering:
				x = x + `i` + '.'
			args = x + ' ' + args
			self.contents.append(level, args, self.nodename)
		self.write('<', type, '>')
		self.expand(args)
		self.write('</', type, '>\n')
		if self.debugging:
			print '---', args

	def do_contents(self, args):
		pass
		# self.listcontents('Table of Contents', 999)

	def do_shortcontents(self, args):
		pass
		# self.listcontents('Short Contents', 0)
	do_summarycontents = do_shortcontents

	def listcontents(self, title, maxlevel):
		self.write('<H1>', title, '</H1>\n<UL COMPACT>\n')
		for level, title, node in self.contents:
			if level <= maxlevel:
				self.write('<LI>', '.   '*level, '<A HREF="', \
					makefile(node), '">')
				self.expand(title)
				self.write('</A> ', node, '\n')
		self.write('</UL>\n')

	# --- Page lay-out ---

	# These commands are only meaningful in printed text

	def do_page(self, args): pass

	def do_need(self, args): pass

	def bgn_group(self, args): pass
	def end_group(self): pass

	# --- Line lay-out ---

	def do_sp(self, args):
		# Insert <args> blank lines
		if args:
			try:
				n = string.atoi(args)
			except string.atoi_error:
				n = 1
		else:
			n = 1
		self.write('<P>\n'*max(n, 0))

	def do_hline(self, args):
		self.write ('<HR>')
	
	# --- Function and variable definitions ---

	def bgn_deffn(self, args):
		self.write('<DL><DT>')
		words = splitwords(args, 2)
		[category, name], rest = words[:2], words[2:]
		self.expand('@b{' + name + '}')
		for word in rest: self.expand(' ' + makevar(word))
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('fn', name)

	def end_deffn(self):
		self.write('</DL>\n')

	def do_deffnx(self, args):
		self.write('<DT>')
		words = splitwords(args, 2)
		[category, name], rest = words[:2], words[2:]
		self.expand('@b{' + name + '}')
		for word in rest: self.expand(' ' + makevar(word))
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('fn', name)

	def bgn_defun(self, args): self.bgn_deffn('Function ' + args)
	end_defun = end_deffn

	def bgn_defmac(self, args): self.bgn_deffn('Macro ' + args)
	end_defmac = end_deffn

	def bgn_defspec(self, args): self.bgn_deffn('{Special Form} ' + args)
	end_defspec = end_deffn

	def bgn_defvr(self, args):
		self.write('<DL><DT>')
		words = splitwords(args, 2)
		[category, name], rest = words[:2], words[2:]
		self.expand('@code{' + name + '}')
		# If there are too many arguments, show them
		for word in rest: self.expand(' ' + word)
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('vr', name)

	end_defvr = end_deffn

	def bgn_defvar(self, args): self.bgn_defvr('Variable ' + args)
	end_defvar = end_defvr

	def bgn_defopt(self, args): self.bgn_defvr('{User Option} ' + args)
	end_defopt = end_defvr

	# --- Ditto for typed languages ---

	def bgn_deftypefn(self, args):
		self.write('<DL><DT>')
		words = splitwords(args, 3)
		[category, datatype, name], rest = words[:3], words[3:]
		self.expand('@code{' + datatype + '} @b{' + name + '}')
		for word in rest: self.expand(' ' + makevar(word))
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('fn', name)

	end_deftypefn = end_deffn

	def bgn_deftypefun(self, args): self.bgn_deftypefn('Function ' + args)
	end_deftypefun = end_deftypefn

	def bgn_deftypevr(self, args):
		words = splitwords(args, 3)
		[category, datatype, name], rest = words[:3], words[3:]
		self.write('<DL><DT>')
		self.expand('@code{' + datatype + '} @b{' + name + '}')
		# If there are too many arguments, show them
		for word in rest: self.expand(' ' + word)
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('fn', name)

	end_deftypevr = end_deftypefn

	def bgn_deftypevar(self, args):
		self.bgn_deftypevr('Variable ' + args)
	end_deftypevar = end_deftypevr

	# --- Ditto for object-oriented languages ---

	def bgn_defcv(self, args):
		words = splitwords(args, 3)
		[category, classname, name], rest = words[:3], words[3:]
		self.write('<DL><DT>')
		self.expand('@b{' + name + '}')
		# If there are too many arguments, show them
		for word in rest: self.expand(' ' + word)
		self.expand(' -- ' + category + ' of ' + classname)
		self.write('<DD>\n')
		self.index('vr', name + ' @r{of ' + classname + '}')

	end_defcv = end_deftypevr

	def bgn_defivar(self, args):
		self.bgn_defcv('{Instance Variable} ' + args)
	end_defivar = end_defcv

	def bgn_defop(self, args):
		self.write('<DL><DT>')
		words = splitwords(args, 3)
		[category, classname, name], rest = words[:3], words[3:]
		self.expand('@b{' + name + '}')
		for word in rest: self.expand(' ' + makevar(word))
		self.expand(' -- ' + category + ' on ' + classname)
		self.write('<DD>\n')
		self.index('fn', name + ' @r{on ' + classname + '}')

	end_defop = end_defcv

	def bgn_defmethod(self, args):
		self.bgn_defop('Method ' + args)
	end_defmethod = end_defop

	# --- Ditto for data types ---

	def bgn_deftp(self, args):
		self.write('<DL><DT>')
		words = splitwords(args, 2)
		[category, name], rest = words[:2], words[2:]
		self.expand('@b{' + name + '}')
		for word in rest: self.expand(' ' + word)
		self.expand(' -- ' + category)
		self.write('<DD>\n')
		self.index('tp', name)

	end_deftp = end_defcv

	# --- Making Lists and Tables

	def bgn_enumerate(self, args):
		if not args:
			self.write('<OL>\n')
			self.stackinfo[len(self.stack)] = '</OL>\n'
		else:
			self.itemnumber = args
			self.write('<UL>\n')
			self.stackinfo[len(self.stack)] = '</UL>\n'
	def end_enumerate(self):
		self.itemnumber = None
		self.write(self.stackinfo[len(self.stack) + 1])
		del self.stackinfo[len(self.stack) + 1]

	def bgn_itemize(self, args):
		self.itemarg = args
		self.write('<UL>\n')
	def end_itemize(self):
		self.itemarg = None
		self.write('</UL>\n')

	def bgn_table(self, args):
		self.itemarg = args
		self.write('<DL>\n')
	def end_table(self):
		self.itemarg = None
		self.write('</DL>\n')

	def bgn_ftable(self, args):
		self.itemindex = 'fn'
		self.bgn_table(args)
	def end_ftable(self):
		self.itemindex = None
		self.end_table()

	def do_item(self, args):
		if self.itemindex: self.index(self.itemindex, args)
		if self.itemarg:
			if self.itemarg[0] == '@' and self.itemarg[1:2] and \
					self.itemarg[1] in string.letters:
				args = self.itemarg + '{' + args + '}'
			else:
				# some other character, e.g. '-'
				args = self.itemarg + ' ' + args
		if self.itemnumber <> None:
			args = self.itemnumber + '. ' + args
			self.itemnumber = increment(self.itemnumber)
		if self.stack and self.stack[-1] == 'table':
			self.write('<DT>')
			self.expand(args)
			self.write('<DD>')
		else:
			self.write('<LI>')
			self.expand(args)
			self.write('  ')
	do_itemx = do_item # XXX Should suppress leading blank line

	# --- Enumerations, displays, quotations ---
	# XXX Most of these should increase the indentation somehow

	def bgn_quotation(self, args): self.write('<P>')
	def end_quotation(self): self.write('<P>\n')

	def bgn_example(self, args):
		self.nofill = self.nofill + 1
		self.write('<PRE><CODE>')
	def end_example(self):
		self.write('</CODE></PRE>')
		self.nofill = self.nofill - 1

	bgn_lisp = bgn_example # Synonym when contents are executable lisp code
	end_lisp = end_example

	bgn_smallexample = bgn_example # XXX Should use smaller font
	end_smallexample = end_example

	bgn_smalllisp = bgn_lisp # Ditto
	end_smalllisp = end_lisp

	def bgn_display(self, args):
		self.nofill = self.nofill + 1
		self.write('<PRE>\n')
	def end_display(self):
		self.write('</PRE>\n')
		self.nofill = self.nofill - 1

	def bgn_format(self, args):
		self.nofill = self.nofill + 1
		self.write('<PRE><CODE>\n')
	def end_format(self):
		self.write('</CODE></PRE>\n')
		self.nofill = self.nofill - 1

	def do_exdent(self, args): self.expand(args + '\n')
	# XXX Should really mess with indentation

	def bgn_flushleft(self, args):
		self.nofill = self.nofill + 1
		self.write('<PRE>\n')
	def end_flushleft(self):
		self.write('</PRE>\n')
		self.nofill = self.nofill - 1

	def bgn_flushright(self, args):
		self.nofill = self.nofill + 1
		self.write('<ADDRESS COMPACT>\n')
	def end_flushright(self):
		self.write('</ADDRESS>\n')
		self.nofill = self.nofill - 1

	def bgn_menu(self, args): self.write('<H2>Menu</H2><DL COMPACT>\n')
	def end_menu(self): self.write('</DL>\n')

	def bgn_cartouche(self, args): pass
	def end_cartouche(self): pass

	# --- Indices ---

	def resetindex(self):
		self.noncodeindices = ['cp']
		self.indextitle = {}
		self.indextitle['cp'] = 'Concept'
		self.indextitle['fn'] = 'Function'
		self.indextitle['ky'] = 'Keyword'
		self.indextitle['pg'] = 'Program'
		self.indextitle['tp'] = 'Type'
		self.indextitle['vr'] = 'Variable'
		#
		self.whichindex = {}
		for name in self.indextitle.keys():
			self.whichindex[name] = []

	def user_index(self, name, args):
		if self.whichindex.has_key(name):
			self.index(name, args)
		else:
			print '*** No index named', `name`

	def do_cindex(self, args): self.index('cp', args)
	def do_findex(self, args): self.index('fn', args)
	def do_kindex(self, args): self.index('ky', args)
	def do_pindex(self, args): self.index('pg', args)
	def do_tindex(self, args): self.index('tp', args)
	def do_vindex(self, args): self.index('vr', args)

	def index(self, name, args):
		self.whichindex[name].append(args, self.nodename)

	def do_synindex(self, args):
		words = string.split(args)
		if len(words) <> 2:
			print '*** bad @synindex', args
			return
		[old, new] = words
		if not self.whichindex.has_key(old) or \
			  not self.whichindex.has_key(new):
			print '*** bad key(s) in @synindex', args
			return
		if old <> new and \
			  self.whichindex[old] is not self.whichindex[new]:
			inew = self.whichindex[new]
			inew[len(inew):] = self.whichindex[old]
			self.whichindex[old] = inew
	do_syncodeindex = do_synindex # XXX Should use code font

	def do_printindex(self, args):
		words = string.split(args)
		for name in words:
			if self.whichindex.has_key(name):
				self.prindex(name)
			else:
				print '*** No index named', `name`

	def prindex(self, name):
		iscodeindex = (name not in self.noncodeindices)
		index = self.whichindex[name]
		if not index: return
		if self.debugging:
			print '--- Generating', self.indextitle[name], 'index'
		#  The node already provides a title
		index1 = []
		junkprog = regex.compile('^\(@[a-z]+\)?{')
		for key, node in index:
			sortkey = string.lower(key)
			# Remove leading `@cmd{' from sort key
			# -- don't bother about the matching `}'
			oldsortkey = sortkey
			while 1:
				i = junkprog.match(sortkey)
				if i < 0: break
				sortkey = sortkey[i:]
			index1.append(sortkey, key, node)
		del index[:]
		index1.sort()
		self.write('<DL COMPACT>\n')
		for sortkey, key, node in index1:
			if self.debugging > 1: print key, ':', node
			self.write('<DT>')
			if iscodeindex: key = '@code{' + key + '}'
			self.expand(key)
			self.write('<DD><A HREF="', makefile(node), \
				   '">', node, '</A>\n')
		self.write('</DL>\n')

	# --- Final error reports ---

	def report(self):
		if self.unknown:
			print '--- Unrecognized commands ---'
			cmds = self.unknown.keys()
			cmds.sort()
			for cmd in cmds:
				print string.ljust(cmd, 20), self.unknown[cmd]


# Put @var{} around alphabetic substrings
def makevar(str):
	return '@var{'+str+'}'


# Split a string in "words" according to findwordend
def splitwords(str, minlength):
	words = []
	i = 0
	n = len(str)
	while i < n:
		while i < n and str[i] in ' \t\n': i = i+1
		if i >= n: break
		start = i
		i = findwordend(str, i, n)
		words.append(str[start:i])
	while len(words) < minlength: words.append('')
	return words


# Find the end of a "word", matching braces and interpreting @@ @{ @}
fwprog = regex.compile('[@{} ]')
def findwordend(str, i, n):
	level = 0
	while i < n:
		i = fwprog.search(str, i)
		if i < 0: break
		c = str[i]; i = i+1
		if c == '@': i = i+1 # Next character is not special
		elif c == '{': level = level+1
		elif c == '}': level = level-1
		elif c == ' ' and level <= 0: return i-1
	return n


# Convert a node name into a file name
def makefile(nodename):
	return fixfunnychars(nodename) + '.html'


# Characters that are perfectly safe in filenames and hyperlinks
goodchars = string.letters + string.digits + '!@-_=+.'

# Replace characters that aren't perfectly safe by underscores
def fixfunnychars(addr):
	i = 0
	while i < len(addr):
		c = addr[i]
		if c not in goodchars:
			c = '_'
			addr = addr[:i] + c + addr[i+1:]
		i = i + len(c)
	return addr


# Increment a string used as an enumeration
def increment(s):
	if not s:
		return '1'
	for sequence in string.digits, string.lowercase, string.uppercase:
		lastc = s[-1]
		if lastc in sequence:
			i = string.index(sequence, lastc) + 1
			if i >= len(sequence):
				if len(s) == 1:
					s = sequence[0]*2
					if s == '00':
						s = '10'
				else:
					s = increment(s[:-1]) + sequence[0]
			else:
				s = s[:-1] + sequence[i]
			return s
	return s # Don't increment


def test():
	import sys
	parser = TexinfoParser()
	while sys.argv[1:2] == ['-d']:
		parser.debugging = parser.debugging + 1
		del sys.argv[1:2]
	if len(sys.argv) <> 3:
		print 'usage: texi2html [-d] [-d] inputfile outputdirectory'
		sys.exit(2)
	file = sys.argv[1]
	parser.setdirname(sys.argv[2])
	if file == '-':
		fp = sys.stdin
	else:
		parser.setincludedir(os.path.dirname(file))
		try:
			fp = open(file, 'r')
		except IOError, msg:
			print file, ':', msg
			sys.exit(1)
	parser.parse(fp)
	fp.close()
	parser.report()


test()
