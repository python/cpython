# uu.py
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
#
# This file implements the UUencode and UUdecode functions.

# encode(filename, mode, in_file, out_file)
# decode(filename, mode, in_file)
# decode(in_file, out_file)
# decode(in_file)

import binascii

# encode a fileobject and write out to a file object
def encode(filename, mode, in_file, out_file):
    out_file.write('begin %o %s\n' % ((mode&0777),filename))
    str = in_file.read(45)
    while len(str) > 0:
	out_file.write(binascii.b2a_uu(str))
	str = in_file.read(45)
    out_file.write(' \nend\n')
    return None


# decode(filename, mode, in_file)
# decode(in_file, out_file)
# decode(in_file)
def decode(*args):
    ok = 1
    _setup = None
    out_file = None
    if len(args) == 3:
	filename, mode, in_file = args
	if type(filename) != type(''):
	    ok = 0
	if type(mode) != type(0):
	    ok = 0
	try:
	    _ = getattr(in_file,'readline')
	except AttributeError:
	    ok = 0
	def _setup(out_file,args):
	    filename, mode, in_file = args
	    # open file as specified and assign out_file for later use
	    out_file = open(filename,'w',mode)
	    _out_file_orig = 0
	    _ = in_file.readline()
	    return (out_file,_out_file_orig)
    elif len(args) == 2:
	in_file, out_file = args
	try:
	    _ = getattr(in_file,'readline')
	    _ = getattr(out_file,'write')
	except AttributeError:
	    ok = 0
	def _setup(out_file, args):
	    in_file, out_file = args
	    # Toss the 'begin mode filename' part.. not needed
	    _ = in_file.readline()
	    _out_file_orig = 1
	    return (out_file,_out_file_orig)
    elif len(args) == 1:
	in_file = args[0]
	try:
	    _ = getattr(in_file,'readline')
	except AttributeError:
	    ok = 0
	def _setup(out_file, args):
	    import strop
	    in_file = args[0]
	    # open file as specified in uu file and 
	    # assign out_file for later use
	    i = in_file.readline()
	    i = strop.strip(i)
	    if 'begin' != i[:5]:
		raise IOError, 'input file not in UUencoded format'
	    [dummy, mode, filename] = strop.split(i)
	    mode = strop.atoi(mode, 8)
	    out_file = open(filename,'w',mode)
	    _out_file_orig = 0
	    return (out_file,_out_file_orig)
    if ok != 1:
	raise SyntaxError, 'must be (filename, mode, in_file) or (in_file,out_file) or (in_file)'
    out_file, _out_file_orig = _setup(out_file, args)
    str = in_file.readline()
    while len(str) > 0 and str != ' \n' and str != 'end\n':
	out_file.write(binascii.a2b_uu(str))
	str = in_file.readline()
    if _out_file_orig == 0:
	out_file.close()
	del out_file
    return None

def test():
    import sys
    if sys.argv[1:2] == ['-d']:
	if sys.argv[2:]:
	    decode(open(sys.argv[2]), sys.stdout)
	else:
	    decode(sys.stdin, sys.stdout)
    elif sys.argv[1:2] == ['-e']:
	if sys.argv[2:]:
	    file = sys.argv[2]
	    fp = open(file)
	else:
	    file = '-'
	    fp = sys.stdin
	encode(file, 0644, fp, sys.stdout)
    else:
	print 'usage: uu -d [file]; (to decode)'
	print 'or:    uu -e [file]; (to encode)'
	sys.exit(2)

if __name__ == '__main__':
    test()
