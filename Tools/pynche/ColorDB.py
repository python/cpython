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
import re


# generic class
class ColorDB:
    def __init__(self, fp, lineno):
	# Maintain several dictionaries for indexing into the color database.
	# Note that while Tk supports RGB intensities of 4, 8, 12, or 16 bits, 
	# for now we only support 8 bit intensities.  At least on OpenWindows, 
	# all intensities in the /usr/openwin/lib/rgb.txt file are 8-bit
	#
	# key is rrggbb, value is (name, [aliases])
	self.__byrrggbb = {}
	#
	# key is name, value is (red, green, blue, rrggbb)
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
	    red, green, blue = map(int, mo.group('red', 'green', 'blue'))
	    name = mo.group('name')
	    #
	    # calculate the 24 bit representation of the color
	    rrggbb = (red << 16) + (blue << 8) + green
	    #
	    # TBD: for now the `name' is just the first named color with the
	    # rgb values we find.  Later, we might want to make the two word
	    # version the `name', or the CapitalizedVersion, etc.
	    foundname, aliases = self.__byrrggbb.get(rrggbb, (name, []))
	    if foundname <> name and foundname not in aliases:
		aliases.append(name)
	    #
	    # add to by 24bit value
	    self.__byrrggbb[rrggbb] = (foundname, aliases)
	    #
	    # add to byname lookup
	    point = (red, green, blue, rrggbb)
	    self.__byname[name] = point
	    lineno = lineno + 1

    def find(self, red, green, blue):
	rrggbb = (red << 16) + (blue << 8) + green
	return self.__byrrggbb.get(rrggbb, (None, []))

    def find_byname(self, name):
	# TBD: is the unfound value right?
	return self.__byname.get(name, (0, 0, 0, 0))

    def nearest(self, red, green, blue):
	# TBD: use Voronoi diagrams, Delaunay triangulation, or octree for
	# speeding up the locating of nearest point.  This is really
	# inefficient!
	nearest = -1
	nearest_name = ''
	for name, (r, g, b, rrggbb) in self.__byname.items():
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
    return colordb


if __name__ == '__main__':
    import string

    colordb = get_colordb('/usr/openwin/lib/rgb.txt')
    if not colordb:
	print 'No parseable color database found'
	sys.exit(1)
    # on my system, this color matches exactly
    target = 'navy'
    target = 'snow'
    red, green, blue, rrggbb = colordb.find_byname(target)
    print target, ':', red, green, blue, hex(rrggbb)
    name, aliases = colordb.find(red, green, blue)
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
