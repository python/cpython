""" Python 'bz2_codec' Codec - bz2 compression encoding

    Unlike most of the other codecs which target Unicode, this codec
    will return Python string objects for both encode and decode.

    Adapted by Raymond Hettinger from zlib_codec.py which was written
    by Marc-Andre Lemburg (mal@lemburg.com).

"""
import codecs
import bz2 # this codec needs the optional bz2 module !

### Codec APIs

def bz2_encode(input,errors='strict'):

    """ Encodes the object input and returns a tuple (output
        object, length consumed).

        errors defines the error handling to apply. It defaults to
        'strict' handling which is the only currently supported
        error handling for this codec.

    """
    assert errors == 'strict'
    output = bz2.compress(input)
    return (output, len(input))

def bz2_decode(input,errors='strict'):

    """ Decodes the object input and returns a tuple (output
        object, length consumed).

        input must be an object which provides the bf_getreadbuf
        buffer slot. Python strings, buffer objects and memory
        mapped files are examples of objects providing this slot.

        errors defines the error handling to apply. It defaults to
        'strict' handling which is the only currently supported
        error handling for this codec.

    """
    assert errors == 'strict'
    output = bz2.decompress(input)
    return (output, len(input))

class Codec(codecs.Codec):

    def encode(self, input, errors='strict'):
        return bz2_encode(input, errors)
    def decode(self, input, errors='strict'):
        return bz2_decode(input, errors)

class StreamWriter(Codec,codecs.StreamWriter):
    pass

class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (bz2_encode,bz2_decode,StreamReader,StreamWriter)
