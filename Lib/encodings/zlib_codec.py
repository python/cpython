""" Python 'zlib_codec' Codec - zlib compression encoding

    Unlike most of the other codecs which target Unicode, this codec
    will return Python string objects for both encode and decode.

    Written by Marc-Andre Lemburg (mal@lemburg.com).

"""
import codecs
import zlib # this codec needs the optional zlib module !

### Codec APIs

def zlib_encode(input,errors='strict'):

    """ Encodes the object input and returns a tuple (output
        object, length consumed).

        errors defines the error handling to apply. It defaults to
        'strict' handling which is the only currently supported
        error handling for this codec.

    """
    assert errors == 'strict'
    output = zlib.compress(input)
    return (output, len(input))

def zlib_decode(input,errors='strict'):

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
    output = zlib.decompress(input)
    return (output, len(input))

class Codec(codecs.Codec):

    def encode(self, input, errors='strict'):
        return zlib_encode(input, errors)
    def decode(self, input, errors='strict'):
        return zlib_decode(input, errors)

class StreamWriter(Codec,codecs.StreamWriter):
    pass
        
class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (zlib_encode,zlib_decode,StreamReader,StreamWriter)
