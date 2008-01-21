"""Color Database.

This file contains one class, called ColorDB, and several utility functions.
The class must be instantiated by the get_colordb() function in this file,
passing it a filename to read a database out of.

The get_colordb() function will try to examine the file to figure out what the
format of the file is.  If it can't figure out the file format, or it has
trouble reading the file, None is returned.  You can pass get_colordb() an
optional filetype argument.

Supporte file types are:

    X_RGB_TXT -- X Consortium rgb.txt format files.  Three columns of numbers
                 from 0 .. 255 separated by whitespace.  Arbitrary trailing
                 columns used as the color name.

The utility functions are useful for converting between the various expected
color formats, and for calculating other color values.

"""

import sys
import re
from types import *
import operator

class BadColor(Exception):
    pass

DEFAULT_DB = None
SPACE = ' '
COMMASPACE = ', '



# generic class
class ColorDB:
    def __init__(self, fp):
        lineno = 2
        self.__name = fp.name
        # Maintain several dictionaries for indexing into the color database.
        # Note that while Tk supports RGB intensities of 4, 8, 12, or 16 bits,
        # for now we only support 8 bit intensities.  At least on OpenWindows,
        # all intensities in the /usr/openwin/lib/rgb.txt file are 8-bit
        #
        # key is (red, green, blue) tuple, value is (name, [aliases])
        self.__byrgb = {}
        # key is name, value is (red, green, blue)
        self.__byname = {}
        # all unique names (non-aliases).  built-on demand
        self.__allnames = None
        for line in fp:
            # get this compiled regular expression from derived class
            mo = self._re.match(line)
            if not mo:
                print('Error in', fp.name, ' line', lineno, file=sys.stderr)
                lineno += 1
                continue
            # extract the red, green, blue, and name
            red, green, blue = self._extractrgb(mo)
            name = self._extractname(mo)
            keyname = name.lower()
            # BAW: for now the `name' is just the first named color with the
            # rgb values we find.  Later, we might want to make the two word
            # version the `name', or the CapitalizedVersion, etc.
            key = (red, green, blue)
            foundname, aliases = self.__byrgb.get(key, (name, []))
            if foundname <> name and foundname not in aliases:
                aliases.append(name)
            self.__byrgb[key] = (foundname, aliases)
            # add to byname lookup
            self.__byname[keyname] = key
            lineno = lineno + 1

    # override in derived classes
    def _extractrgb(self, mo):
        return [int(x) for x in mo.group('red', 'green', 'blue')]

    def _extractname(self, mo):
        return mo.group('name')

    def filename(self):
        return self.__name

    def find_byrgb(self, rgbtuple):
        """Return name for rgbtuple"""
        try:
            return self.__byrgb[rgbtuple]
        except KeyError:
            raise BadColor(rgbtuple)

    def find_byname(self, name):
        """Return (red, green, blue) for name"""
        name = name.lower()
        try:
            return self.__byname[name]
        except KeyError:
            raise BadColor(name)

    def nearest(self, red, green, blue):
        """Return the name of color nearest (red, green, blue)"""
        # BAW: should we use Voronoi diagrams, Delaunay triangulation, or
        # octree for speeding up the locating of nearest point?  Exhaustive
        # search is inefficient, but seems fast enough.
        nearest = -1
        nearest_name = ''
        for name, aliases in self.__byrgb.values():
            r, g, b = self.__byname[name.lower()]
            rdelta = red - r
            gdelta = green - g
            bdelta = blue - b
            distance = rdelta * rdelta + gdelta * gdelta + bdelta * bdelta
            if nearest == -1 or distance < nearest:
                nearest = distance
                nearest_name = name
        return nearest_name

    def unique_names(self):
        # sorted
        if not self.__allnames:
            self.__allnames = []
            for name, aliases in self.__byrgb.values():
                self.__allnames.append(name)
            # sort irregardless of case
            def nocase_cmp(n1, n2):
                return cmp(n1.lower(), n2.lower())
            self.__allnames.sort(nocase_cmp)
        return self.__allnames

    def aliases_of(self, red, green, blue):
        try:
            name, aliases = self.__byrgb[(red, green, blue)]
        except KeyError:
            raise BadColor((red, green, blue))
        return [name] + aliases


class RGBColorDB(ColorDB):
    _re = re.compile(
        '\s*(?P<red>\d+)\s+(?P<green>\d+)\s+(?P<blue>\d+)\s+(?P<name>.*)')


class HTML40DB(ColorDB):
    _re = re.compile('(?P<name>\S+)\s+(?P<hexrgb>#[0-9a-fA-F]{6})')

    def _extractrgb(self, mo):
        return rrggbb_to_triplet(mo.group('hexrgb'))

class LightlinkDB(HTML40DB):
    _re = re.compile('(?P<name>(.+))\s+(?P<hexrgb>#[0-9a-fA-F]{6})')

    def _extractname(self, mo):
        return mo.group('name').strip()

class WebsafeDB(ColorDB):
    _re = re.compile('(?P<hexrgb>#[0-9a-fA-F]{6})')

    def _extractrgb(self, mo):
        return rrggbb_to_triplet(mo.group('hexrgb'))

    def _extractname(self, mo):
        return mo.group('hexrgb').upper()



# format is a tuple (RE, SCANLINES, CLASS) where RE is a compiled regular
# expression, SCANLINES is the number of header lines to scan, and CLASS is
# the class to instantiate if a match is found

FILETYPES = [
    (re.compile('Xorg'), RGBColorDB),
    (re.compile('XConsortium'), RGBColorDB),
    (re.compile('HTML'), HTML40DB),
    (re.compile('lightlink'), LightlinkDB),
    (re.compile('Websafe'), WebsafeDB),
    ]

def get_colordb(file, filetype=None):
    colordb = None
    fp = open(file)
    try:
        line = fp.readline()
        if not line:
            return None
        # try to determine the type of RGB file it is
        if filetype is None:
            filetypes = FILETYPES
        else:
            filetypes = [filetype]
        for typere, class_ in filetypes:
            mo = typere.search(line)
            if mo:
                break
        else:
            # no matching type
            return None
        # we know the type and the class to grok the type, so suck it in
        colordb = class_(fp)
    finally:
        fp.close()
    # save a global copy
    global DEFAULT_DB
    DEFAULT_DB = colordb
    return colordb



_namedict = {}

def rrggbb_to_triplet(color):
    """Converts a #rrggbb color to the tuple (red, green, blue)."""
    rgbtuple = _namedict.get(color)
    if rgbtuple is None:
        if color[0] <> '#':
            raise BadColor(color)
        red = color[1:3]
        green = color[3:5]
        blue = color[5:7]
        rgbtuple = int(red, 16), int(green, 16), int(blue, 16)
        _namedict[color] = rgbtuple
    return rgbtuple


_tripdict = {}
def triplet_to_rrggbb(rgbtuple):
    """Converts a (red, green, blue) tuple to #rrggbb."""
    global _tripdict
    hexname = _tripdict.get(rgbtuple)
    if hexname is None:
        hexname = '#%02x%02x%02x' % rgbtuple
        _tripdict[rgbtuple] = hexname
    return hexname


_maxtuple = (256.0,) * 3
def triplet_to_fractional_rgb(rgbtuple):
    return map(operator.__div__, rgbtuple, _maxtuple)


def triplet_to_brightness(rgbtuple):
    # return the brightness (grey level) along the scale 0.0==black to
    # 1.0==white
    r = 0.299
    g = 0.587
    b = 0.114
    return r*rgbtuple[0] + g*rgbtuple[1] + b*rgbtuple[2]



if __name__ == '__main__':
    colordb = get_colordb('/usr/openwin/lib/rgb.txt')
    if not colordb:
        print('No parseable color database found')
        sys.exit(1)
    # on my system, this color matches exactly
    target = 'navy'
    red, green, blue = rgbtuple = colordb.find_byname(target)
    print(target, ':', red, green, blue, triplet_to_rrggbb(rgbtuple))
    name, aliases = colordb.find_byrgb(rgbtuple)
    print('name:', name, 'aliases:', COMMASPACE.join(aliases))
    r, g, b = (1, 1, 128)                         # nearest to navy
    r, g, b = (145, 238, 144)                     # nearest to lightgreen
    r, g, b = (255, 251, 250)                     # snow
    print('finding nearest to', target, '...')
    import time
    t0 = time.time()
    nearest = colordb.nearest(r, g, b)
    t1 = time.time()
    print('found nearest color', nearest, 'in', t1-t0, 'seconds')
    # dump the database
    for n in colordb.unique_names():
        r, g, b = colordb.find_byname(n)
        aliases = colordb.aliases_of(r, g, b)
        print('%20s: (%3d/%3d/%3d) == %s' % (n, r, g, b,
                                             SPACE.join(aliases[1:])))
