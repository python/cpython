#
# shift_jis.py: Python Unicode Codec for SHIFT_JIS
#
# Written by Hye-Shik Chang <perky@FreeBSD.org>
#

import _codecs_jp, codecs
import _multibytecodec as mbc

codec = _codecs_jp.getcodec('shift_jis')

class Codec(codecs.Codec):
    encode = codec.encode
    decode = codec.decode

class IncrementalEncoder(mbc.MultibyteIncrementalEncoder,
                         codecs.IncrementalEncoder):
    codec = codec

class IncrementalDecoder(mbc.MultibyteIncrementalDecoder,
                         codecs.IncrementalDecoder):
    codec = codec

class StreamReader(Codec, mbc.MultibyteStreamReader, codecs.StreamReader):
    codec = codec

class StreamWriter(Codec, mbc.MultibyteStreamWriter, codecs.StreamWriter):
    codec = codec

def getregentry():
    return codecs.CodecInfo(
        name='shift_jis',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
        _expat_decoding_table=(*range(128),
            -1, -2, -2, -2, -2, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -1, 0xff61, 0xff62, 0xff63, 0xff64, 0xff65, 0xff66, 0xff67,
            0xff68, 0xff69, 0xff6a, 0xff6b, 0xff6c, 0xff6d, 0xff6e, 0xff6f,
            0xff70, 0xff71, 0xff72, 0xff73, 0xff74, 0xff75, 0xff76, 0xff77,
            0xff78, 0xff79, 0xff7a, 0xff7b, 0xff7c, 0xff7d, 0xff7e, 0xff7f,
            0xff80, 0xff81, 0xff82, 0xff83, 0xff84, 0xff85, 0xff86, 0xff87,
            0xff88, 0xff89, 0xff8a, 0xff8b, 0xff8c, 0xff8d, 0xff8e, 0xff8f,
            0xff90, 0xff91, 0xff92, 0xff93, 0xff94, 0xff95, 0xff96, 0xff97,
            0xff98, 0xff99, 0xff9a, 0xff9b, 0xff9c, 0xff9d, 0xff9e, 0xff9f,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
    )
