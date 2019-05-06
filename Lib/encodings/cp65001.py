"""
Code page 65001: Windows UTF-8 (CP_UTF8).
"""

import codecs

if not hasattr(codecs, 'code_page_encode'):
    raise LookupError("cp65001 encoding is only available on Windows")

### Codec APIs

encode = lambda *args, **kw: codecs.code_page_encode(65001, *args, **kw)
_decode = lambda *args, **kw: codecs.code_page_decode(65001, *args, **kw)

def decode(input, errors='strict'):
    return codecs.code_page_decode(65001, input, errors, True)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return encode(input, self.errors)[0]

class IncrementalDecoder(codecs.BufferedIncrementalDecoder):
    _buffer_decode = staticmethod(_decode)

class StreamWriter(codecs.StreamWriter):
    encode = staticmethod(encode)

class StreamReader(codecs.StreamReader):
    decode = staticmethod(_decode)

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='cp65001',
        encode=encode,
        decode=decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )
