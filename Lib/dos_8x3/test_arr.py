#! /usr/bin/env python
"""Test the arraymodule.
Roger E. Masse
"""
import array

def testtype(type, example):

    
	a = array.array(type)
	a.append(example)
	#print 40*'*'
	#print 'array after append: ', a
	a.typecode
	a.itemsize
	if a.typecode in ('i', 'b', 'h', 'l'):
	    a.byteswap()

	if a.typecode == 'c':
	    f = open('/etc/passwd', 'r')
	    a.fromfile(f, 10)
	    #print 'char array with 10 bytes of /etc/passwd appended: ', a
	    a.fromlist(['a', 'b', 'c'])
	    #print 'char array with list appended: ', a

	a.insert(0, example)
	#print 'array of %s after inserting another:' % a.typecode, a
	f = open('/dev/null', 'w')
	a.tofile(f)
	a.tolist()
	a.tostring()
	#print 'array of %s converted to a list: ' % a.typecode, a.tolist()
	#print 'array of %s converted to a string: ' % a.typecode, a.tostring()

testtype('c', 'c')

for type in (['b', 'h', 'i', 'l', 'f', 'd']):
    testtype(type, 1)

	
