"""Type and Variable classes and a modest collection of standard types."""

from bgenOutput import *


# Values to represent argument transfer modes
InMode    = 1 # input-only argument
OutMode   = 2 # output-only argument
InOutMode = 3 # input-output argument
ModeMask  = 3 # bits to keep for mode


# Special cases for mode/flags argument
# XXX This is still a mess!
SelfMode   =  4+InMode  # this is 'self' -- don't declare it
ReturnMode =  8+OutMode # this is the function return value
ErrorMode  = 16+OutMode # this is an error status -- turn it into an exception


class Type:

	"""Define the various things you can do with a C type.

	Most methods are intended to be extended or overridden.
	"""

	def __init__(self, typeName, fmt):
		"""Call with the C name and getargs format for the type.

		Example: int = Type("int", "i")
		"""
		self.typeName = typeName
		self.fmt = fmt

	def declare(self, name):
		"""Declare a variable of the type with a given name.

		Example: int.declare('spam') prints "int spam;"
		"""
		Output("%s %s;", self.typeName, name)

	def getargs(self):
		return self.getargsFormat(), self.getargsArgs()

	def getargsFormat(self):
		"""Return the format for this type for use with [new]getargs().

		Example: int.getargsFormat() returns the string "i".
		"""
		return self.fmt

	def getargsArgs(self, name):
		"""Return an argument for use with [new]getargs().

		Example: int.getargsArgs("spam") returns the string "&spam".
		"""
		return "&" + name

	def getargsCheck(self, name):
		"""Perform any needed post-[new]getargs() checks.

		This is type-dependent; the default does not check for errors.
		An example would be a check for a maximum string length."""

	def passInput(self, name):
		"""Return an argument for passing a variable into a call.

		Example: int.passInput("spam") returns the string "spam".
		"""
		return name

	def passOutput(self, name):
		"""Return an argument for returning a variable out of a call.

		Example: int.passOutput("spam") returns the string "&spam".
		"""
		return "&" + name

	def errorCheck(self, name):
		"""Check for an error returned in the variable.

		This is type-dependent; the default does not check for errors.
		An example would be a check for a NULL pointer.
		If an error is found, the generated routine should
		raise an exception and return NULL.

		XXX There should be a way to add error clean-up code.
		"""
		Output("/* XXX no err check for %s %s */", self.typeName, name)

	def mkvalue(self):
		return self.mkvalueFormat(), self.mkvalueArgs()

	def mkvalueFormat(self):
		"""Return the format for this type for use with mkvalue().

		This is normally the same as getargsFormat() but it is
		a separate function to allow future divergence.
		"""
		return self.getargsFormat()

	def mkvalueArgs(self, name):
		"""Return an argument for use with mkvalue().

		Example: int.mkvalueArgs("spam") returns the string "spam".
		"""
		return name

	def mkvalueCleanup(self, name):
		"""Clean up if necessary after mkvalue().

		This is normally empty; it may deallocate buffers etc.
		"""
		pass


# Sometimes it's useful to define a type that's only usable as input or output parameter

class InputOnlyMixIn:

	"Mix-in class to boobytrap passOutput"

	def passOutput(self):
		raise RuntimeError, "this type can only be used for input parameters"

class InputOnlyType(InputOnlyMixIn, Type):

	"Same as Type, but only usable for input parameters -- passOutput is boobytrapped"

class OutputOnlyMixIn:

	"Mix-in class to boobytrap passInput"

	def passInput(self):
		raise RuntimeError, "this type can only be used for output parameters"

class OutputOnlyType(OutputOnlyMixIn, Type):

	"Same as Type, but only usable for output parameters -- passInput is boobytrapped"


# A modest collection of standard C types.
void = None
char = Type("char", "c")
short = Type("short", "h")
int = Type("int", "i")
long = Type("long", "l")
float = Type("float", "f")
double = Type("double", "d")


# The most common use of character pointers is a null-terminated string.
# For input, this is easy.  For output, and for other uses of char *,
# see the various Buffer types below.
stringptr = InputOnlyType("char*", "s")


# Some Python related types.
objectptr = Type("PyObject*", "O")
stringobjectptr = Type("PyStringObject*", "S")
# Etc.


class FakeType(InputOnlyType):

	"""A type that is not represented in the Python version of the interface.

	Instantiate with a value to pass in the call.
	"""

	def __init__(self, substitute):
		self.substitute = substitute

	def declare(self, name):
		pass

	def getargsFormat(self):
		return ""

	def getargsArgs(self, name):
		return None

	def passInput(self, name):
		return self.substitute


class AbstractBufferType:
	"""Buffers are character arrays that may contain null bytes.

	There are a number of variants depending on:
	- how the buffer is allocated (for output buffers), and
	- whether and how the size is passed into and/or out of the called function.

	The AbstractbufferType only serves as a placeholder for this doc string.
	"""

	def declare(self, name):
		self.declareBuffer(name)
		self.declareSize(name)


class FixedBufferType(AbstractBufferType):

	"""Fixed size buffer -- passed without size information.

	Instantiate with the size as parameter.
	THIS IS STILL AN ABSTRACT BASE CLASS -- DO NOT INSTANTIATE.
	"""

	def __init__(self, size):
		self.size = str(size)

	def declareSize(self, name):
		Output("int %s__len__;", name)


class FixedInputBufferType(InputOnlyMixIn, FixedBufferType):

	"""Fixed size input buffer -- passed without size information.

	Instantiate with the size as parameter.
	"""

	def declareBuffer(self, name):
		Output("char *%s;", name)

	def getargsFormat(self):
		return "s#"

	def getargsArgs(self, name):
		return "&%s, &%s__len__" % (name, name)

	def getargsCheck(self, name):
		Output("if (%s__len__ != %s)", name, self.size)
		OutLbrace()
		Output('PyErr_SetString(PyExc_TypeError, "buffer length should be %s");',
		       self.size)
		Output('return NULL;') # XXX should do a goto
		OutRbrace()

	def passInput(self, name):
		return name


class FixedOutputBufferType(OutputOnlyMixIn, FixedBufferType):

	"""Fixed size output buffer -- passed without size information.

	Instantiate with the size as parameter.
	"""

	def declareBuffer(self, name):
		Output("char %s[%s];", name, self.size)

	def passOutput(self, name):
		return name

	def mkvalueFormat(self):
		return "s#"

	def mkvalueArgs(self):
		return "%s, %s" % (name, self.size)


class StructBufferType(FixedBufferType):

	"""Structure buffer -- passed as a structure pointer.

	Instantiate with the struct type as parameter.
	"""

	def __init__(self, type):
		FixedBufferType.__init__(self, "sizeof(%s)" % type)
		self.type = type


class StructInputBufferType(StructBufferType, FixedInputBufferType):

	"""Fixed size input buffer -- passed as a pointer to a structure.

	Instantiate with the struct type as parameter.
	"""

	def declareBuffer(self, name):
		Output("%s *%s;", self.type, name)


class StructByValueBufferType(StructInputBufferType):

	"""Fixed size input buffer -- passed as a structure BY VALUE.

	Instantiate with the struct type as parameter.
	"""

	def passInput(self, name):
		return "*%s" % name


class StructOutputBufferType(StructBufferType, FixedOutputBufferType):

	"""Fixed size output buffer -- passed as a pointer to a structure.

	Instantiate with the struct type as parameter.
	"""

	def declareBuffer(self, name):
		Output("%s %s;", self.type, name)

	def passOutput(self, name):
		return "&%s" % name

	def mkvalueArgs(self, name):
		return "(char *)&%s" % name


class VarInputBufferType(InputOnlyMixIn, FixedInputBufferType):

	"""Input buffer -- passed as (buffer, size).

	Instantiate without parameters.
	"""

	def __init__(self):
		pass

	def getargsCheck(self, name):
		pass

	def passInput(self, name):
		return "%s, %s__len__" % (name, name)


class StackOutputBufferType(OutputOnlyMixIn, FixedOutputBufferType):

	"""Output buffer -- passed as (buffer, size).

	Instantiate with the buffer size as parameter.
	"""

	def passOutput(self, name):
		return "%s, %s" % (name, self.size)


class VarStackOutputBufferType(StackOutputBufferType):

	"""Output buffer allocated on the stack -- passed as (buffer, &size).

	Instantiate with the buffer size as parameter.
	"""

	def declareSize(self, name):
		Output("int %s__len__ = %s;", name, self.size)

	def passOutput(self, name):
		return "%s, &%s__len__" % (name, name)

	def mkvalueArgs(self, name):
		return "%s, %s__len__" % (name, name)


class VarVarStackOutputBufferType(VarStackOutputBufferType):

	"""Output buffer allocated on the stack -- passed as (buffer, size, &size).

	Instantiate with the buffer size as parameter.
	"""

	def passOutput(self, name):
		return "%s, %s__len__, &%s__len__" % (name, name, name)


class HeapOutputBufferType(FixedOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""

	def __init__(self):
		pass

	def declareBuffer(self, name):
		Output("char *%s;", name)

	def getargsFormat(self):
		return "i"

	def getargsArgs(self, name):
		return "&%s__len__" % name

	def getargsCheck(self, name):
		Output("if ((%s = malloc(%s__len__)) == NULL) goto %s__error__;",
		             name,       name,                     name)

	def passOutput(self, name):
		return "%s, %s__len__" % (name, name)

	def mkvalueArgs(self, name):
		return "%s, %s__len__" % (name, name)

	def mkvalueCleanup(self, name):
		Output("free(%s);", name)
		DedentLevel()
		Output(" %s__error__: ;", name);
		IndentLevel()


class VarHeapOutputBufferType(HeapOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, &size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""

	def passOutput(self, name):
		return "%s, &%s__len__" % (name, name)


class VarVarHeapOutputBufferType(VarHeapOutputBufferType):

	"""Output buffer allocated on the heap -- passed as (buffer, size, &size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""

	def passOutput(self, name):
		return "%s, %s__len__, &%s__len__" % (name, name, name)


class StringBufferType:

	"""Mix-in class to create various string buffer types.

	Strings are character arrays terminated by a null byte.
	For input, this is already covered by stringptr.
	For output, there are again three variants:
	- Fixed (size is a constant given in the documentation),
	- Stack (size is passed to the C function but we decide on a size at
	  code generation time so we can still allocate on the heap), or
	- Heap (size is passed to the C function and we let the Python caller
	  pass a size.
	(Note that this doesn't cover output parameters in which a string
	pointer is returned.  These are actually easier (no allocation) but far
	less common.  I'll write the classes when there is demand.)
	"""

	def mkvalueFormat(self):
		return "s"

	def mkvalueArgs(self, name):
		return name


class FixedOutputStringType(StringBufferType, FixedOutputBufferType):

	"""Null-terminated output string -- passed without size.

	Instantiate with buffer size as parameter.
	"""


class StackOutputStringType(StringBufferType, StackOutputBufferType):

	"""Null-terminated output string -- passed as (buffer, size).

	Instantiate with buffer size as parameter.
	"""

class HeapOutputStringType(StringBufferType, HeapOutputBufferType):

	"""Null-terminated output string -- passed as (buffer, size).

	Instantiate without parameters.
	Call from Python with buffer size.
	"""


class OpaqueType(Type):

	"""A type represented by an opaque object type, always passed by address.

	Instantiate with the type name, and optional an object type name whose
	New/Convert functions will be used.
	"""

	def __init__(self, name, sameAs = None):
		self.typeName = name
		self.sameAs = sameAs or name

	def getargsFormat(self):
		return 'O&'

	def getargsArgs(self, name):
		return "%s_Convert, &%s" % (self.sameAs, name)

	def passInput(self, name):
		return "&%s" % name

	def mkvalueFormat(self):
		return 'O&'

	def mkvalueArgs(self, name):
		return "%s_New, &%s" % (self.sameAs, name)


class OpaqueByValueType(OpaqueType):

	"""A type represented by an opaque object type, on input passed BY VALUE.

	Instantiate with the type name, and optional an object type name whose
	New/Convert functions will be used.
	"""

	def passInput(self, name):
		return name

	def mkvalueArgs(self, name):
		return "%s_New, %s" % (self.sameAs, name)


class OpaqueArrayType(OpaqueByValueType):

	"""A type represented by an opaque object type, with ARRAY passing semantics.

	Instantiate with the type name, and optional an object type name whose
	New/Convert functions will be used.
	"""

	def getargsArgs(self, name):
		return "%s_Convert, &%s" % (self.sameAs, name)

	def passOutput(self, name):
		return name


class Variable:

	"""A Variable holds a type, a name, a transfer mode and flags.

	Most of its methods call the correponding type method with the
	variable name.
	"""

	def __init__(self, type, name = None, flags = InMode):
		"""Call with a type, a name and flags.

		If name is None, it muse be set later.
		flags defaults to InMode.
		"""
		self.type = type
		self.name = name
		self.flags = flags
		self.mode = flags & ModeMask

	def declare(self):
		"""Declare the variable if necessary.

		If it is "self", it is not declared.
		"""
		if self.flags != SelfMode:
			self.type.declare(self.name)

	def getargsFormat(self):
		"""Call the type's getargsFormatmethod."""
		return self.type.getargsFormat()

	def getargsArgs(self):
		"""Call the type's getargsArgsmethod."""
		return self.type.getargsArgs(self.name)

	def getargsCheck(self):
		return self.type.getargsCheck(self.name)

	def passArgument(self):
		"""Return the string required to pass the variable as argument.

		For "in" arguments, return the variable name.
		For "out" and "in out" arguments,
		return its name prefixed with "&".
		"""
		if self.mode == InMode:
			return self.type.passInput(self.name)
		if self.mode in (OutMode, InOutMode):
			return self.type.passOutput(self.name)
		# XXX Shouldn't get here
		return "/*mode?*/" + self.type.passInput(self.name)

	def errorCheck(self):
		"""Check for an error if necessary.

		This only generates code if the variable's mode is ErrorMode.
		"""
		if self.flags == ErrorMode:
			self.type.errorCheck(self.name)

	def mkvalueFormat (self):
		"""Call the type's mkvalueFormatmethod."""
		return self.type.mkvalueFormat()

	def mkvalueArgs(self):
		"""Call the type's mkvalueArgs method."""
		return self.type.mkvalueArgs(self.name)

	def mkvalueCleanup(self):
		"""Call the type's mkvalueCleanup method."""
		return self.type.mkvalueCleanup(self.name)
