# Module 'util' -- some useful functions that don't fit elsewhere


# Remove an item from a list.
# No complaints if it isn't in the list at all.
# If it occurs more than once, remove the first occurrence.
#
def remove(item, list):
	for i in range(len(list)):
		if list[i] = item:
			del list[i]
			break


# Return a string containing a file's contents.
#
def readfile(fn):
	return readopenfile(open(fn, 'r'))


# Read an open file until EOF.
#
def readopenfile(fp):
	BUFSIZE = 512*8
	data = ''
	while 1:
		buf = fp.read(BUFSIZE)
		if not buf: break
		data = data + buf
	return data
