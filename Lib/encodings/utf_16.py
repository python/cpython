""" Python 'utf-16' Codec


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""
import codecs, sys

### Codec APIs

class Codec(codecs.Codec):

    # Note: Binding these as C functions will result in the class not
    # converting them to methods. This is intended.
    encode = codecs.utf_16_encode
    decode = codecs.utf_16_decode

class StreamWriter(Codec,codecs.StreamWriter):
    def __init__(self, stream, errors='strict'):
        self.bom_written = 0
        codecs.StreamWriter.__init__(self, stream, errors)

    def write(self, data):
        result = codecs.StreamWriter.write(self, data)
        if not self.bom_written:
            self.bom_written = 1
            if sys.byteorder == 'little':
                self.encode = codecs.utf_16_le_encode
            else:
                self.encode = codecs.utf_16_be_encode
        return result
        
class StreamReader(Codec,codecs.StreamReader):
    def __init__(self, stream, errors='strict'):
        self.bom_read = 0
        codecs.StreamReader.__init__(self, stream, errors)

    def read(self, size=-1):
        if not self.bom_read:
            signature = self.stream.read(2)
            if signature == codecs.BOM_BE:
                self.decode = codecs.utf_16_be_decode
            elif signature == codecs.BOM_LE:
                self.decode = codecs.utf_16_le_decode
            else:
                raise UnicodeError,"UTF-16 stream does not start with BOM"
            if size > 2:
                size -= 2
            elif size >= 0:
                size = 0
            self.bom_read = 1
        return codecs.StreamReader.read(self, size)

### encodings module API

def getregentry():

    return (Codec.encode,Codec.decode,StreamReader,StreamWriter)

