#! /usr/bin/env python

"""Conversions to/from base64 transport encoding as per RFC-1521."""

# Modified 04-Oct-95 by Jack to use binascii module

import binascii

MAXLINESIZE = 76 # Excluding the CRLF
MAXBINSIZE = (MAXLINESIZE/4)*3

def encode(input, output):
    """Encode a file."""
    while 1:
        s = input.read(MAXBINSIZE)
        if not s: break
        while len(s) < MAXBINSIZE:
            ns = input.read(MAXBINSIZE-len(s))
            if not ns: break
            s = s + ns
        line = binascii.b2a_base64(s)
        output.write(line)

def decode(input, output):
    """Decode a file."""
    while 1:
        line = input.readline()
        if not line: break
        s = binascii.a2b_base64(line)
        output.write(s)

def encodestring(s):
    """Encode a string."""
    import StringIO
    f = StringIO.StringIO(s)
    g = StringIO.StringIO()
    encode(f, g)
    return g.getvalue()

def decodestring(s):
    """Decode a string."""
    import StringIO
    f = StringIO.StringIO(s)
    g = StringIO.StringIO()
    decode(f, g)
    return g.getvalue()

def test():
    """Small test program"""
    import sys, getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'deut')
    except getopt.error, msg:
        sys.stdout = sys.stderr
        print msg
        print """usage: %s [-d|-e|-u|-t] [file|-]
        -d, -u: decode
        -e: encode (default)
        -t: encode and decode string 'Aladdin:open sesame'"""%sys.argv[0]
        sys.exit(2)
    func = encode
    for o, a in opts:
        if o == '-e': func = encode
        if o == '-d': func = decode
        if o == '-u': func = decode
        if o == '-t': test1(); return
    if args and args[0] != '-':
        func(open(args[0], 'rb'), sys.stdout)
    else:
        func(sys.stdin, sys.stdout)

def test1():
    s0 = "Aladdin:open sesame"
    s1 = encodestring(s0)
    s2 = decodestring(s1)
    print s0, `s1`, s2

if __name__ == '__main__':
    test()
