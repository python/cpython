# Test the pickle module

# break into multiple strings to please font-lock-mode
DATA = """(lp0
I0
aL1L
aF2.0
ac__builtin__
complex
p1
""" \
"""(F3.0
F0.0
tp2
Rp3
a(S'abc'
p4
g4
""" \
"""(i__main__
C
p5
""" \
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

BINDATA = ']q\000(K\000L1L\012G@\000\000\000\000\000\000\000c__builtin__\012complex\012q\001(G@\010\000\000\000\000\000\000G\000\000\000\000\000\000\000\000tq\002Rq\003(U\003abcq\004h\004(c__main__\012C\012q\005oq\006}q\007(U\003fooq\010K\001U\003barq\011K\002ubh\006tq\012h\012K\005e.'

class C:
    def __cmp__(self, other):
        return cmp(self.__dict__, other.__dict__)

import __main__
__main__.C = C

def dotest(pickle):
    c = C()
    c.foo = 1
    c.bar = 2
    x = [0, 1L, 2.0, 3.0+0j]
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
    if x2 == x: print "ok"
    else: print "bad"
    print "loads() DATA"
    x2 = pickle.loads(DATA)
    if x2 == x: print "ok"
    else: print "bad"
    print "dumps() binary"
    s = pickle.dumps(x, 1)
    print "loads() binary"
    x2 = pickle.loads(s)
    if x2 == x: print "ok"
    else: print "bad"
    print "loads() BINDATA"
    x2 = pickle.loads(BINDATA)
    if x2 == x: print "ok"
    else: print "bad"
    s = pickle.dumps(r)
    print "dumps() RECURSIVE"
    x2 = pickle.loads(s)
    if x2 == r: print "ok"
    else: print "bad"
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
        

import pickle
dotest(pickle)
