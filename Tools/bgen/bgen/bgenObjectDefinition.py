from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class ObjectDefinition(GeneratorGroup):

	def __init__(self, name, prefix, itselftype):
		import string
		GeneratorGroup.__init__(self, prefix or name)
		self.name = name
		self.itselftype = itselftype
		self.objecttype = name + 'Object'
		self.typename = name + '_Type'
		self.argref = ""	# set to "*" if arg to <type>_New should be pointer

	def add(self, g):
		g.setselftype(self.objecttype, self.itselftype)
		GeneratorGroup.add(self, g)

	def reference(self):
		# In case we are referenced from a module
		pass

	def generate(self):
		# XXX This should use long strings and %(varname)s substitution!

		OutHeader2("Object type " + self.name)

		Output("staticforward PyTypeObject %s;", self.typename)
		Output()

		Output("#define %s_Check(x) ((x)->ob_type == &%s)",
		       self.prefix, self.typename)
		Output()

		Output("typedef struct {")
		IndentLevel()
		Output("PyObject_HEAD")
		Output("%s ob_itself;", self.itselftype)
		DedentLevel()
		Output("} %s;", self.objecttype)
		Output()

		self.outputNew()
		
		self.outputConvert()

		self.outputDealloc()

		GeneratorGroup.generate(self)

		self.outputGetattr()

		self.outputSetattr()
		
		self.outputTypeObject()

		OutHeader2("End object type " + self.name)

	def outputNew(self):
		Output("static PyObject *%s_New(itself)", self.prefix)
		IndentLevel()
		Output("const %s %sitself;", self.itselftype, self.argref)
		DedentLevel()
		OutLbrace()
		Output("%s *it;", self.objecttype)
		self.outputCheckNewArg()
		Output("it = PyObject_NEW(%s, &%s);", self.objecttype, self.typename)
		Output("if (it == NULL) return NULL;")
		Output("it->ob_itself = %sitself;", self.argref)
		Output("return (PyObject *)it;")
		OutRbrace()
		Output()
	
	def outputCheckNewArg(self):
		pass

	def outputConvert(self):
		Output("""\
static int %(prefix)s_Convert(v, p_itself)
	PyObject *v;
	%(itselftype)s *p_itself;
{
	if (v == NULL || !%(prefix)s_Check(v)) {
		PyErr_SetString(PyExc_TypeError, "%(name)s required");
		return 0;
	}
	*p_itself = ((%(objecttype)s *)v)->ob_itself;
	return 1;
}
""" % self.__dict__)

	def outputDealloc(self):
		Output("static void %s_dealloc(self)", self.prefix)
		IndentLevel()
		Output("%s *self;", self.objecttype)
		DedentLevel()
		OutLbrace()
		self.outputFreeIt("self->ob_itself")
		Output("PyMem_DEL(self);")
		OutRbrace()
		Output()

	def outputFreeIt(self, name):
		Output("/* Cleanup of %s goes here */", name)

	def outputGetattr(self):
		Output("static PyObject *%s_getattr(self, name)", self.prefix)
		IndentLevel()
		Output("%s *self;", self.objecttype)
		Output("char *name;")
		DedentLevel()
		OutLbrace()
		self.outputGetattrBody()
		OutRbrace()
		Output()

	def outputGetattrBody(self):
		self.outputGetattrHook()
		Output("return Py_FindMethod(%s_methods, (PyObject *)self, name);", self.prefix)

	def outputGetattrHook(self):
		pass

	def outputSetattr(self):
		Output("#define %s_setattr NULL", self.prefix)
		Output()

	def outputTypeObject(self):
		Output("static PyTypeObject %s = {", self.typename)
		IndentLevel()
		Output("PyObject_HEAD_INIT(&PyType_Type)")
		Output("0, /*ob_size*/")
		Output("\"%s\", /*tp_name*/", self.name)
		Output("sizeof(%s), /*tp_basicsize*/", self.objecttype)
		Output("0, /*tp_itemsize*/")
		Output("/* methods */")
		Output("(destructor) %s_dealloc, /*tp_dealloc*/", self.prefix)
		Output("0, /*tp_print*/")
		Output("(getattrfunc) %s_getattr, /*tp_getattr*/", self.prefix)
		Output("(setattrfunc) %s_setattr, /*tp_setattr*/", self.prefix)
		DedentLevel()
		Output("};")
