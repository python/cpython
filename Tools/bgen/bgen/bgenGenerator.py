from bgenOutput import *
from bgenType import *


Error = "bgenGenerator.Error"


# Strings to specify argument transfer modes in generator calls
IN = "in"
OUT = "out"
INOUT = IN_OUT = "in-out"


class Generator:

	# XXX There are some funny things with the argument list.
	# XXX It would be better to get rid of this and require
	# XXX each argument to be a type object or a tuple of the form
	# XXX (inoutmode, typeobject, argumentname)
	# XXX or possibly even a Variable() instance...

	def __init__(self, *argumentList):
		apply(self.parseArguments, argumentList)
		self.prefix     = "XXX"    # Will be changed by setprefix() call
		self.objecttype = "PyObject" # Type of _self argument to function
		self.itselftype = None     # Type of _self->ob_itself, if defined

	def setprefix(self, prefix):
		self.prefix = prefix

	def setselftype(self, selftype, itselftype):
		self.objecttype = selftype
		self.itselftype = itselftype

	def parseArguments(self, returnType, name, *argumentList):
		if returnType:
			self.rv = Variable(returnType, "_rv", OutMode)
		else:
			self.rv = None
		self.name = name
		self.argumentList = []
		if self.rv:
			self.argumentList.append(rv)
		self.parseArgumentList(argumentList)

	def parseArgumentList(self, argumentList):
		from types import *
		iarg = 0
		for arg in argumentList:
			# Arguments can either be:
			# - a type
			# - a tuple (type, [name, [mode]])
			# - a variable
			iarg = iarg + 1
			if hasattr(arg, 'typeName'):
				arg = Variable(arg)
			elif type(arg) == TupleType:
				arg = apply(Variable, arg)
			if arg.name is None:
				arg.name = "_arg%d" % iarg
			self.argumentList.append(arg)

	def reference(self, name = None):
		if name is None:
			name = self.name
		Output("{\"%s\", (PyCFunction)%s_%s, 1},",
		       name, self.prefix, self.name)

	def generate(self):
		self.functionheader()
		self.declarations()
		self.getargs()
		self.callit()
		self.checkit()
		self.returnvalue()
		self.functiontrailer()

	def functionheader(self):
		Output()
		Output("static PyObject *%s_%s(_self, _args)",
		       self.prefix, self.name)
		IndentLevel()
		Output("%s *_self;", self.objecttype)
		Output("PyObject *_args;")
		DedentLevel()
		OutLbrace()

	def declarations(self):
		for arg in self.argumentList:
			arg.declare()

	def getargs(self):
		fmt = ""
		lst = ""
		sep = ",\n" + ' '*len("if (!PyArg_ParseTuple(")
		for arg in self.argumentList:
			if arg.flags == SelfMode:
				continue
			if arg.mode in (InMode, InOutMode):
				fmt = fmt + arg.getargsFormat()
				lst = lst + sep + arg.getargsArgs()
		Output("if (!PyArg_ParseTuple(_args, \"%s\"%s))", fmt, lst)
		IndentLevel()
		Output("return NULL;")
		DedentLevel()
		for arg in self.argumentList:
			if arg.flags == SelfMode:
				continue
			if arg.mode in (InMode, InOutMode):
				arg.getargsCheck()

	def callit(self):
		args = ""
		sep = ",\n" + ' '*len("%s = %s(" % (self.rv.name, self.name))
		for arg in self.argumentList:
			if arg is self.rv:
				continue
			s = arg.passArgument()
			if args: s = sep + s
			args = args + s
		if self.rv:
			Output("%s = %s(%s);",
			       self.rv.name, self.name, args)
		else:
			Output("%s(%s);", self.name, args)

	def checkit(self):
		for arg in self.argumentList:
			arg.errorCheck()

	def returnvalue(self):
		fmt = ""
		lst = ""
		sep = ",\n" + ' '*len("return Py_BuildValue(")
		for arg in self.argumentList:
			if not arg: continue
			if arg.flags == ErrorMode: continue
			if arg.mode in (OutMode, InOutMode):
				fmt = fmt + arg.mkvalueFormat()
				lst = lst + sep + arg.mkvalueArgs()
		if fmt == "":
			Output("Py_INCREF(Py_None);")
			Output("return Py_None;");
		else:
			Output("return Py_BuildValue(\"%s\"%s);", fmt, lst)

	def functiontrailer(self):
		OutRbrace()


class ManualGenerator(Generator):

	def __init__(self, name, body):
		self.name = name
		self.body = body

	def definition(self):
		self.functionheader()
		Output("%s", self.body)
		self.functiontrailer()


def _test():
	void = None
	eggs = Generator(void, "eggs",
	                 Variable(stringptr, 'cmd'),
	                 Variable(int, 'x'),
	                 Variable(double, 'y', InOutMode),
	                 Variable(int, 'status', ErrorMode),
	                )
	eggs.setprefix("spam")
	print "/* START */"
	eggs.generate()

if __name__ == "__main__":
	_test()
