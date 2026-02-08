"""Python 'hex_codec' Codec - 2-digit hex content transfer encoding.

This codec de/encodes from bytes to bytes.

Written by Marc-Andre Lemburg (mal@lemburg.com).
"""

import codecs
import binascii

### Codec Helpers

def _assert_strict(errors):
    if errors != 'strict':
        raise ValueError(f'Unsupported error handling mode: "{errors}" - must be "strict"')

### Codec APIs

def hex_encode(input, errors='strict'):
    _assert_strict(errors)
    return (binascii.b2a_hex(input), len(input))

def hex_decode(input, errors='strict'):
    _assert_strict(errors)
    return (binascii.a2b_hex(input), len(input))

class Codec(codecs.Codec):
    def encode(self, input, errors='strict'):
        return hex_encode(input, errors)
    def decode(self, input, errors='strict'):
        return hex_decode(input, errors)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        _assert_strict(self.errors)
        return binascii.b2a_hex(input)

class IncrementalDecoder(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        _assert_strict(self.errors)
        return binascii.a2b_hex(input)

class StreamWriter(Codec, codecs.StreamWriter):
    charbuffertype = bytes

class StreamReader(Codec, codecs.StreamReader):
    charbuffertype = bytes

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='hex',
        encode=hex_encode,
        decode=hex_decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamwriter=StreamWriter,
        streamreader=StreamReader,
        _is_text_encoding=False,
    )
