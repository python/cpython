""" Python 'mbcs' Codec for Windows


Cloned by Mark Hammond (mhammond@skippinet.com.au) from ascii.py,
which was written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""
import codecs

### Codec APIs

class Codec(codecs.Codec):

    # Note: Binding these as C functions will result in the class not
    # converting them to methods. This is intended.
    encode = codecs.mbcs_encode
    decode = codecs.mbcs_decode

class StreamWriter(Codec,codecs.StreamWriter):
    pass

class StreamReader(Codec,codecs.StreamReader):
    pass

class StreamConverter(StreamWriter,StreamReader):

    encode = codecs.mbcs_decode
    decode = codecs.mbcs_encode

### encodings module API

def getregentry():

    return (Codec.encode,Codec.decode,StreamReader,StreamWriter)
