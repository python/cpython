"""Color Database.

To create a class that contains color lookup methods, use the module global
function `get_colordb(file)'.  This function will try to examine the file to
figure out what the format of the file is.  If it can't figure out the file
format, or it has trouble reading the file, None is returned.  You can pass
get_colordb() an optional filetype argument.

Supporte file types are:

    X_RGB_TXT -- X Consortium rgb.txt format files.  Three columns of numbers
                 from 0 .. 255 separated by whitespace.  Arbitrary trailing
                 columns used as the color name.
"""

import sys
import string
import re
from types import *

class BadColor(Exception):
    pass

DEFAULT_DB = None


# generic class
class ColorDB:
    def __init__(self, fp, lineno):
	# Maintain several dictionaries for indexing into the color database.
	# Note that while Tk supports RGB intensities of 4, 8, 12, or 16 bits, 
	# for now we only support 8 bit intensities.  At least on OpenWindows, 
	# all intensities in the /usr/openwin/lib/rgb.txt file are 8-bit
	#
	# key is (red, green, blue) tuple, value is (name, [aliases])
	self.__byrgb = {}
	#
	# key is name, value is (red, green, blue)
	self.__byname = {}
	#
	while 1:
	    line = fp.readline()
	    if not line:
		break
	    # get this compiled regular expression from derived class
	    mo = self._re.match(line)
	    if not mo:
		sys.stderr.write('Error in %s, line %d\n' % (fp.name, lineno))
		lineno = lineno + 1
		continue
	    #
	    # extract the red, green, blue, and name
	    #
	    red, green, blue = map(int, mo.group('red', 'green', 'blue'))
	    name = mo.group('name')
	    keyname = string.lower(name)
	    #
	    # TBD: for now the `name' is just the first named color with the
	    # rgb values we find.  Later, we might want to make the two word
	    # version the `name', or the CapitalizedVersion, etc.
	    #
	    key = (red, green, blue)
	    foundname, aliases = self.__byrgb.get(key, (name, []))
	    if foundname <> name and foundname not in aliases:
		aliases.append(name)
	    self.__byrgb[key] = (foundname, aliases)
	    #
	    # add to byname lookup
	    #
	    self.__byname[keyname] = key
	    lineno = lineno + 1

    def find_byrgb(self, rgbtuple):
	try:
	    return self.__byrgb[rgbtuple]
	except KeyError:
	    raise BadColor(rgbtuple)

    def find_byname(self, name):
	name = string.lower(name)
	try:
	    return self.__byname[name]
	except KeyError:
	    raise BadColor(name)

    def nearest(self, rgbtuple):
	# TBD: use Voronoi diagrams, Delaunay triangulation, or octree for
	# speeding up the locating of nearest point.  Exhaustive search is
	# inefficient, but may be fast enough.
	red, green, blue = rgbtuple
	nearest = -1
	nearest_name = ''
	for name, aliases in self.__byrgb.values():
	    r, g, b = self.__byname[string.lower(name)]
	    rdelta = red - r
	    gdelta = green - g
	    bdelta = blue - b
	    distance = rdelta * rdelta + gdelta * gdelta + bdelta * bdelta
	    if nearest == -1 or distance < nearest:
		nearest = distance
		nearest_name = name
	return nearest_name
	

class RGBColorDB(ColorDB):
    _re = re.compile(
	'\s*(?P<red>\d+)\s+(?P<green>\d+)\s+(?P<blue>\d+)\s+(?P<name>.*)')



# format is a tuple (RE, SCANLINES, CLASS) where RE is a compiled regular
# expression, SCANLINES is the number of header lines to scan, and CLASS is
# the class to instantiate if a match is found

X_RGB_TXT = re.compile('XConsortium'), 1, RGBColorDB

def get_colordb(file, filetype=X_RGB_TXT):
    colordb = None
    fp = None
    typere, scanlines, class_ = filetype
    try:
	try:
	    lineno = 0
	    fp = open(file)
	    while lineno < scanlines:
		line = fp.readline()
		if not line:
		    break
		mo = typere.search(line)
		if mo:
		    colordb = class_(fp, lineno)
		    break
		lineno = lineno + 1
	except IOError:
	    pass
    finally:
	if fp:
	    fp.close()
    # save a global copy
    global DEFAULT_DB
    DEFAULT_DB = colordb
    return colordb



def rrggbb_to_triplet(color):
    """Converts a #rrggbb color to the tuple (red, green, blue)."""
    if color[0] <> '#':
	raise BadColor(color)

    red = color[1:3]
    green = color[3:5]
    blue = color[5:7]
    return tuple(map(lambda v: string.atoi(v, 16), (red, green, blue)))


def triplet_to_rrggbb(rgbtuple):
    """Converts a (red, green, blue) tuple to #rrggbb."""
    def hexify(v):
	hexstr = hex(v)[2:4]
	if len(hexstr) < 2:
	    hexstr = '0' + hexstr
	return hexstr
    return '#%s%s%s' % tuple(map(hexify, rgbtuple))


def triplet_to_pmwrgb(rgbtuple, MAX=256.0):
    r, g, b = rgbtuple
    return r/MAX, g/MAX, b/MAX



if __name__ == '__main__':
    import string

    colordb = get_colordb('/usr/openwin/lib/rgb.txt')
    if not colordb:
	print 'No parseable color database found'
	sys.exit(1)
    # on my system, this color matches exactly
    target = 'navy'
    target = 'snow'
    red, green, blue = colordb.find_byname(target)
    print target, ':', red, green, blue, hex(rrggbb)
    name, aliases = colordb.find_byrgb((red, green, blue))
    print 'name:', name, 'aliases:', string.join(aliases, ", ")
    target = (1, 1, 128)			  # nearest to navy
    target = (145, 238, 144)			  # nearest to lightgreen
    target = (255, 251, 250)			  # snow
    print 'finding nearest to', target, '...'
    import time
    t0 = time.time()
    nearest = apply(colordb.nearest, target)
    t1 = time.time()
    print 'found nearest color', nearest, 'in', t1-t0, 'seconds'

