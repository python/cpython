#! /usr/bin/env python

# Copyright 1994 by Lance Ellinghouse
# Cathedral City, California Republic, United States of America.
#                        All Rights Reserved
# Permission to use, copy, modify, and distribute this software and its 
# documentation for any purpose and without fee is hereby granted, 
# provided that the above copyright notice appear in all copies and that
# both that copyright notice and this permission notice appear in 
# supporting documentation, and that the name of Lance Ellinghouse
# not be used in advertising or publicity pertaining to distribution 
# of the software without specific, written prior permission.
# LANCE ELLINGHOUSE DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS, IN NO EVENT SHALL LANCE ELLINGHOUSE CENTRUM BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# Modified by Jack Jansen, CWI, July 1995:
# - Use binascii module to do the actual line-by-line conversion
#   between ascii and binary. This results in a 1000-fold speedup. The C
#   version is still 5 times faster, though.
# - Arguments more compliant with python standard

"""Implementation of the UUencode and UUdecode functions.

encode(in_file, out_file [,name, mode])
decode(in_file [, out_file, mode])
"""

import binascii
import os
import string
import sys

class Error(Exception):
    pass

def encode(in_file, out_file, name=None, mode=None):
    """Uuencode file"""
    #
    # If in_file is a pathname open it and change defaults
    #
    if in_file == '-':
        in_file = sys.stdin
    elif type(in_file) == type(''):
        if name == None:
            name = os.path.basename(in_file)
        if mode == None:
            try:
                mode = os.stat(in_file)[0]
            except AttributeError:
                pass
        in_file = open(in_file, 'rb')
    #
    # Open out_file if it is a pathname
    #
    if out_file == '-':
        out_file = sys.stdout
    elif type(out_file) == type(''):
        out_file = open(out_file, 'w')
    #
    # Set defaults for name and mode
    #
    if name == None:
        name = '-'
    if mode == None:
        mode = 0666
    #
    # Write the data
    #
    out_file.write('begin %o %s\n' % ((mode&0777),name))
    str = in_file.read(45)
    while len(str) > 0:
        out_file.write(binascii.b2a_uu(str))
        str = in_file.read(45)
    out_file.write(' \nend\n')


def decode(in_file, out_file=None, mode=None):
    """Decode uuencoded file"""
    #
    # Open the input file, if needed.
    #
    if in_file == '-':
        in_file = sys.stdin
    elif type(in_file) == type(''):
        in_file = open(in_file)
    #
    # Read until a begin is encountered or we've exhausted the file
    #
    while 1:
        hdr = in_file.readline()
        if not hdr:
            raise Error, 'No valid begin line found in input file'
        if hdr[:5] != 'begin':
            continue
        hdrfields = string.split(hdr)
        if len(hdrfields) == 3 and hdrfields[0] == 'begin':
            try:
                string.atoi(hdrfields[1], 8)
                break
            except ValueError:
                pass
    if out_file == None:
        out_file = hdrfields[2]
    if mode == None:
        mode = string.atoi(hdrfields[1], 8)
    #
    # Open the output file
    #
    if out_file == '-':
        out_file = sys.stdout
    elif type(out_file) == type(''):
        fp = open(out_file, 'wb')
        try:
            os.path.chmod(out_file, mode)
        except AttributeError:
            pass
        out_file = fp
    #
    # Main decoding loop
    #
    s = in_file.readline()
    while s and s != 'end\n':
        try:
            data = binascii.a2b_uu(s)
        except binascii.Error, v:
            # Workaround for broken uuencoders by /Fredrik Lundh
            nbytes = (((ord(s[0])-32) & 63) * 4 + 5) / 3
            data = binascii.a2b_uu(s[:nbytes])
            sys.stderr.write("Warning: %s\n" % str(v))
        out_file.write(data)
        s = in_file.readline()
    if not str:
        raise Error, 'Truncated input file'

def test():
    """uuencode/uudecode main program"""
    import getopt

    dopt = 0
    topt = 0
    input = sys.stdin
    output = sys.stdout
    ok = 1
    try:
        optlist, args = getopt.getopt(sys.argv[1:], 'dt')
    except getopt.error:
        ok = 0
    if not ok or len(args) > 2:
        print 'Usage:', sys.argv[0], '[-d] [-t] [input [output]]'
        print ' -d: Decode (in stead of encode)'
        print ' -t: data is text, encoded format unix-compatible text'
        sys.exit(1)
        
    for o, a in optlist:
        if o == '-d': dopt = 1
        if o == '-t': topt = 1

    if len(args) > 0:
        input = args[0]
    if len(args) > 1:
        output = args[1]

    if dopt:
        if topt:
            if type(output) == type(''):
                output = open(output, 'w')
            else:
                print sys.argv[0], ': cannot do -t to stdout'
                sys.exit(1)
        decode(input, output)
    else:
        if topt:
            if type(input) == type(''):
                input = open(input, 'r')
            else:
                print sys.argv[0], ': cannot do -t from stdin'
                sys.exit(1)
        encode(input, output)

if __name__ == '__main__':
    test()
