# A parser for SGML, using the derived class as static DTD.

# XXX This only supports those SGML features used by HTML.

# XXX There should be a way to distinguish between PCDATA (parsed
# character data -- the normal case), RCDATA (replaceable character
# data -- only char and entity references and end tags are special)
# and CDATA (character data -- only end tags are special).


import regex
import string


# Regular expressions used for parsing

incomplete = regex.compile(
	  '<!-?\|</[a-zA-Z][a-zA-Z0-9]*[ \t\n]*\|</?\|' +
	  '&#[a-zA-Z0-9]*\|&[a-zA-Z][a-zA-Z0-9]*\|&')
entityref = regex.compile('&[a-zA-Z][a-zA-Z0-9]*[;.]')
charref = regex.compile('&#[a-zA-Z0-9]+;')
starttagopen = regex.compile('<[a-zA-Z]')
endtag = regex.compile('</[a-zA-Z][a-zA-Z0-9]*[ \t\n]*>')
commentopen = regex.compile('<!--')


# SGML parser base class -- find tags and call handler functions.
# Usage: p = SGMLParser(); p.feed(data); ...; p.close().
# The dtd is defined by deriving a class which defines methods
# with special names to handle tags: start_foo and end_foo to handle
# <foo> and </foo>, respectively, or do_foo to handle <foo> by itself.
# (Tags are converted to lower case for this purpose.)  The data
# between tags is passed to the parser by calling self.handle_data()
# with some data as argument (the data may be split up in arbutrary
# chunks).  Entity references are passed by calling
# self.handle_entityref() with the entity reference as argument.

class SGMLParser:

	# Interface -- initialize and reset this instance
	def __init__(self):
		self.reset()

	# Interface -- reset this instance.  Loses all unprocessed data
	def reset(self):
		self.rawdata = ''
		self.stack = []
		self.nomoretags = 0
		self.literal = 0

	# For derived classes only -- enter literal mode (CDATA) till EOF
	def setnomoretags(self):
		self.nomoretags = self.literal = 1

	# For derived classes only -- enter literal mode (CDATA)
	def setliteral(self, *args):
		self.literal = 1

	# Interface -- feed some data to the parser.  Call this as
	# often as you want, with as little or as much text as you
	# want (may include '\n').  (This just saves the text, all the
	# processing is done by goahead().)
	def feed(self, data):
		self.rawdata = self.rawdata + data
		self.goahead(0)

	# Interface -- handle the remaining data
	def close(self):
		self.goahead(1)

	# Internal -- handle data as far as reasonable.  May leave state
	# and data to be processed by a subsequent call.  If 'end' is
	# true, force handling all data as if followed by EOF marker.
	def goahead(self, end):
		rawdata = self.rawdata
		i = 0
		n = len(rawdata)
		while i < n:
			if self.nomoretags:
				self.handle_data(rawdata[i:n])
				i = n
				break
			j = incomplete.search(rawdata, i)
			if j < 0: j = n
			if i < j: self.handle_data(rawdata[i:j])
			i = j
			if i == n: break
			if rawdata[i] == '<':
				if starttagopen.match(rawdata, i) >= 0:
					if self.literal:
						self.handle_data(rawdata[i])
						i = i+1
						continue
					k = self.parse_starttag(i)
					if k < 0: break
					i = i + k
					continue
				k = endtag.match(rawdata, i)
				if k >= 0:
					j = i+k
					self.parse_endtag(rawdata[i:j])
					i = j
					self.literal = 0
					continue
				if commentopen.match(rawdata, i) >= 0:
					if self.literal:
						self.handle_data(rawdata[i])
						i = i+1
						continue
					k = self.parse_comment(i)
					if k < 0: break
					i = i+k
					continue
			elif rawdata[i] == '&':
				k = charref.match(rawdata, i)
				if k >= 0:
					j = i+k
					self.handle_charref(rawdata[i+2:j-1])
					i = j
					continue
				k = entityref.match(rawdata, i)
				if k >= 0:
					j = i+k
					self.handle_entityref(rawdata[i+1:j-1])
					i = j
					continue
			else:
				raise RuntimeError, 'neither < nor & ??'
			# We get here only if incomplete matches but
			# nothing else
			k = incomplete.match(rawdata, i)
			if k < 0: raise RuntimeError, 'no incomplete match ??'
			j = i+k
			if j == n: break # Really incomplete
			self.handle_data(rawdata[i:j])
			i = j
		# end while
		if end and i < n:
			self.handle_data(rawdata[i:n])
			i = n
		self.rawdata = rawdata[i:]
		# XXX if end: check for empty stack

	# Internal -- parse comment, return length or -1 if not ternimated
	def parse_comment(self, i):
		rawdata = self.rawdata
		if rawdata[i:i+4] <> '<!--':
			raise RuntimeError, 'unexpected call to handle_comment'
		try:
			j = string.index(rawdata, '--', i+4)
		except string.index_error:
			return -1
		self.handle_comment(rawdata[i+4: j])
		j = j+2
		n = len(rawdata)
		while j < n and rawdata[j] in ' \t\n': j = j+1
		if j == n: return -1 # Wait for final '>'
		if rawdata[j] == '>':
			j = j+1
		else:
			print '*** comment not terminated with >'
			print repr(rawdata[j-5:j]), '*!*', repr(rawdata[j:j+5])
		return j-i

	# Internal -- handle starttag, return length or -1 if not terminated
	def parse_starttag(self, i):
		rawdata = self.rawdata
		try:
			j = string.index(rawdata, '>', i)
		except string.index_error:
			return -1
		# Now parse the data between i+1 and j into a tag and attrs
		attrs = []
		tagfind = regex.compile('[a-zA-Z][a-zA-Z0-9]*')
		attrfind = regex.compile(
		  '[ \t\n]+\([a-zA-Z][a-zA-Z0-9]*\)' +
		  '\([ \t\n]*=[ \t\n]*' +
		     '\(\'[^\']*\';\|"[^"]*"\|[-a-zA-Z0-9./:+*%?!()_#]+\)\)?')
		k = tagfind.match(rawdata, i+1)
		if k < 0:
			raise RuntimeError, 'unexpected call to parse_starttag'
		k = i+1+k
		tag = string.lower(rawdata[i+1:k])
		while k < j:
			l = attrfind.match(rawdata, k)
			if l < 0: break
			regs = attrfind.regs
			a1, b1 = regs[1]
			a2, b2 = regs[2]
			a3, b3 = regs[3]
			attrname = rawdata[a1:b1]
			if '=' in rawdata[k:k+l]:
				attrvalue = rawdata[a3:b3]
				if attrvalue[:1] == '\'' == attrvalue[-1:] or \
				   attrvalue[:1] == '"' == attrvalue[-1:]:
					attrvalue = attrvalue[1:-1]
			else:
				attrvalue = ''
			attrs.append((string.lower(attrname), attrvalue))
			k = k + l
		j = j+1
		try:
			method = getattr(self, 'start_' + tag)
		except AttributeError:
			try:
				method = getattr(self, 'do_' + tag)
			except AttributeError:
				self.unknown_starttag(tag, attrs)
				return j-i
			method(attrs)
			return j-i
		self.stack.append(tag)
		method(attrs)
		return j-i

	# Internal -- parse endtag
	def parse_endtag(self, data):
		if data[:2] <> '</' or data[-1:] <> '>':
			raise RuntimeError, 'unexpected call to parse_endtag'
		tag = string.lower(string.strip(data[2:-1]))
		try:
			method = getattr(self, 'end_' + tag)
		except AttributeError:
			self.unknown_endtag(tag)
			return
		if self.stack and self.stack[-1] == tag:
			del self.stack[-1]
		else:
			self.report_unbalanced(tag)
			# Now repair it
			found = None
			for i in range(len(self.stack)):
				if self.stack[i] == tag: found = i
			if found <> None:
				del self.stack[found:]
		method()

	# Example -- report an unbalanced </...> tag.
	def report_unbalanced(self, tag):
		print '*** Unbalanced </' + tag + '>'
		print '*** Stack:', self.stack

	# Example -- handle character reference, no need to override
	def handle_charref(self, name):
		try:
			n = string.atoi(name)
		except string.atoi_error:
			self.unknown_charref(name)
			return
		if not 0 <= n <= 255:
			self.unknown_charref(name)
			return
		self.handle_data(chr(n))

	# Definition of entities -- derived classes may override
	entitydefs = \
		{'lt': '<', 'gt': '>', 'amp': '&', 'quot': '"', 'apos': '\''}

	# Example -- handle entity reference, no need to override
	def handle_entityref(self, name):
		table = self.entitydefs
		name = string.lower(name)
		if table.has_key(name):
			self.handle_data(table[name])
		else:
			self.unknown_entityref(name)
			return

	# Example -- handle data, should be overridden
	def handle_data(self, data):
		pass

	# Example -- handle comment, could be overridden
	def handle_comment(self, data):
		pass

	# To be overridden -- handlers for unknown objects
	def unknown_starttag(self, tag, attrs): pass
	def unknown_endtag(self, tag): pass
	def unknown_charref(self, ref): pass
	def unknown_entityref(self, ref): pass


class TestSGML(SGMLParser):

	def handle_data(self, data):
		r = repr(data)
		if len(r) > 72:
			r = r[:35] + '...' + r[-35:]
		print 'data:', r

	def handle_comment(self, data):
		r = repr(data)
		if len(r) > 68:
			r = r[:32] + '...' + r[-32:]
		print 'comment:', r

	def unknown_starttag(self, tag, attrs):
		print 'start tag: <' + tag,
		for name, value in attrs:
			print name + '=' + '"' + value + '"',
		print '>'

	def unknown_endtag(self, tag):
		print 'end tag: </' + tag + '>'

	def unknown_entityref(self, ref):
		print '*** unknown entity ref: &' + ref + ';'

	def unknown_charref(self, ref):
		print '*** unknown char ref: &#' + ref + ';'


def test():
	file = 'test.html'
	f = open(file, 'r')
	x = TestSGML()
	while 1:
		line = f.readline()
		if not line:
			x.close()
			break
		x.feed(line)


if __name__ == '__main__':
	test()
