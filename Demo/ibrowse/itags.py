# Utility module for 'icache.py': interpret tag tables and indirect nodes.

# (This module is a bit chatty when confronted with the unexpected.)


import regexp
import string
import ifile


# Get the tag table of an open file, as a dictionary.
# Seeks around in the file; after reading, the position is undefined.
# Return an empty tag table if none is found.
#
def get_tags(f):
	#
	# First see if the last "node" is the end of tag table marker.
	#
	f.seek(0, 2) # Seek to EOF
	end = f.tell()
	buf = ifile.backup_node(f, end)
	if not labelmatch(buf, 0, 'end tag table\n'):
		return {} # No succes
	#
	# Next backup to the previous "node" -- the tag table itself.
	#
	###print 'Getting prebuilt tag table...'
	end = f.tell() - len(buf)
	buf = ifile.backup_node(f, end)
	label = 'tag table:\n'
	if not labelmatch(buf, 0, label):
		print 'Weird: end tag table marker but no tag table?'
		print 'Node begins:', `buf[:50]`
		return {}
	#
	# Now read the whole tag table.
	#
	end = f.tell() - len(buf) # Do this first!
	buf = ifile.read_node(f, buf)
	#
	# First check for an indirection table.
	#
	indirlist = []
	if labelmatch(buf, len(label), '(indirect)\n'):
		indirbuf = ifile.backup_node(f, end)
		if not labelmatch(indirbuf, 0, 'indirect:\n'):
			print 'Weird: promised indirection table not found'
			print 'Node begins:', `indirbuf[:50]`
			# Carry on.  Things probably won't work though.
		else:
			indirbuf = ifile.read_node(f, indirbuf)
			indirlist = parse_indirlist(indirbuf)
	#
	# Now parse the tag table.
	#
	findtag = regexp.compile('^(.*[nN]ode:[ \t]*(.*))\177([0-9]+)$').match
	i = 0
	tags = {}
	while 1:
		match = findtag(buf, i)
		if not match:
			break
		(a,b), (a1,b1), (a2,b2), (a3,b3) = match
		i = b
		line = buf[a1:b1]
		node = string.lower(buf[a2:b2])
		offset = eval(buf[a3:b3]) # XXX What if it overflows?
		if tags.has_key(node):
			print 'Duplicate key in tag table:', `node`
		file, offset = map_offset(offset, indirlist)
		tags[node] = file, offset, line
	#
	return tags


# Return true if buf[i:] begins with a label, after lower case conversion.
# The label argument must be in lower case.
#
def labelmatch(buf, i, label):
	return string.lower(buf[i:i+len(label)]) == label


# Parse the indirection list.
# Return a list of (filename, offset) pairs ready for use.
#
def parse_indirlist(buf):
	list = []
	findindir = regexp.compile('^(.+):[ \t]*([0-9]+)$').match
	i = 0
	while 1:
		match = findindir(buf, i)
		if not match:
			break
		(a,b), (a1,b1), (a2,b2) = match
		file = buf[a1:b1]
		offset = eval(buf[a2:b2]) # XXX What if this gets overflow?
		list.append(file, offset)
		i = b
	return list


# Map an offset through the indirection list.
# Return (filename, new_offset).
# If the list is empty, return the given offset and an empty file name.
#
def map_offset(offset, indirlist):
	if not indirlist:
		return '', offset
	#
	# XXX This could be done more elegant.
	#
	filex, offx = indirlist[0]
	for i in range(len(indirlist)):
		file1, off1 = indirlist[i]
		if i+1 >= len(indirlist):
			file2, off2 = '', 0x7fffffff
		else:
			file2, off2 = indirlist[i+1]
		if off1 <= offset < off2:
			# Add offx+2 to compensate for extra header.
			# No idea whether this is always correct.
			return file1, offset-off1 + offx+2
	#
	# XXX Shouldn't get here.
	#
	print 'Oops, map_offset fell through'
	return '', offset # Not likely to get good results
