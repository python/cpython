# Module 'util' -- some useful functions that don't fit elsewhere

# NB: These are now built-in functions, but this module is provided
# for compatibility.  Don't use in new programs unless you need backward
# compatibility (i.e. need to run with old interpreters).


# Remove an item from a list.
# No complaints if it isn't in the list at all.
# If it occurs more than once, remove the first occurrence.
#
def remove(item, list):
	if item in list: list.remove(item)


# Return a string containing a file's contents.
#
def readfile(fn):
	return readopenfile(open(fn, 'r'))


# Read an open file until EOF.
#
def readopenfile(fp):
	return fp.read()
