"""PixMapWrapper - defines the PixMapWrapper class, which wraps an opaque
QuickDraw PixMap data structure in a handy Python class.  Also provides
methods to convert to/from pixel data (from, e.g., the img module) or a
Python Imaging Library Image object.

J. Strout <joe@strout.net>  February 1999"""

from Carbon import Qd
from Carbon import QuickDraw
import struct
import MacOS
import img
import imgformat

# PixMap data structure element format (as used with struct)
_pmElemFormat = {
    'baseAddr':'l',     # address of pixel data
    'rowBytes':'H',     # bytes per row, plus 0x8000
    'bounds':'hhhh',    # coordinates imposed over pixel data
        'top':'h',
        'left':'h',
        'bottom':'h',
        'right':'h',
    'pmVersion':'h',    # flags for Color QuickDraw
    'packType':'h',     # format of compression algorithm
    'packSize':'l',     # size after compression
    'hRes':'l',         # horizontal pixels per inch
    'vRes':'l',         # vertical pixels per inch
    'pixelType':'h',    # pixel format
    'pixelSize':'h',    # bits per pixel
    'cmpCount':'h',     # color components per pixel
    'cmpSize':'h',      # bits per component
    'planeBytes':'l',   # offset in bytes to next plane
    'pmTable':'l',      # handle to color table
    'pmReserved':'l'    # reserved for future use
}

# PixMap data structure element offset
_pmElemOffset = {
    'baseAddr':0,
    'rowBytes':4,
    'bounds':6,
        'top':6,
        'left':8,
        'bottom':10,
        'right':12,
    'pmVersion':14,
    'packType':16,
    'packSize':18,
    'hRes':22,
    'vRes':26,
    'pixelType':30,
    'pixelSize':32,
    'cmpCount':34,
    'cmpSize':36,
    'planeBytes':38,
    'pmTable':42,
    'pmReserved':46
}

class PixMapWrapper:
    """PixMapWrapper -- wraps the QD PixMap object in a Python class,
    with methods to easily get/set various pixmap fields.  Note: Use the
    PixMap() method when passing to QD calls."""

    def __init__(self):
        self.__dict__['data'] = ''
        self._header = struct.pack("lhhhhhhhlllhhhhlll",
            id(self.data)+MacOS.string_id_to_buffer,
            0,                      # rowBytes
            0, 0, 0, 0,             # bounds
            0,                      # pmVersion
            0, 0,                   # packType, packSize
            72<<16, 72<<16,         # hRes, vRes
            QuickDraw.RGBDirect,    # pixelType
            16,                     # pixelSize
            2, 5,                   # cmpCount, cmpSize,
            0, 0, 0)                # planeBytes, pmTable, pmReserved
        self.__dict__['_pm'] = Qd.RawBitMap(self._header)

    def _stuff(self, element, bytes):
        offset = _pmElemOffset[element]
        fmt = _pmElemFormat[element]
        self._header = self._header[:offset] \
            + struct.pack(fmt, bytes) \
            + self._header[offset + struct.calcsize(fmt):]
        self.__dict__['_pm'] = None

    def _unstuff(self, element):
        offset = _pmElemOffset[element]
        fmt = _pmElemFormat[element]
        return struct.unpack(fmt, self._header[offset:offset+struct.calcsize(fmt)])[0]

    def __setattr__(self, attr, val):
        if attr == 'baseAddr':
            raise 'UseErr', "don't assign to .baseAddr -- assign to .data instead"
        elif attr == 'data':
            self.__dict__['data'] = val
            self._stuff('baseAddr', id(self.data) + MacOS.string_id_to_buffer)
        elif attr == 'rowBytes':
            # high bit is always set for some odd reason
            self._stuff('rowBytes', val | 0x8000)
        elif attr == 'bounds':
            # assume val is in official Left, Top, Right, Bottom order!
            self._stuff('left',val[0])
            self._stuff('top',val[1])
            self._stuff('right',val[2])
            self._stuff('bottom',val[3])
        elif attr == 'hRes' or attr == 'vRes':
            # 16.16 fixed format, so just shift 16 bits
            self._stuff(attr, int(val) << 16)
        elif attr in _pmElemFormat.keys():
            # any other pm attribute -- just stuff
            self._stuff(attr, val)
        else:
            self.__dict__[attr] = val

    def __getattr__(self, attr):
        if attr == 'rowBytes':
            # high bit is always set for some odd reason
            return self._unstuff('rowBytes') & 0x7FFF
        elif attr == 'bounds':
            # return bounds in official Left, Top, Right, Bottom order!
            return ( \
                self._unstuff('left'),
                self._unstuff('top'),
                self._unstuff('right'),
                self._unstuff('bottom') )
        elif attr == 'hRes' or attr == 'vRes':
            # 16.16 fixed format, so just shift 16 bits
            return self._unstuff(attr) >> 16
        elif attr in _pmElemFormat.keys():
            # any other pm attribute -- just unstuff
            return self._unstuff(attr)
        else:
            return self.__dict__[attr]


    def PixMap(self):
        "Return a QuickDraw PixMap corresponding to this data."
        if not self.__dict__['_pm']:
            self.__dict__['_pm'] = Qd.RawBitMap(self._header)
        return self.__dict__['_pm']

    def blit(self, x1=0,y1=0,x2=None,y2=None, port=None):
        """Draw this pixmap into the given (default current) grafport."""
        src = self.bounds
        dest = [x1,y1,x2,y2]
        if x2 == None:
            dest[2] = x1 + src[2]-src[0]
        if y2 == None:
            dest[3] = y1 + src[3]-src[1]
        if not port: port = Qd.GetPort()
        Qd.CopyBits(self.PixMap(), port.GetPortBitMapForCopyBits(), src, tuple(dest),
                QuickDraw.srcCopy, None)

    def fromstring(self,s,width,height,format=imgformat.macrgb):
        """Stuff this pixmap with raw pixel data from a string.
        Supply width, height, and one of the imgformat specifiers."""
        # we only support 16- and 32-bit mac rgb...
        # so convert if necessary
        if format != imgformat.macrgb and format != imgformat.macrgb16:
            # (LATER!)
            raise "NotImplementedError", "conversion to macrgb or macrgb16"
        self.data = s
        self.bounds = (0,0,width,height)
        self.cmpCount = 3
        self.pixelType = QuickDraw.RGBDirect
        if format == imgformat.macrgb:
            self.pixelSize = 32
            self.cmpSize = 8
        else:
            self.pixelSize = 16
            self.cmpSize = 5
        self.rowBytes = width*self.pixelSize/8

    def tostring(self, format=imgformat.macrgb):
        """Return raw data as a string in the specified format."""
        # is the native format requested?  if so, just return data
        if (format == imgformat.macrgb and self.pixelSize == 32) or \
           (format == imgformat.macrgb16 and self.pixelsize == 16):
            return self.data
        # otherwise, convert to the requested format
        # (LATER!)
            raise "NotImplementedError", "data format conversion"

    def fromImage(self,im):
        """Initialize this PixMap from a PIL Image object."""
        # We need data in ARGB format; PIL can't currently do that,
        # but it can do RGBA, which we can use by inserting one null
        # up frontpm =
        if im.mode != 'RGBA': im = im.convert('RGBA')
        data = chr(0) + im.tostring()
        self.fromstring(data, im.size[0], im.size[1])

    def toImage(self):
        """Return the contents of this PixMap as a PIL Image object."""
        import Image
        # our tostring() method returns data in ARGB format,
        # whereas Image uses RGBA; a bit of slicing fixes this...
        data = self.tostring()[1:] + chr(0)
        bounds = self.bounds
        return Image.fromstring('RGBA',(bounds[2]-bounds[0],bounds[3]-bounds[1]),data)

def test():
    import MacOS
    import EasyDialogs
    import Image
    path = EasyDialogs.AskFileForOpen("Image File:")
    if not path: return
    pm = PixMapWrapper()
    pm.fromImage( Image.open(path) )
    pm.blit(20,20)
    return pm
