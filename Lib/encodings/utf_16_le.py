""" Python 'utf-16-le' Codec


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""
import codecs

### Codec APIs

encode = codecs.utf_16_le_encode

def decode(input, errors='strict'):
    return codecs.utf_16_le_decode(input, errors, True)

class StreamWriter(codecs.StreamWriter):
    encode = codecs.utf_16_le_encode

class StreamReader(codecs.StreamReader):
    decode = codecs.utf_16_le_decode


### encodings module API

def getregentry():

    return (encode,decode,StreamReader,StreamWriter)
