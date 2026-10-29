"""Python 'base64_codec' Codec - base64 content transfer encoding.

This codec de/encodes from bytes to bytes.

Written by Marc-Andre Lemburg (mal@lemburg.com).
"""

import codecs
import base64

### Codec APIs

def base64_encode(input, errors='strict'):
    assert errors == 'strict'
    return (base64.encodebytes(input), len(input))

def base64_decode(input, errors='strict'):
    assert errors == 'strict'
    return (base64.decodebytes(input), len(input))

class Codec(codecs.Codec):
    def encode(self, input, errors='strict'):
        return base64_encode(input, errors)
    def decode(self, input, errors='strict'):
        return base64_decode(input, errors)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        assert self.errors == 'strict'
        return base64.encodebytes(input)

_alphabet = (b'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
             b'abcdefghijklmnopqrstuvwxyz0123456789+/')

class IncrementalDecoder(codecs.BufferedIncrementalDecoder):
    def _buffer_decode(self, input, errors, final):
        assert errors == 'strict'
        if not final:
            # Only decode complete groups of four base64 characters and keep
            # the rest for the next call.  Other bytes, such as newlines and
            # padding, are not counted.
            count = len(input) - len(input.translate(None, _alphabet))
            remainder = count % 4
            end = len(input)
            while remainder:
                end -= 1
                if input[end] in _alphabet:
                    remainder -= 1
            input = input[:end]
        return (base64.decodebytes(input), len(input))

class StreamWriter(Codec, codecs.StreamWriter):
    charbuffertype = bytes

class StreamReader(Codec, codecs.StreamReader):
    charbuffertype = bytes

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='base64',
        encode=base64_encode,
        decode=base64_decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamwriter=StreamWriter,
        streamreader=StreamReader,
        _is_text_encoding=False,
    )
