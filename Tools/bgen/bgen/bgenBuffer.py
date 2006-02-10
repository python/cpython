"""Buffers are character arrays that may contain null bytes.

There are a number of variants depending on:
- how the buffer is allocated (for output buffers), and
- whether and how the size is passed into and/or out of the called function.
"""


from bgenType import Type, InputOnlyMixIn, OutputOnlyMixIn, InputOnlyType, OutputOnlyType
from bgenOutput import *


# Map common types to their format characters
type2format = {
    'long': 'l',
    'int': 'i',
    'short': 'h',
    'char': 'b',
    'unsigned long': 'l',
    'unsigned int': 'i',
    'unsigned short': 'h',
    'unsigned char': 'b',
}


# ----- PART 1: Fixed character buffers -----


class FixedInputOutputBufferType(InputOnlyType):

    """Fixed buffer -- passed as (inbuffer, outbuffer)."""

    def __init__(self, size, datatype = 'char', sizetype = 'int', sizeformat = None):
        self.typeName = "Buffer"
        self.size = str(size)
        self.datatype = datatype
        self.sizetype = sizetype
        self.sizeformat = sizeformat or type2format[sizetype]
        self.label_needed = 0

    def getArgDeclarations(self, name, reference=False, constmode=False, outmode=False):
        if reference:
            raise RuntimeError, "Cannot pass buffer types by reference"
        return (self.getBufferDeclarations(name, constmode, outmode) +
                self.getSizeDeclarations(name, outmode))

    def getBufferDeclarations(self, name, constmode=False, outmode=False):
        return self.getInputBufferDeclarations(name, constmode) + \
                self.getOutputBufferDeclarations(name, constmode, outmode)

    def getInputBufferDeclarations(self, name, constmode=False):
        if constmode:
            const = "const "
        else:
            const = ""
        return ["%s%s *%s__in__" % (const, self.datatype, name)]

    def getOutputBufferDeclarations(self, name, constmode=False, outmode=False):
        if constmode:
            raise RuntimeError, "Cannot use const output buffer"
        if outmode:
            out = "*"
        else:
            out = ""
        return ["%s%s %s__out__[%s]" % (self.datatype, out, name, self.size)]

    def getSizeDeclarations(self, name, outmode=False):
        if outmode:
            out = "*"
        else:
            out = ""
        return ["%s%s %s__len__" %(self.sizetype, out, name)]

    def getAuxDeclarations(self, name):
        return ["int %s__in_len__" %(name)]

    def getargsFormat(self):
        return "s#"

    def getargsArgs(self, name):
        return "&%s__in__, &%s__in_len__" % (name, name)

    def getargsCheck(self, name):
        Output("if (%s__in_len__ != %s)", name, self.size)
        OutLbrace()
        Output('PyErr_SetString(PyExc_TypeError, "buffer length should be %s");',
               self.size)
        Output("goto %s__error__;", name)
        self.label_needed = 1
        OutRbrace()
        self.transferSize(name)

    def transferSize(self, name):
        Output("%s__len__ = %s__in_len__;", name, name)

    def passOutput(self, name):
        return "%s__in__, %s__out__" % (name, name)

    def mkvalueFormat(self):
        return "s#"

    def mkvalueArgs(self, name):
        return "%s__out__, (int)%s" % (name, self.size)

    def cleanup(self, name):
        if self.label_needed:
            DedentLevel()
            Output(" %s__error__: ;", name)
            IndentLevel()


class FixedCombinedInputOutputBufferType(FixedInputOutputBufferType):

    """Like fixed buffer -- but same parameter is input and output."""

    def passOutput(self, name):
        return "(%s *)memcpy(%s__out__, %s__in__, %s)" % \
            (self.datatype, name,   name,     self.size)


class InputOnlyBufferMixIn(InputOnlyMixIn):

    def getOutputBufferDeclarations(self, name, constmode=False, outmode=False):
        return []


class OutputOnlyBufferMixIn(OutputOnlyMixIn):

    def getInputBufferDeclarations(self, name, constmode=False):
        return []

class OptionalInputBufferMixIn:

    """Add to input buffers if the buffer may be omitted: pass None in Python
    and the C code will get a NULL pointer and zero size"""

    def getargsFormat(self):
        return "z#"


class FixedInputBufferType(InputOnlyBufferMixIn, FixedInputOutputBufferType):

    """Fixed size input buffer -- passed without size information.

    Instantiate with the size as parameter.
    """

    def passInput(self, name):
        return "%s__in__" % name

class OptionalFixedInputBufferType(OptionalInputBufferMixIn, FixedInputBufferType):
    pass

class FixedOutputBufferType(OutputOnlyBufferMixIn, FixedInputOutputBufferType):

    """Fixed size output buffer -- passed without size information.

    Instantiate with the size as parameter.
    """

    def passOutput(self, name):
        return "%s__out__" % name


class VarInputBufferType(FixedInputBufferType):

    """Variable size input buffer -- passed as (buffer, size).

    Instantiate without size parameter.
    """

    def __init__(self, datatype = 'char', sizetype = 'int', sizeformat = None):
        FixedInputBufferType.__init__(self, "0", datatype, sizetype, sizeformat)

    def getargsCheck(self, name):
        Output("%s__len__ = %s__in_len__;", name, name)

    def passInput(self, name):
        return "%s__in__, %s__len__" % (name, name)

class ReverseInputBufferMixin:
    """ Mixin for input buffers that are passed as (size, buffer) """

    def passInput(self, name):
        return "%s__len__, %s__in__" % (name, name)

class OptionalVarInputBufferType(OptionalInputBufferMixIn, VarInputBufferType):
    pass

# ----- PART 2: Structure buffers -----


class StructInputOutputBufferType(FixedInputOutputBufferType):

    """Structure buffer -- passed as a structure pointer.

    Instantiate with the struct type as parameter.
    """

    def __init__(self, type):
        FixedInputOutputBufferType.__init__(self, "sizeof(%s)" % type)
        self.typeName = self.type = type

    def getInputBufferDeclarations(self, name, constmode=False):
        if constmode:
            const = "const "
        else:
            const = ""
        return ["%s%s *%s__in__" % (const, self.type, name)]

    def getSizeDeclarations(self, name, outmode=False):
        return []

    def getAuxDeclarations(self, name):
        return ["int %s__in_len__" % (name)]

    def getOutputBufferDeclarations(self, name, constmode=False, outmode=False):
        if constmode:
            raise RuntimeError, "Cannot use const output buffer"
        if outmode:
            out = "*"
        else:
            out = ""
        return ["%s%s %s__out__" % (self.type, out, name)]

    def getargsArgs(self, name):
        return "(char **)&%s__in__, &%s__in_len__" % (name, name)

    def transferSize(self, name):
        pass

    def passInput(self, name):
        return "%s__in__" % name

    def passOutput(self, name):
        return "%s__in__, &%s__out__" % (name, name)

    def mkvalueArgs(self, name):
        return "(char *)&%s__out__, (int)%s" % (name, self.size)


class StructCombinedInputOutputBufferType(StructInputOutputBufferType):

    """Like structure buffer -- but same parameter is input and output."""

    def passOutput(self, name):
        return "(%s *)memcpy((char *)%s__out__, (char *)%s__in__, %s)" % \
            (self.type,          name,              name,     self.size)


class StructInputBufferType(InputOnlyBufferMixIn, StructInputOutputBufferType):

    """Fixed size input buffer -- passed as a pointer to a structure.

    Instantiate with the struct type as parameter.
    """


class StructByValueBufferType(StructInputBufferType):

    """Fixed size input buffer -- passed as a structure BY VALUE.

    Instantiate with the struct type as parameter.
    """

    def passInput(self, name):
        return "*%s__in__" % name


class StructOutputBufferType(OutputOnlyBufferMixIn, StructInputOutputBufferType):

    """Fixed size output buffer -- passed as a pointer to a structure.

    Instantiate with the struct type as parameter.
    """

    def getSizeDeclarations(self, name, outmode=False):
        return []

    def getAuxDeclarations(self, name):
        return []

    def passOutput(self, name):
        return "&%s__out__" % name


class ArrayOutputBufferType(OutputOnlyBufferMixIn, StructInputOutputBufferType):

    """Fixed size output buffer -- declared as a typedef, passed as an array.

    Instantiate with the struct type as parameter.
    """

    def getSizeDeclarations(self, name, outmode=False):
        return []

    def getAuxDeclarations(self, name):
        return []

    def passOutput(self, name):
        return "%s__out__" % name
