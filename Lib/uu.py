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

# This file implements the UUencode and UUdecode functions.

# encode(filename, mode, in_file, out_file)
# decode(filename, mode, in_file)
# decode(in_file, out_file)
# decode(in_file)

# encode a single char to always be printable
def _ENC(ch):
    if type(ch) == type(''):
	a = ch[:1] # only 1 char
	if len(a) == 0:
	    raise ValueError, 'need to pass in at least 1 char'
	a = ord(a)
    elif type(ch) == type(0):
	a = ch
    else:
	raise TypeError, 'must pass in an integer or single character'
    return chr((a & 077) + ord(' '))

# input 3 chars, output 4 encoded chars
def _outenc(str):
    if len(str) > 3:
	raise ValueError, 'can only accept strings of 3 chars'
    p0, p1, p2 = 0, 0, 0
    if len(str) > 2:
	p2 = ord(str[2])
    if len(str) > 1:
	p1 = ord(str[1])
    if len(str) > 0:
	p0 = ord(str[0])
    c1 = p0 >> 2
    c2 = (p0 << 4) & 060 | (p1 >> 4) & 017
    c3 = (p1 << 2) & 074 | (p2 >> 6) & 03
    c4 = p2 & 077
    rtn = _ENC(c1)
    rtn = rtn + _ENC(c2)
    rtn = rtn + _ENC(c3)
    rtn = rtn + _ENC(c4)
    return rtn

# pass in 45 bytes max, returns 62 bytes encoded
def _encode(str):
    if len(str) > 45:
	raise ValueError, 'cannot handle more than 45 chars at once'
    length = len(str)
    rtn = _ENC(length)
    i = 0
    while i < length:
	rtn = rtn + _outenc(str[i:(i+3)])
	i = i + 3
    rtn = rtn + '\n'
    return rtn
    
# encode a fileobject and write out to a file object
def encode(filename, mode, in_file, out_file):
    out_file.write('begin %o %s\n' % ((mode&0777),filename))
    str = in_file.read(45)
    while len(str) > 0:
	out_file.write(_encode(str))
	str = in_file.read(45)
    out_file.write(' \nend\n')
    return None

# def decode a single char from printable to possibly non-printable
def _DEC(ch):
    if type(ch) != type('') or len(ch) != 1:
	raise ValueError, 'need to pass in a single char'
    a = ord(ch[0:1])
    return (a - ord(' '))

# input 4 chars encoded, output 3 chars unencoded
def _outdec(str):
    if len(str) > 4:
	raise ValueError, 'can only accept strings of 4 chars'
    p0, p1, p2, p3 = 0, 0, 0, 0
    if len(str) > 3:
	p3 = _DEC(str[3])
    if len(str) > 2:
	p2 = _DEC(str[2])
    if len(str) > 1:
	p1 = _DEC(str[1])
    if len(str) > 0:
	p0 = _DEC(str[0])
    c1 = p0 << 2 | (p1 & 060) >> 4
    c2 = (p1 & 017) << 4 | (p2 & 074) >> 2
    c3 = (p2 & 03) << 6 | (p3 & 077)
    rtn = chr(c1)
    rtn = rtn + chr(c2)
    rtn = rtn + chr(c3)
    return rtn

# pass in 62 bytes and return 45 bytes unencoded
def _decode(str):
    if len(str) > 62:
	raise ValueError, 'cannot handle more than 62 chars at once'
    length = _DEC(str[0])
    i = 1
    rtn = ''
    while len(rtn) < length:
	rtn = rtn + _outdec(str[i:(i+4)])
	i = i + 4
    return rtn[0:length]

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
	out_file.write(_decode(str))
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
