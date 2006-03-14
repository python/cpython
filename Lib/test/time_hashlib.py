# It's intended that this script be run by hand.  It runs speed tests on
# hashlib functions; it does not test for correctness.

import sys, time
import hashlib


def creatorFunc():
    raise RuntimeError, "eek, creatorFunc not overridden"

def test_scaled_msg(scale, name):
    iterations = 106201/scale * 20
    longStr = 'Z'*scale

    localCF = creatorFunc
    start = time.time()
    for f in xrange(iterations):
        x = localCF(longStr).digest()
    end = time.time()

    print ('%2.2f' % (end-start)), "seconds", iterations, "x", len(longStr), "bytes", name

def test_create():
    start = time.time()
    for f in xrange(20000):
        d = creatorFunc()
    end = time.time()

    print ('%2.2f' % (end-start)), "seconds", '[20000 creations]'

def test_zero():
    start = time.time()
    for f in xrange(20000):
        x = creatorFunc().digest()
    end = time.time()

    print ('%2.2f' % (end-start)), "seconds", '[20000 "" digests]'



hName = sys.argv[1]

#
# setup our creatorFunc to test the requested hash
#
if hName in ('_md5', '_sha'):
    exec 'import '+hName
    exec 'creatorFunc = '+hName+'.new'
    print "testing speed of old", hName, "legacy interface"
elif hName == '_hashlib' and len(sys.argv) > 3:
    import _hashlib
    exec 'creatorFunc = _hashlib.%s' % sys.argv[2]
    print "testing speed of _hashlib.%s" % sys.argv[2], getattr(_hashlib, sys.argv[2])
elif hName == '_hashlib' and len(sys.argv) == 3:
    import _hashlib
    exec 'creatorFunc = lambda x=_hashlib.new : x(%r)' % sys.argv[2]
    print "testing speed of _hashlib.new(%r)" % sys.argv[2]
elif hasattr(hashlib, hName) and callable(getattr(hashlib, hName)):
    creatorFunc = getattr(hashlib, hName)
    print "testing speed of hashlib."+hName, getattr(hashlib, hName)
else:
    exec "creatorFunc = lambda x=hashlib.new : x(%r)" % hName
    print "testing speed of hashlib.new(%r)" % hName

try:
    test_create()
except ValueError:
    print
    print "pass argument(s) naming the hash to run a speed test on:"
    print " '_md5' and '_sha' test the legacy builtin md5 and sha"
    print " '_hashlib' 'openssl_hName' 'fast' tests the builtin _hashlib"
    print " '_hashlib' 'hName' tests builtin _hashlib.new(shaFOO)"
    print " 'hName' tests the hashlib.hName() implementation if it exists"
    print "         otherwise it uses hashlib.new(hName)."
    print
    raise

test_zero()
test_scaled_msg(scale=106201, name='[huge data]')
test_scaled_msg(scale=10620, name='[large data]')
test_scaled_msg(scale=1062, name='[medium data]')
test_scaled_msg(scale=424, name='[4*small data]')
test_scaled_msg(scale=336, name='[3*small data]')
test_scaled_msg(scale=212, name='[2*small data]')
test_scaled_msg(scale=106, name='[small data]')
test_scaled_msg(scale=creatorFunc().digest_size, name='[digest_size data]')
test_scaled_msg(scale=10, name='[tiny data]')
