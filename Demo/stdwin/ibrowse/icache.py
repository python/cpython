# Cache management for info file processing.
# The function get_node() is the standard interface;
# its signature is the same as ifile.get_node() but it uses
# the cache and supports indirect tag tables.


import string
import ifile
from ifile import NoSuchNode, NoSuchFile
import itags


# Special hack to save the cache when using reload().
# This can just be "cache = {}" in a production version.
#
try:
	dummy = cache
	del dummy
except NameError:
	cache = {}


# Clear the entire cache.
#
def resetcache():
	for key in cache.keys():
		del cache[key]


# Clear the node info from the cache (the most voluminous data).
#
def resetnodecache():
	for key in cache.keys():
		tags, nodes = cache[key]
		cache[key] = tags, {}


# Get a node.
#
def get_node(curfile, ref):
	file, node = ifile.parse_ref(curfile, ref)
	file = string.lower(file)
	node = string.lower(node)
	if node == '*':
		# Don't cache whole file references;
		# reading the data is faster than displaying it anyway.
		return ifile.get_whole_file(file) # May raise NoSuchFile
	if not cache.has_key(file):
		cache[file] = get_tags(file), {} # May raise NoSuchFile
	tags, nodes = cache[file]
	if not nodes.has_key(node):
		if not tags.has_key(node):
			raise NoSuchNode, ref
		file1, offset, line = tags[node]
		if not file1:
			file1 = file
		file1, node1, header, menu, footnotes, text = \
			ifile.get_file_node(file1, offset, node)
		nodes[node] = file, node1, header, menu, footnotes, text
	return nodes[node]


# Get the tag table for a file.
# Either construct one or get the one found in the file.
# Raise NoSuchFile if the file isn't found.
#
def get_tags(file):
	f = ifile.try_open(file) # May raise NoSuchFile
	tags = itags.get_tags(f)
	if not tags:
		###print 'Scanning file...'
		f.seek(0)
		tags = ifile.make_tags(f)
	return tags
