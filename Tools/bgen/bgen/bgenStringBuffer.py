"""Buffers used to hold null-terminated strings."""


from bgenBuffer import FixedOutputBufferType
from bgenStackBuffer import StackOutputBufferType
from bgenHeapBuffer import HeapOutputBufferType


class StringBufferMixIn:

    """Mix-in class to create various string buffer types.

    Strings are character arrays terminated by a null byte.
    (For input, this is also covered by stringptr.)
    For output, there are again three variants:
    - Fixed: size is a constant given in the documentation; or
    - Stack: size is passed to the C function but we decide on a size at
      code generation time so we can still allocate on the heap); or
    - Heap: size is passed to the C function and we let the Python caller
      pass a size.
    (Note that this doesn't cover output parameters in which a string
    pointer is returned.  These are actually easier (no allocation) but far
    less common.  I'll write the classes when there is demand.)
    """

    def declareSize(self, name):
        pass

    def getargsFormat(self):
        return "s"

    def getargsArgs(self, name):
        return "&%s__in__" % name

    def mkvalueFormat(self):
        return "s"

    def mkvalueArgs(self, name):
        return "%s__out__" % name


class FixedOutputStringType(StringBufferMixIn, FixedOutputBufferType):

    """Null-terminated output string -- passed without size.

    Instantiate with buffer size as parameter.
    """


class StackOutputStringType(StringBufferMixIn, StackOutputBufferType):

    """Null-terminated output string -- passed as (buffer, size).

    Instantiate with buffer size as parameter.
    """


class HeapOutputStringType(StringBufferMixIn, HeapOutputBufferType):

    """Null-terminated output string -- passed as (buffer, size).

    Instantiate without parameters.
    Call from Python with buffer size.
    """
