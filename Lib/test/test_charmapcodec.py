""" Python character mapping codec test

This uses the test codec in testcodec.py and thus also tests the
encodings package lookup scheme.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright 2000 Guido van Rossum.

"""#"

def check(a, b):
    if a != b:
        print '*** check failed: %s != %s' % (repr(a), repr(b))
    else:
        print '%s == %s: OK' % (a, b)

# test codec's full path name (see test/testcodec.py)
codecname = 'test.testcodec'

check(unicode('abc', codecname), u'abc')
check(unicode('xdef', codecname), u'abcdef')
check(unicode('defx', codecname), u'defabc')
check(unicode('dxf', codecname), u'dabcf')
check(unicode('dxfx', codecname), u'dabcfabc')

check(u'abc'.encode(codecname), 'abc')
check(u'xdef'.encode(codecname), 'abcdef')
check(u'defx'.encode(codecname), 'defabc')
check(u'dxf'.encode(codecname), 'dabcf')
check(u'dxfx'.encode(codecname), 'dabcfabc')

check(unicode('ydef', codecname), u'def')
check(unicode('defy', codecname), u'def')
check(unicode('dyf', codecname), u'df')
check(unicode('dyfy', codecname), u'df')

try:
    unicode('abc\001', codecname)
except UnicodeError:
    print '\\001 maps to undefined: OK'
else:
    print '*** check failed: \\001 does not map to undefined'
