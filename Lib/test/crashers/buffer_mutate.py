#
# The various methods of bufferobject.c (here buffer_subscript()) call
# get_buf() before calling potentially more Python code (here via
# PySlice_GetIndicesEx()).  But get_buf() already returned a void*
# pointer.  This void* pointer can become invalid if the object
# underlying the buffer is mutated (here a bytearray object).
#
# As usual, please keep in mind that the three "here" in the sentence
# above are only examples.  Each can be changed easily and lead to
# another crasher.
#
# This crashes for me on Linux 32-bits with CPython 2.6 and 2.7
# with a segmentation fault.
#


class PseudoIndex(object):
    def __index__(self):
        for c in "foobar"*n:
            a.append(c)
        return n * 4


for n in range(1, 100000, 100):
    a = bytearray("test"*n)
    buf = buffer(a)

    s = buf[:PseudoIndex():1]
    #print repr(s)
    #assert s == "test"*n
