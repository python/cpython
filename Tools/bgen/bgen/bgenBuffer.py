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

	def declare(self, name):
		self.declareBuffer(name)
		self.declareSize(name)
	
	def declareBuffer(self, name):
		self.declareInputBuffer(name)
		self.declareOutputBuffer(name)
	
	def declareInputBuffer(self, name):
		Output("%s *%s__in__;", self.datatype, name)
	
	def declareOutputBuffer(self, name):
		Output("%s %s__out__[%s];", self.datatype, name, self.size)

	def declareSize(self, name):
		Output("%s %s__len__;", self.sizetype, name)

	def getargsFormat(self):
		# XXX This only works if the size is int-sized!!!
		return "s#"

	def getargsArgs(self, name):
		return "&%s__in__, &%s__len__" % (name, name)
	
	def getargsCheck(self, name):
		Output("if (%s__len__ != %s)", name, self.size)
		OutLbrace()
		Output('PyErr_SetString(PyExc_TypeError, "buffer length should be %s");',
		       self.size)
		Output("goto %s__error__;", name)
		OutRbrace()

	def passOutput(self, name):
		return "%s__in__, %s__out__" % (name, name)
	
	def mkvalueFormat(self):
		return "s#"

	def mkvalueArgs(self, name):
		return "%s__out__, %s" % (name, self.size)
	
	def cleanup(self, name):
		DedentLevel()
		Output(" %s__error__: ;", name)
		IndentLevel()


class FixedCombinedInputOutputBufferType(FixedInputOutputBufferType):
	
	"""Like fixed buffer -- but same parameter is input and output."""
	
	def passOutput(self, name):
		return "(%s *)memcpy(%s__out__, %s__in__, %s)" % \
			(self.datatype, name,   name,     self.size)


class InputOnlyBufferMixIn(InputOnlyMixIn):

	def declareOutputBuffer(self, name):
		pass


class OutputOnlyBufferMixIn(OutputOnlyMixIn):

	def declareInputBuffer(self, name):
		pass


class FixedInputBufferType(InputOnlyBufferMixIn, FixedInputOutputBufferType):

	"""Fixed size input buffer -- passed without size information.

	Instantiate with the size as parameter.
	"""

	def passInput(self, name):
		return "%s__in__" % name


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
		pass
	
	def passInput(self, name):
		return "%s__in__, %s__len__" % (name, name)


# ----- PART 2: Structure buffers -----


class StructInputOutputBufferType(FixedInputOutputBufferType):
	
	"""Structure buffer -- passed as a structure pointer.

	Instantiate with the struct type as parameter.
	"""
	
	def __init__(self, type):
		FixedInputOutputBufferType.__init__(self, "sizeof(%s)" % type)
		self.typeName = self.type = type
	
	def declareInputBuffer(self, name):
		Output("%s *%s__in__;", self.type, name)
	
	def declareOutputBuffer(self, name):
		Output("%s %s__out__;", self.type, name)
	
	def getargsArgs(self, name):
		return "(char **)&%s__in__, &%s__len__" % (name, name)
	
	def passInput(self, name):
		return "%s__in__" % name
	
	def passOutput(self, name):
		return "%s__in__, &%s__out__" % (name, name)
	
	def mkvalueArgs(self, name):
		return "(char *)&%s__out__, %s" % (name, self.size)


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

	def passOutput(self, name):
		return "&%s__out__" % name


class ArrayOutputBufferType(OutputOnlyBufferMixIn, StructInputOutputBufferType):

	"""Fixed size output buffer -- declared as a typedef, passed as an array.

	Instantiate with the struct type as parameter.
	"""

	def passOutput(self, name):
		return "%s__out__" % name
