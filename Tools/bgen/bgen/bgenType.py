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


# A modest collection of standard C types.
void = None
short = Type("short", "h")
int = Type("int", "i")
long = Type("long", "l")
float = Type("float", "f")
double = Type("double", "d")
stringptr = Type("char*", "s")
char = Type("char", "c")


# Some Python related types.
objectptr = Type("PyObject*", "O")
stringobjectptr = Type("PyStringObject*", "S")
# Etc.


# Buffers are character arrays that may contain null bytes.
# Their length is either Fixed or Sized (i.e. given by a separate argument).

class SizedInputBuffer:

	"Sized input buffer -- buffer size is an input parameter"

	def __init__(self, size = ''):
		self.size = size

	def declare(self, name):
		Output("char *%s;", name)
		Output("int %s__len__;", name)

	def getargsFormat(self):
		return "s#"

	def getargsArgs(self, name):
		return "&%s, &%s__len__" % (name, name)

	def getargsCheck(self, name):
		pass

	def passInput(self, name):
		return "%s, %s__len__" % (name, name)


class FixedInputBuffer(SizedInputBuffer):

	"Fixed input buffer -- buffer size is a constant"

	def getargsCheck(self, name):
		Output("if (%s__len__ != %s)", name, str(self.size))
		OutLbrace()
		Output('PyErr_SetString(PyExc_TypeError, "bad string length");')
		Output('return NULL;')
		OutRbrace()

	def passInput(self, name):
		return name


class RecordBuffer(FixedInputBuffer):

	"Like fixed input buffer -- but declared as a record type pointer"

	def __init__(self, type):
		self.type = type
		self.size = "sizeof(%s)" % type

	def declare(self, name):
		Output("%s *%s;", self.type, name)
		Output("int %s__len__;", name)
	

class SizedOutputBuffer:

	"Sized output buffer -- buffer size is an input-output parameter"

	def __init__(self, maxsize):
		self.maxsize = maxsize

	def declare(self, name):
		Output("char %s[%s];", name, str(self.maxsize))
		Output("int %s__len__ = %s;", name, str(self.maxsize))

	def passOutput(self, name):
		return "%s, &%s__len__" % (name, name)

	def errorCheck(self, name):
		pass

	def mkvalueFormat(self):
		return "s#"

	def mkvalueArgs(self, name):
		return "%s, %s__len__" % (name, name)


class VarSizedOutputBuffer(SizedOutputBuffer):

	"Variable Sized output buffer -- buffer size is an input and an output parameter"

	def getargsFormat(self):
		return "i"
		
	def getargsArgs(self, name):
		return "&%s__len__" % name
		
	def getargsCheck(self, name):
		pass

	def passOutput(self, name):
		return "%s, %s__len__, &%s__len__" % (name, name, name)


class FixedOutputBuffer:

	"Fixed output buffer -- buffer size is a constant"

	def __init__(self, size):
		self.size = size

	def declare(self, name):
		Output("char %s[%s];", name, str(self.size))

	def passOutput(self, name):
		return name

	def errorCheck(self, name):
		pass

	def mkvalueFormat(self):
		return "s#"

	def mkvalueArgs(self, name):
		return "%s, %s" % (name, str(self.size))


# Strings are character arrays terminated by a null byte.
# For input, this is covered by stringptr.
# For output, there are again two variants: Fixed (size is a constant
# given in the documentation) or Sized (size is given by a variable).
# (Note that this doesn't cover output parameters in which a string
# pointer is returned.)

class SizedOutputString:

	"Null-terminated output string -- buffer size is an input parameter"

	def __init__(self, bufsize):
		self.bufsize = bufsize

	def declare(self, name):
		Output("char %s[%s];", name, str(self.bufsize))

	def passOutput(self, name):
		return "%s, %s" % (name, str(self.bufsize))

	def errorCheck(self, name):
		pass

	def mkvalueFormat(self):
		return "s"

	def mkvalueArgs(self, name):
		return name


class FixedOutputString(SizedOutputString):

	"Null-terminated output string -- buffer size is a constant"

	def passOutput(self, name):
		return name


class StructureByValue:

	"Structure passed by value -- mapped to a Python string (for now)"

	def __init__(self, typeName):
		self.typeName = typeName

	def declare(self, name):
		n = len(self.typeName)
		Output("%-*s %s;",        n, self.typeName, name)
		Output("%-*s*%s__str__;", n, "char",        name)
		Output("%-*s %s__len__;", n, "int",         name)

	def getargsFormat(self):
		return "s#"

	def getargsArgs(self, name):
		return "&%s__str__, &%s__len__" % (name, name)

	def getargsCheck(self, name):
		Output("if (%s__len__ != sizeof %s)", name, name)
		IndentLevel()
		Output('PyErr_SetString(PyExc_TypeError, "bad structure length");')
		DedentLevel()
		Output("memcpy(&%s, %s__str__, %s__len__);",
		       name, name, name)

	def passInput(self, name):
		return "%s" % name

	def passOutput(self, name):
		return "&%s" % name

	def errorCheck(self, name):
		pass

	def mkvalueFormat(self):
		return "s#"

	def mkvalueArgs(self, name):
		return "(char *)&%s, (int)sizeof %s" % (name, name)


class StructureByAddress(StructureByValue):

	def passInput(self, name):
		return "&%s" % name


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
