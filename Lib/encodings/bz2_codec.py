""" Python 'bz2_codec' Codec - bz2 compression encoding

    Unlike most of the other codecs which target Unicode, this codec
    will return Python string objects for both encode and decode.

    Adapted by Raymond Hettinger from zlib_codec.py which was written
    by Marc-Andre Lemburg (mal@lemburg.com).

"""
import codecs
import bz2

def encode(input, errors='strict'):
    assert errors == 'strict'
    output = bz2.compress(input)
    return (output, len(input))

def decode(input, errors='strict'):
    assert errors == 'strict'
    output = bz2.decompress(input)
    return (output, len(input))

### encodings module API

def getregentry():

    return (encode, decode, codecs.StreamReader, codecs.StreamWriter)
