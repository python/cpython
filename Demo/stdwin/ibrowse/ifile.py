# Tools for info file processing.

# XXX Need to be more careful with reading ahead searching for nodes.


import regexp
import string


# Exported exceptions.
#
NoSuchFile = 'no such file'
NoSuchNode = 'no such node'


# The search path for info files; this is site-specific.
# Directory names should end in a partname delimiter,
# so they can simply be concatenated to a relative pathname.
#
#INFOPATH = ['', ':Info.Ibrowse:', ':Info:']	# Mac
INFOPATH = ['', '/usr/local/emacs/info/']	# X11 on UNIX


# Tunable constants.
#
BLOCKSIZE = 512			# Qty to align reads to, if possible
FUZZ = 2*BLOCKSIZE		# Qty to back-up before searching for a node
CHUNKSIZE = 4*BLOCKSIZE		# Qty to read at once when reading lots of data


# Regular expressions used.
# Note that it is essential that Python leaves unrecognized backslash
# escapes in a string so they can be seen by regexp.compile!
#
findheader = regexp.compile('\037\014?\n(.*\n)').match
findescape = regexp.compile('\037').match
parseheader = regexp.compile('[nN]ode:[ \t]*([^\t,\n]*)').match
findfirstline = regexp.compile('^.*\n').match
findnode = regexp.compile('[nN]ode:[ \t]*([^\t,\n]*)').match
findprev = regexp.compile('[pP]rev[ious]*:[ \t]*([^\t,\n]*)').match
findnext = regexp.compile('[nN]ext:[ \t]*([^\t,\n]*)').match
findup = regexp.compile('[uU]p:[ \t]*([^\t,\n]*)').match
findmenu = regexp.compile('^\* [mM]enu:').match
findmenuitem = regexp.compile( \
	'^\* ([^:]+):[ \t]*(:|\([^\t]*\)[^\t,\n.]*|[^:(][^\t,\n.]*)').match
findfootnote = regexp.compile( \
	'\*[nN]ote ([^:]+):[ \t]*(:|[^:][^\t,\n.]*)').match
parsenoderef = regexp.compile('^\((.*)\)(.*)$').match


# Get a node and all information pertaining to it.
# This doesn't work if there is an indirect tag table,
# and in general you are better off using icache.get_node() instead.
# Functions get_whole_file() and get_file_node() provide part
# functionality used by icache.
# Raise NoSuchFile or NoSuchNode as appropriate.
#
def get_node(curfile, ref):
	file, node = parse_ref(curfile, ref)
	if node == '*':
		return get_whole_file(file)
	else:
		return get_file_node(file, 0, node)
#
def get_whole_file(file):
	f = try_open(file) # May raise NoSuchFile
	text = f.read()
	header, menu, footnotes = ('', '', ''), [], []
	return file, '*', header, menu, footnotes, text
#
def get_file_node(file, offset, node):
	f = try_open(file) # May raise NoSuchFile
	text = find_node(f, offset, node) # May raise NoSuchNode
	node, header, menu, footnotes = analyze_node(text)
	return file, node, header, menu, footnotes, text


# Parse a node reference into a file (possibly default) and node name.
# Possible reference formats are: "NODE", "(FILE)", "(FILE)NODE".
# Default file is the curfile argument; default node is Top.
# A node value of '*' is a special case: the whole file should
# be interpreted (by the caller!) as a single node.
#
def parse_ref(curfile, ref):
	match = parsenoderef(ref)
	if not match:
		file, node = curfile, ref
	else:
		(a, b), (a1, b1), (a2, b2) = match
		file, node = ref[a1:b1], ref[a2:b2]
	if not file:
		file = curfile # (Is this necessary?)
	if not node:
		node = 'Top'
	return file, node


# Extract node name, links, menu and footnotes from the node text.
#
def analyze_node(text):
	#
	# Get node name and links from the header line
	#
	match = findfirstline(text)
	if match:
		(a, b) = match[0]
		line = text[a:b]
	else:
		line = ''
	node = get_it(text, findnode)
	prev = get_it(text, findprev)
	next = get_it(text, findnext)
	up = get_it(text, findup)
	#
	# Get the menu items, if there is a menu
	#
	menu = []
	match = findmenu(text)
	if match:
		(a, b) = match[0]
		while 1:
			match = findmenuitem(text, b)
			if not match:
				break
			(a, b), (a1, b1), (a2, b2) = match
			topic, ref = text[a1:b1], text[a2:b2]
			if ref == ':':
				ref = topic
			menu.append(topic, ref)
	#
	# Get the footnotes
	#
	footnotes = []
	b = 0
	while 1:
		match = findfootnote(text, b)
		if not match:
			break
		(a, b), (a1, b1), (a2, b2) = match
		topic, ref = text[a1:b1], text[a2:b2]
		if ref == ':':
			ref = topic
		footnotes.append(topic, ref)
	#
	return node, (prev, next, up), menu, footnotes
#
def get_it(line, matcher):
	match = matcher(line)
	if not match:
		return ''
	else:
		(a, b), (a1, b1) = match
		return line[a1:b1]


# Find a node in an open file.
# The offset (from the tags table) is a hint about the node's position.
# Pass zero if there is no tags table.
# Raise NoSuchNode if the node isn't found.
# NB: This seeks around in the file.
#
def find_node(f, offset, node):
	node = string.lower(node) # Just to be sure
	#
	# Position a little before the given offset,
	# so we may find the node even if it has moved around
	# in the file a little.
	#
	offset = max(0, ((offset-FUZZ) / BLOCKSIZE) * BLOCKSIZE)
	f.seek(offset)
	#
	# Loop, hunting for a matching node header.
	#
	while 1:
		buf = f.read(CHUNKSIZE)
		if not buf:
			break
		i = 0
		while 1:
			match = findheader(buf, i)
			if match:
				(a,b), (a1,b1) = match
				start = a1
				line = buf[a1:b1]
				i = b
				match = parseheader(line)
				if match:
					(a,b), (a1,b1) = match
					key = string.lower(line[a1:b1])
					if key == node:
						# Got it!  Now read the rest.
						return read_node(f, buf[start:])
			elif findescape(buf, i):
				next = f.read(CHUNKSIZE)
				if not next:
					break
				buf = buf + next
			else:
				break
	#
	# If we get here, we didn't find it.  Too bad.
	#
	raise NoSuchNode, node


# Finish off getting a node (subroutine for find_node()).
# The node begins at the start of buf and may end in buf;
# if it doesn't end there, read additional data from f.
#
def read_node(f, buf):
	i = 0
	match = findescape(buf, i)
	while not match:
		next = f.read(CHUNKSIZE)
		if not next:
			end = len(buf)
			break
		i = len(buf)
		buf = buf + next
		match = findescape(buf, i)
	else:
		# Got a match
		(a, b) = match[0]
		end = a
	# Strip trailing newlines
	while end > 0 and buf[end-1] == '\n':
		end = end-1
	buf = buf[:end]
	return buf


# Read reverse starting at offset until the beginning of a node is found.
# Then return a buffer containing the beginning of the node,
# with f positioned just after the buffer.
# The buffer will contain at least the full header line of the node;
# the caller should finish off with read_node() if it is the right node.
# (It is also possible that the buffer extends beyond the node!)
# Return an empty string if there is no node before the given offset.
#
def backup_node(f, offset):
	start = max(0, ((offset-CHUNKSIZE) / BLOCKSIZE) * BLOCKSIZE)
	end = offset
	while start < end:
		f.seek(start)
		buf = f.read(end-start)
		i = 0
		hit = -1
		while 1:
			match = findheader(buf, i)
			if match:
				(a,b), (a1,b1) = match
				hit = a1
				i = b
			elif end < offset and findescape(buf, i):
				next = f.read(min(offset-end, BLOCKSIZE))
				if not next:
					break
				buf = buf + next
				end = end + len(next)
			else:
				break
		if hit >= 0:
			return buf[hit:]
		end = start
		start = max(0, end - CHUNKSIZE)
	return ''


# Make a tag table for the given file by scanning the file.
# The file must be open for reading, and positioned at the beginning
# (or wherever the hunt for tags must begin; it is read till the end).
#
def make_tags(f):
	tags = {}
	while 1:
		offset = f.tell()
		buf = f.read(CHUNKSIZE)
		if not buf:
			break
		i = 0
		while 1:
			match = findheader(buf, i)
			if match:
				(a,b), (a1,b1) = match
				start = offset+a1
				line = buf[a1:b1]
				i = b
				match = parseheader(line)
				if match:
					(a,b), (a1,b1) = match
					key = string.lower(line[a1:b1])
					if tags.has_key(key):
						print 'Duplicate node:',
						print key
					tags[key] = '', start, line
			elif findescape(buf, i):
				next = f.read(CHUNKSIZE)
				if not next:
					break
				buf = buf + next
			else:
				break
	return tags


# Try to open a file, return a file object if succeeds.
# Raise NoSuchFile if the file can't be opened.
# Should treat absolute pathnames special.
#
def try_open(file):
	for dir in INFOPATH:
		try:
			return open(dir + file, 'r')
		except IOError:
			pass
	raise NoSuchFile, file


# A little test for the speed of make_tags().
#
TESTFILE = 'texinfo-1'
def test_make_tags():
	import time
	f = try_open(TESTFILE)
	t1 = time.millitimer()
	tags = make_tags(f)
	t2 = time.millitimer()
	print 'Making tag table for', `TESTFILE`, 'took', t2-t1, 'msec.'
