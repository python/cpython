#
# iso2022_jp_ext.py: Python Unicode Codec for ISO2022_JP_EXT
#
# Written by Hye-Shik Chang <perky@FreeBSD.org>
# $CJKCodecs: iso2022_jp_ext.py,v 1.2 2004/06/28 18:16:03 perky Exp $
#

import _codecs_iso2022, codecs

codec = _codecs_iso2022.getcodec('iso2022_jp_ext')

class Codec(codecs.Codec):
    encode = codec.encode
    decode = codec.decode

class StreamReader(Codec, codecs.StreamReader):
    def __init__(self, stream, errors='strict'):
        codecs.StreamReader.__init__(self, stream, errors)
        __codec = codec.StreamReader(stream, errors)
        self.read = __codec.read
        self.readline = __codec.readline
        self.readlines = __codec.readlines
        self.reset = __codec.reset

class StreamWriter(Codec, codecs.StreamWriter):
    def __init__(self, stream, errors='strict'):
        codecs.StreamWriter.__init__(self, stream, errors)
        __codec = codec.StreamWriter(stream, errors)
        self.write = __codec.write
        self.writelines = __codec.writelines
        self.reset = __codec.reset

def getregentry():
    return (codec.encode, codec.decode, StreamReader, StreamWriter)
