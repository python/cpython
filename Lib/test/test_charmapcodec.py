""" Python Character Mapping Codec test

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright 2000 Guido van Rossum.

"""#"
import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):

        return codecs.charmap_encode(input,errors,encoding_map)
        
    def decode(self,input,errors='strict'):

        return codecs.charmap_decode(input,errors,decoding_map)

class StreamWriter(Codec,codecs.StreamWriter):
    pass
        
class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (Codec().encode,Codec().decode,StreamReader,StreamWriter)

### Decoding Map

decoding_map = codecs.make_identity_dict(range(256))
decoding_map.update({
        0x0078: u"abc",
        "abc": 0x0078,
})

### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k


### Tests

def check(a, b):
    if a != b:
        print '*** check failed: %s != %s' % (repr(a), repr(b))
    
check(unicode('abc', 'mycp'), u'abc')
check(unicode('xdef', 'mycp'), u'abcdef')
check(unicode('defx', 'mycp'), u'defabc')
check(unicode('dxf', 'mycp'), u'dabcf')
check(unicode('dxfx', 'mycp'), u'dabcfabc')

check(u'abc'.encode('mycp'), 'abc')
check(u'xdef'.encode('mycp'), 'abcdef')
check(u'defx'.encode('mycp'), 'defabc')
check(u'dxf'.encode('mycp'), 'dabcf')
check(u'dxfx'.encode('mycp'), 'dabcfabc')
