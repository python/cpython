""" Python 'iconv' Codec


Written by Hye-Shik Chang (perky@FreeBSD.org).

Copyright(c) Python Software Foundation, All Rights Reserved. NO WARRANTY.

"""

import _iconv_codec
import codecs

def lookup(enc):
    class IconvCodec(_iconv_codec.iconvcodec, codecs.Codec):
        encoding = enc

    try:
        c = IconvCodec()

        class IconvStreamReader(IconvCodec, codecs.StreamReader):
            __init__ = codecs.StreamReader.__init__
        class IconvStreamWriter(IconvCodec, codecs.StreamWriter):
            __init__ = codecs.StreamWriter.__init__

        return (
            c.encode, c.decode,
            IconvStreamReader, IconvStreamWriter
        )
    except ValueError:
        return None

codecs.register(lookup)

# ex: ts=8 sts=4 et
