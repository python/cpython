#! /usr/bin/env python
"""Test the arraymodule.
   Roger E. Masse
"""
import array
from test_support import verbose, TESTFN, unlink, TestFailed,\
     have_unicode, vereq

def main():
    testtype('c', 'c')
    if have_unicode:
        testtype('u', unicode(r'\u263a', 'unicode-escape'))
    for type in (['b', 'h', 'i', 'l', 'f', 'd']):
        testtype(type, 1)
    if have_unicode:
        testunicode()
    testsubclassing()
    unlink(TESTFN)

def testunicode():
    try:
        array.array('b', unicode('foo', 'ascii'))
    except TypeError:
        pass
    else:
        raise TestFailed("creating a non-unicode array from "
                         "a Unicode string should fail")

    x = array.array('u', unicode(r'\xa0\xc2\u1234', 'unicode-escape'))
    x.fromunicode(unicode(' ', 'ascii'))
    x.fromunicode(unicode('', 'ascii'))
    x.fromunicode(unicode('', 'ascii'))
    x.fromunicode(unicode(r'\x11abc\xff\u1234', 'unicode-escape'))
    s = x.tounicode()
    if s != unicode(r'\xa0\xc2\u1234 \x11abc\xff\u1234', 'unicode-escape'):
        raise TestFailed("fromunicode()/tounicode()")

    s = unicode(r'\x00="\'a\\b\x80\xff\u0000\u0001\u1234', 'unicode-escape')
    a = array.array('u', s)
    if verbose:
        print "repr of type 'u' array:", repr(a)
        print "              expected: array('u', %r)" % s

def testsubclassing():
    class EditableString(array.array):
        def __new__(cls, s, *args, **kwargs):
            return array.array.__new__(cls, 'c', s)

        def __init__(self, s, color='blue'):
            array.array.__init__(self, 'c', s)
            self.color = color

        def strip(self):
            self[:] = array.array('c', self.tostring().strip())

        def __repr__(self):
            return 'EditableString(%r)' % self.tostring()

    s = EditableString("\ttest\r\n")
    s.strip()
    if s.tostring() != 'test':
        raise TestFailed, "subclassing array.array failed somewhere"
    if s.color != 'blue':
        raise TestFailed, "assigning attributes to instance of array subclass"
    s.color = 'red'
    if s.color != 'red':
        raise TestFailed, "assigning attributes to instance of array subclass"
    if s.__dict__.keys() != ['color']:
        raise TestFailed, "array subclass __dict__"

    class ExaggeratingArray(array.array):
        __slots__ = ['offset']

        def __new__(cls, typecode, data, offset):
            return array.array.__new__(cls, typecode, data)

        def __init__(self, typecode, data, offset):
            self.offset = offset

        def __getitem__(self, i):
            return array.array.__getitem__(self, i) + self.offset

    a = ExaggeratingArray('i', [3, 6, 7, 11], 4)
    if a[0] != 7:
        raise TestFailed, "array subclass overriding __getitem__"
    try:
        a.color = 'blue'
    except AttributeError:
        pass
    else:
        raise TestFailed, "array subclass __slots__ was ignored"


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

    # This block is just to verify that the operations don't blow up.
    a.tolist()
    a.tostring()
    repr(a)
    str(a)

    if verbose:
        print 'array of %s converted to a list: ' % a.typecode, a.tolist()
    if verbose:
        print 'array of %s converted to a string: ' \
               % a.typecode, `a.tostring()`

    # Try out inplace addition and multiplication
    a = array.array(type, [example])
    b = a
    a += array.array(type, [example]*2)
    if a is not b:
        raise TestFailed, "array(%s) inplace addition" % `type`
    if a != array.array(type, [example] * 3):
        raise TestFailed, "array(%s) inplace addition" % `type`

    a *= 5
    if a is not b:
        raise TestFailed, "array(%s) inplace multiplication" % `type`
    if a != array.array(type, [example] * 15):
        raise TestFailed, "array(%s) inplace multiplication" % `type`

    a *= 0
    if a is not b:
        raise TestFailed, "array(%s) inplace multiplication by 0" % `type`
    if a != array.array(type, []):
        raise TestFailed, "array(%s) inplace multiplication by 0" % `type`

    a *= 1000
    if a is not b:
        raise TestFailed, "empty array(%s) inplace multiplication" % `type`
    if a != array.array(type, []):
        raise TestFailed, "empty array(%s) inplace multiplication" % `type`

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
    elif type == 'u':
        a = array.array(type, unicode("abcde", 'ascii'))
        a[:-1] = a
        if a != array.array(type, unicode("abcdee", 'ascii')):
            raise TestFailed, "array(%s) self-slice-assign (head)" % `type`
        a = array.array(type, unicode("abcde", 'ascii'))
        a[1:] = a
        if a != array.array(type, unicode("aabcde", 'ascii')):
            raise TestFailed, "array(%s) self-slice-assign (tail)" % `type`
        a = array.array(type, unicode("abcde", 'ascii'))
        a[1:-1] = a
        if a != array.array(type, unicode("aabcdee", 'ascii')):
            raise TestFailed, "array(%s) self-slice-assign (cntr)" % `type`
        if a.index(unicode("e", 'ascii')) != 5:
            raise TestFailed, "array(%s) index-test" % `type`
        if a.count(unicode("a", 'ascii')) != 2:
            raise TestFailed, "array(%s) count-test" % `type`
        a.remove(unicode("e", 'ascii'))
        if a != array.array(type, unicode("aabcde", 'ascii')):
            raise TestFailed, "array(%s) remove-test" % `type`
        if a.pop(0) != unicode("a", 'ascii'):
            raise TestFailed, "array(%s) pop-test" % `type`
        if a.pop(1) != unicode("b", 'ascii'):
            raise TestFailed, "array(%s) pop-test" % `type`
        a.extend(array.array(type, unicode("xyz", 'ascii')))
        if a != array.array(type, unicode("acdexyz", 'ascii')):
            raise TestFailed, "array(%s) extend-test" % `type`
        a.pop()
        a.pop()
        a.pop()
        x = a.pop()
        if x != unicode('e', 'ascii'):
            raise TestFailed, "array(%s) pop-test" % `type`
        if a != array.array(type, unicode("acd", 'ascii')):
            raise TestFailed, "array(%s) pop-test" % `type`
        a.reverse()
        if a != array.array(type, unicode("dca", 'ascii')):
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
        # extended slicing
        # subscription
        a = array.array(type, [0,1,2,3,4])
        vereq(a[::], a)
        vereq(a[::2], array.array(type, [0,2,4]))
        vereq(a[1::2], array.array(type, [1,3]))
        vereq(a[::-1], array.array(type, [4,3,2,1,0]))
        vereq(a[::-2], array.array(type, [4,2,0]))
        vereq(a[3::-2], array.array(type, [3,1]))
        vereq(a[-100:100:], a)
        vereq(a[100:-100:-1], a[::-1])
        vereq(a[-100L:100L:2L], array.array(type, [0,2,4]))
        vereq(a[1000:2000:2], array.array(type, []))
        vereq(a[-1000:-2000:-2], array.array(type, []))
        #  deletion
        del a[::2]
        vereq(a, array.array(type, [1,3]))
        a = array.array(type, range(5))
        del a[1::2]
        vereq(a, array.array(type, [0,2,4]))
        a = array.array(type, range(5))
        del a[1::-2]
        vereq(a, array.array(type, [0,2,3,4]))
        #  assignment
        a = array.array(type, range(10))
        a[::2] = array.array(type, [-1]*5)
        vereq(a, array.array(type, [-1, 1, -1, 3, -1, 5, -1, 7, -1, 9]))
        a = array.array(type, range(10))
        a[::-4] = array.array(type, [10]*3)
        vereq(a, array.array(type, [0, 10, 2, 3, 4, 10, 6, 7, 8 ,10]))
        a = array.array(type, range(4))
        a[::-1] = a
        vereq(a, array.array(type, [3, 2, 1, 0]))
        a = array.array(type, range(10))
        b = a[:]
        c = a[:]
        ins = array.array(type, range(2))
        a[2:3] = ins
        b[slice(2,3)] = ins
        c[2:3:] = ins

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
