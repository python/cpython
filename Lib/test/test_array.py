#! /usr/bin/env python
"""Test the arraymodule.
   Roger E. Masse
"""
import array
from test_support import verbose

def main():

    testtype('c', 'c')

    for type in (['b', 'h', 'i', 'l', 'f', 'd']):
	testtype(type, 1)


def testtype(type, example):

    
	a = array.array(type)
	a.append(example)
	if verbose:
	    print 40*'*'
	    print 'array after append: ', a
	a.typecode
	a.itemsize
	if a.typecode in ('i', 'b', 'h', 'l'):
	    a.byteswap()

	if a.typecode == 'c':
	    f = open('/etc/passwd', 'r')
	    a.fromfile(f, 10)
	    if verbose:
		print 'char array with 10 bytes of /etc/passwd appended: ', a
	    a.fromlist(['a', 'b', 'c'])
	    if verbose:
		print 'char array with list appended: ', a

	a.insert(0, example)
	if verbose:
	    print 'array of %s after inserting another:' % a.typecode, a
	f = open('/dev/null', 'w')
	a.tofile(f)
	a.tolist()
	a.tostring()
	if verbose:
	    print 'array of %s converted to a list: ' % a.typecode, a.tolist()
	if verbose:
	    print 'array of %s converted to a string: ' \
	           % a.typecode, a.tostring()

main()
	
