#! /usr/bin/env python
"""Test the arraymodule.
   Roger E. Masse
"""
import array
from test_support import verbose, TESTFN, unlink, TestFailed

def main():

    testtype('c', 'c')

    for type in (['b', 'h', 'i', 'l', 'f', 'd']):
        testtype(type, 1)

    unlink(TESTFN)


def testoverflow(type, lowerLimit, upperLimit):
	# should not overflow assigning lower limit
	if verbose:
		print "overflow test: array(%s, [%s])" % (`type`, `lowerLimit`)
	try:
		a = array.array(type, [lowerLimit])
	except:
		raise TestFailed, "array(%s) overflowed assigning %s" %\
			(`type`, `lowerLimit`)
	# should overflow assigning less than lower limit
	if verbose:
		print "overflow test: array(%s, [%s])" % (`type`, `lowerLimit-1`)
	try:
		a = array.array(type, [lowerLimit-1])
		raise TestFailed, "array(%s) did not overflow assigning %s" %\
			(`type`, `lowerLimit-1`)
	except OverflowError:
		pass
	# should not overflow assigning upper limit
	if verbose:
		print "overflow test: array(%s, [%s])" % (`type`, `upperLimit`)
	try:
		a = array.array(type, [upperLimit])
	except:
		raise TestFailed, "array(%s) overflowed assigning %s" %\
			(`type`, `upperLimit`)
	# should overflow assigning more than upper limit
	if verbose:
		print "overflow test: array(%s, [%s])" % (`type`, `upperLimit+1`)
	try:
		a = array.array(type, [upperLimit+1])
		raise TestFailed, "array(%s) did not overflow assigning %s" %\
			(`type`, `upperLimit+1`)
	except OverflowError:
		pass



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
            f = open(TESTFN, "w")
            f.write("The quick brown fox jumps over the lazy dog.\n")
            f.close()
            f = open(TESTFN, 'r')
            a.fromfile(f, 10)
            f.close()
            if verbose:
                print 'char array with 10 bytes of TESTFN appended: ', a
            a.fromlist(['a', 'b', 'c'])
            if verbose:
                print 'char array with list appended: ', a

        a.insert(0, example)
        if verbose:
            print 'array of %s after inserting another:' % a.typecode, a
        f = open(TESTFN, 'w')
        a.tofile(f)
        f.close()
        a.tolist()
        a.tostring()
        if verbose:
            print 'array of %s converted to a list: ' % a.typecode, a.tolist()
        if verbose:
            print 'array of %s converted to a string: ' \
                   % a.typecode, `a.tostring()`

        if type == 'c':
            a = array.array(type, "abcde")
            a[:-1] = a
            if a != array.array(type, "abcdee"):
                raise TestFailed, "array(%s) self-slice-assign (head)" % `type`
            a = array.array(type, "abcde")
            a[1:] = a
            if a != array.array(type, "aabcde"):
                raise TestFailed, "array(%s) self-slice-assign (tail)" % `type`
            a = array.array(type, "abcde")
            a[1:-1] = a
            if a != array.array(type, "aabcdee"):
                raise TestFailed, "array(%s) self-slice-assign (cntr)" % `type`
            if a.index("e") != 5:
            	raise TestFailed, "array(%s) index-test" % `type`
            if a.count("a") != 2:
            	raise TestFailed, "array(%s) count-test" % `type`
            a.remove("e")
            if a != array.array(type, "aabcde"):
            	raise TestFailed, "array(%s) remove-test" % `type`
            if a.pop(0) != "a":
            	raise TestFailed, "array(%s) pop-test" % `type`
            if a.pop(1) != "b":
            	raise TestFailed, "array(%s) pop-test" % `type`
            a.extend(array.array(type, "xyz"))
            if a != array.array(type, "acdexyz"):
                raise TestFailed, "array(%s) extend-test" % `type`
            a.pop()
            a.pop()
            a.pop()
            x = a.pop()
            if x != 'e':
                raise TestFailed, "array(%s) pop-test" % `type`
            if a != array.array(type, "acd"):
            	raise TestFailed, "array(%s) pop-test" % `type`
            a.reverse()
            if a != array.array(type, "dca"):
            	raise TestFailed, "array(%s) reverse-test" % `type`
        else:
            a = array.array(type, [1, 2, 3, 4, 5])
            a[:-1] = a
            if a != array.array(type, [1, 2, 3, 4, 5, 5]):
                raise TestFailed, "array(%s) self-slice-assign (head)" % `type`
            a = array.array(type, [1, 2, 3, 4, 5])
            a[1:] = a
            if a != array.array(type, [1, 1, 2, 3, 4, 5]):
                raise TestFailed, "array(%s) self-slice-assign (tail)" % `type`
            a = array.array(type, [1, 2, 3, 4, 5])
            a[1:-1] = a
            if a != array.array(type, [1, 1, 2, 3, 4, 5, 5]):
                raise TestFailed, "array(%s) self-slice-assign (cntr)" % `type`
            if a.index(5) != 5:
            	raise TestFailed, "array(%s) index-test" % `type`
            if a.count(1) != 2:
            	raise TestFailed, "array(%s) count-test" % `type`
            a.remove(5)
            if a != array.array(type, [1, 1, 2, 3, 4, 5]):
            	raise TestFailed, "array(%s) remove-test" % `type`
            if a.pop(0) != 1:
            	raise TestFailed, "array(%s) pop-test" % `type`
            if a.pop(1) != 2:
            	raise TestFailed, "array(%s) pop-test" % `type`
            a.extend(array.array(type, [7, 8, 9]))
            if a != array.array(type, [1, 3, 4, 5, 7, 8, 9]):
                raise TestFailed, "array(%s) extend-test" % `type`
            a.pop()
            a.pop()
            a.pop()
            x = a.pop()
            if x != 5:
                raise TestFailed, "array(%s) pop-test" % `type`
            if a != array.array(type, [1, 3, 4]):
            	raise TestFailed, "array(%s) pop-test" % `type`
            a.reverse()
            if a != array.array(type, [4, 3, 1]):
            	raise TestFailed, "array(%s) reverse-test" % `type`

        # test that overflow exceptions are raised as expected for assignment
        # to array of specific integral types
        from math import pow
        if type in ('b', 'h', 'i', 'l'):
            # check signed and unsigned versions
            a = array.array(type)
            signedLowerLimit = -1 * long(pow(2, a.itemsize * 8 - 1))
            signedUpperLimit = long(pow(2, a.itemsize * 8 - 1)) - 1L
            unsignedLowerLimit = 0
            unsignedUpperLimit = long(pow(2, a.itemsize * 8)) - 1L
            testoverflow(type, signedLowerLimit, signedUpperLimit)
            testoverflow(type.upper(), unsignedLowerLimit, unsignedUpperLimit)
			
			
		
main()

