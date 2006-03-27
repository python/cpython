import os, string

codecs = {
    'cn': ('gb2312', 'gbk', 'gb18030', 'hz'),
    'tw': ('big5', 'cp950'),
    'hk': ('big5hkscs',),
    'jp': ('cp932', 'shift_jis', 'euc_jp', 'euc_jisx0213', 'shift_jisx0213',
           'euc_jis_2004', 'shift_jis_2004'),
    'kr': ('cp949', 'euc_kr', 'johab'),
    'iso2022': ('iso2022_jp', 'iso2022_jp_1', 'iso2022_jp_2',
                'iso2022_jp_2004', 'iso2022_jp_3', 'iso2022_jp_ext',
                'iso2022_kr'),
}

TEMPLATE = string.Template("""\
#
# $encoding.py: Python Unicode Codec for $ENCODING
#
# Written by Hye-Shik Chang <perky@FreeBSD.org>
#

import _codecs_$owner, codecs
import _multibytecodec as mbc

codec = _codecs_$owner.getcodec('$encoding')

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
        name='$encoding',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )
""")

def gencodecs(prefix):
    for loc, encodings in codecs.iteritems():
        for enc in encodings:
            code = TEMPLATE.substitute(ENCODING=enc.upper(),
                                       encoding=enc.lower(),
                                       owner=loc)
            codecpath = os.path.join(prefix, enc + '.py')
            open(codecpath, 'w').write(code)

if __name__ == '__main__':
    import sys
    gencodecs(sys.argv[1])
