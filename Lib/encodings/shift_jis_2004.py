#
# shift_jis_2004.py: Python Unicode Codec for SHIFT_JIS_2004
#
# Written by Hye-Shik Chang <perky@FreeBSD.org>
# $CJKCodecs: shift_jis_2004.py,v 1.1 2004/07/07 16:18:25 perky Exp $
#

import _codecs_jp, codecs

codec = _codecs_jp.getcodec('shift_jis_2004')

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
