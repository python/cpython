r'''
This tests the '_objects' attribute of ctypes instances.  '_objects'
holds references to objects that must be kept alive as long as the
ctypes instance, to make sure that the memory buffer is valid.

WARNING: The '_objects' attribute is exposed ONLY for debugging ctypes itself,
it MUST NEVER BE MODIFIED!

'_objects' is initialized to a dictionary on first use, before that it
is None.

Here is an array of string pointers:

>>> from ctypes import *
>>> array = (c_char_p * 5)()
>>> print(array._objects)
None
>>>

The memory block stores pointers to strings, and the strings itself
assigned from Python must be kept.

>>> array[4] = b'foo bar'
>>> array._objects
{'4': b'foo bar'}
>>> array[4]
b'foo bar'
>>>

It gets more complicated when the ctypes instance itself is contained
in a 'base' object.

>>> class X(Structure):
...     _fields_ = [("x", c_int), ("y", c_int), ("array", c_char_p * 5)]
...
>>> x = X()
>>> print(x._objects)
None
>>>

The'array' attribute of the 'x' object shares part of the memory buffer
of 'x' ('_b_base_' is either None, or the root object owning the memory block):

>>> print(x.array._b_base_) # doctest: +ELLIPSIS
<ctypes.test.test_objects.X object at 0x...>
>>>

>>> x.array[0] = b'spam spam spam'
>>> x._objects
{'0:2': b'spam spam spam'}
>>> x.array._b_base_._objects
{'0:2': b'spam spam spam'}
>>>

'''

import unittest, doctest

import ctypes.test.test_objects

class TestCase(unittest.TestCase):
    def test(self):
        failures, tests = doctest.testmod(ctypes.test.test_objects)
        self.assertFalse(failures, 'doctests failed, see output above')

if __name__ == '__main__':
    doctest.testmod(ctypes.test.test_objects)
