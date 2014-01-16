#!/usr/bin/env python

""" Compare the output of two codecs.

(c) Copyright 2005, Marc-Andre Lemburg (mal@lemburg.com).

    Licensed to PSF under a Contributor Agreement.

"""
import sys

def compare_codecs(encoding1, encoding2):

    print 'Comparing encoding/decoding of   %r and   %r' % (encoding1, encoding2)
    mismatch = 0
    # Check encoding
    for i in range(sys.maxunicode):
        u = unichr(i)
        try:
            c1 = u.encode(encoding1)
        except UnicodeError, reason:
            c1 = '<undefined>'
        try:
            c2 = u.encode(encoding2)
        except UnicodeError, reason:
            c2 = '<undefined>'
        if c1 != c2:
            print ' * encoding mismatch for 0x%04X: %-14r != %r' % \
                  (i, c1, c2)
            mismatch += 1
    # Check decoding
    for i in range(256):
        c = chr(i)
        try:
            u1 = c.decode(encoding1)
        except UnicodeError:
            u1 = u'<undefined>'
        try:
            u2 = c.decode(encoding2)
        except UnicodeError:
            u2 = u'<undefined>'
        if u1 != u2:
            print ' * decoding mismatch for 0x%04X: %-14r != %r' % \
                  (i, u1, u2)
            mismatch += 1
    if mismatch:
        print
        print 'Found %i mismatches' % mismatch
    else:
        print '-> Codecs are identical.'

if __name__ == '__main__':
    compare_codecs(sys.argv[1], sys.argv[2])
