# Buffers allocated on the heap

from bgenOutput import *
from bgenType import OutputOnlyMixIn
from bgenBuffer import FixedInputOutputBufferType


class HeapInputOutputBufferType(FixedInputOutputBufferType):

	"""Input-output buffer allocated on the heap -- passed as (inbuffer, outbuffer, size).

	Instantiate without parameters.
	Call from Python with input buffer.
	"""

	def __init__(self, datatype = 'char', sizetype = 'int', sizeformat = None):
		FixedInputOutputBufferType.__init__(self, "0", datatype, sizetype, sizeformat)

	def declareOutputBuffer(self, name):
		Output("%s *%s__out__;", self.datatype, name)

	def getargsCheck(self, name):
		Output("if ((%s__out__ = malloc(%s__in_len__)) == NULL)", name, name)
		OutLbrace()
		Output('PyErr_NoMemory();')
		Output("goto %s__error__;", name)
		OutRbrace()
		Output("%s__len__ = %s__in_len__;", name, name)

	def passOutput(self, name):
		return "%s__in__, %s__out__, (%s)%s__len__" % \
			(name, name, self.sizetype, name)

	def mkvalueArgs(self, name):
		return "%s__out__, (int)%s__len__" % (name, name)

	def cleanup(self, name):
		Output("free(%s__out__);", name)
		FixedInputOutputBufferType.cleanup(self, name)


class VarHeapInputOutputBufferType(HeapInputOutputBufferType):

	"""same as base class, but passed as (inbuffer, outbuffer, &size)"""
	
	def passOutput(self, name):
		return "%s__in__, %s__out__, &%s__len__" % (name, name, name)


class HeapCombinedInputOutputBufferType(HeapInputOutputBufferType):

	"""same as base class, but passed as (inoutbuffer, size)"""
	
	def passOutput(self, name):
		return "(%s *)memcpy(%s__out__, %s__in__, %s__len__)" % \
			(self.datatype, name,   name,     name)


class VarHeapCombinedInputOutputBufferType(HeapInputOutputBufferType):

	"""same as base class, but passed as (inoutbuffer, &size)"""
	
	def passOutput(self, name):
		return "(%s *)memcpy(%s__out__, %s__in__, &%s__len__)" % \
			(self.datatype, name,   name,      name)


class HeapOutputBufferType(OutputOnlyMixIn, HeapInputOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""
	
	def declareInputBuffer(self, name):
		pass
	
	def getargsFormat(self):
		return "i"
	
	def getargsArgs(self, name):
		return "&%s__in_len__" % name
	
	def passOutput(self, name):
		return "%s__out__, %s__len__" % (name, name)


class VarHeapOutputBufferType(HeapOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, &size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""

	def passOutput(self, name):
		return "%s__out__, &%s__len__" % (name, name)


class VarVarHeapOutputBufferType(VarHeapOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, size, &size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""

	def passOutput(self, name):
		return "%s__out__, %s__len__, &%s__len__" % (name, name, name)
