"""
Code page 65001: Windows UTF-8 (CP_UTF8).
"""

import codecs

if not hasattr(codecs, 'code_page_encode'):
    raise LookupError("cp65001 encoding is only available on Windows")

### Codec APIs

def encode(input, errors='strict'):
    return codecs.code_page_encode(65001, input, errors)

def _decode(input, errors='strict'):
    return codecs.code_page_decode(65001, input, errors)

def decode(input, errors='strict'):
    return codecs.code_page_decode(65001, input, errors, True)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return encode(input, self.errors)[0]

class IncrementalDecoder(codecs.BufferedIncrementalDecoder):
    def _buffer_decode(self, input, errors, final=False):
        return _decode(input, self.errors)

class StreamWriter(codecs.StreamWriter):
    def encode(self, input, errors='strict'):
        return encode(input, self.errors)

class StreamReader(codecs.StreamReader):
    def decode(self, input, errors='strict'):
        return _decode(input, errors)

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
