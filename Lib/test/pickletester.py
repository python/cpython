# test_pickle and test_cpickle both use this.

# break into multiple strings to please font-lock-mode
DATA = """(lp1
I0
aL1L
aF2
ac__builtin__
complex
p2
""" + \
"""(F3
F0
tRp3
aI1
aI-1
aI255
aI-255
aI-256
aI65535
aI-65535
aI-65536
aI2147483647
aI-2147483647
aI-2147483648
a""" + \
"""(S'abc'
p4
g4
""" + \
"""(ipickletester
C
p5
""" + \
"""(dp6
S'foo'
p7
I1
sS'bar'
p8
I2
sbg5
tp9
ag9
aI5
a.
"""

BINDATA = ']q\x01(K\x00L1L\nG@\x00\x00\x00\x00\x00\x00\x00' + \
          'c__builtin__\ncomplex\nq\x02(G@\x08\x00\x00\x00\x00\x00' + \
          '\x00G\x00\x00\x00\x00\x00\x00\x00\x00tRq\x03K\x01J\xff\xff' + \
          '\xff\xffK\xffJ\x01\xff\xff\xffJ\x00\xff\xff\xffM\xff\xff' + \
          'J\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff\xff\xff\x7fJ\x01\x00' + \
          '\x00\x80J\x00\x00\x00\x80(U\x03abcq\x04h\x04(cpickletester\n' + \
          'C\nq\x05oq\x06}q\x07(U\x03fooq\x08K\x01U\x03barq\tK\x02ubh' + \
          '\x06tq\nh\nK\x05e.'

class C:
    def __cmp__(self, other):
        return cmp(self.__dict__, other.__dict__)

import __main__
__main__.C = C

# Call this with the module to be tested (pickle or cPickle).

def dotest(pickle):
    c = C()
    c.foo = 1
    c.bar = 2
    x = [0, 1L, 2.0, 3.0+0j]
    # Append some integer test cases at cPickle.c's internal size
    # cutoffs.
    uint1max = 0xff
    uint2max = 0xffff
    int4max = 0x7fffffff
    x.extend([1, -1,
              uint1max, -uint1max, -uint1max-1,
              uint2max, -uint2max, -uint2max-1,
               int4max,  -int4max,  -int4max-1])
    y = ('abc', 'abc', c, c)
    x.append(y)
    x.append(y)
    x.append(5)
    r = []
    r.append(r)

    print "dumps()"
    s = pickle.dumps(x)

    print "loads()"
    x2 = pickle.loads(s)
    if x2 == x:
        print "ok"
    else:
        print "bad"

    print "loads() DATA"
    x2 = pickle.loads(DATA)
    if x2 == x:
        print "ok"
    else:
        print "bad"

    print "dumps() binary"
    s = pickle.dumps(x, 1)

    print "loads() binary"
    x2 = pickle.loads(s)
    if x2 == x:
        print "ok"
    else:
        print "bad"

    print "loads() BINDATA"
    x2 = pickle.loads(BINDATA)
    if x2 == x:
        print "ok"
    else:
        print "bad"

    print "dumps() RECURSIVE"
    s = pickle.dumps(r)
    x2 = pickle.loads(s)
    if x2 == r:
        print "ok"
    else:
        print "bad"

    # don't create cyclic garbage
    del x2[0]
    del r[0]

    # Test protection against closed files
    import tempfile, os
    fn = tempfile.mktemp()
    f = open(fn, "w")
    f.close()
    try:
        pickle.dump(123, f)
    except ValueError:
        pass
    else:
        print "dump to closed file should raise ValueError"

    f = open(fn, "r")
    f.close()
    try:
        pickle.load(f)
    except ValueError:
        pass
    else:
        print "load from closed file should raise ValueError"
    os.remove(fn)

    # Test specific bad cases
    for i in range(10):
        try:
            x = pickle.loads('garyp')
        except KeyError, y:
            # pickle
            del y
        except pickle.BadPickleGet, y:
            # cPickle
            del y
        else:
            print "unexpected success!"
            break

    # Test insecure strings
    insecure = ["abc", "2 + 2", # not quoted
                "'abc' + 'def'", # not a single quoted string
                "'abc", # quote is not closed
                "'abc\"", # open quote and close quote don't match
                "'abc'   ?", # junk after close quote
                # some tests of the quoting rules
                "'abc\"\''",
                "'\\\\a\'\'\'\\\'\\\\\''",
                ]
    for s in insecure:
        buf = "S" + s + "\012p0\012."
        try:
            x = pickle.loads(buf)
        except ValueError:
            pass
        else:
            print "accepted insecure string: %s" % repr(buf)

    # Test some Unicode end cases
    endcases = [u'', u'<\\u>', u'<\\\u1234>', u'<\n>',  u'<\\>']
    for u in endcases:
        try:
            u2 = pickle.loads(pickle.dumps(u))
        except Exception, msg:
            print "Endcase exception: %s => %s(%s)" % \
                  (`u`, msg.__class__.__name__, str(msg))
        else:
            if u2 != u:
                print "Endcase failure: %s => %s" % (`u`, `u2`)
