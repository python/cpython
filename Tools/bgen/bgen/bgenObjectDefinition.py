from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class ObjectDefinition(GeneratorGroup):
	"Spit out code that together defines a new Python object type"

	def __init__(self, name, prefix, itselftype):
		"""ObjectDefinition constructor.  May be extended, but do not override.
		
		- name: the object's official name, e.g. 'SndChannel'.
		- prefix: the prefix used for the object's functions and data, e.g. 'SndCh'.
		- itselftype: the C type actually contained in the object, e.g. 'SndChannelPtr'.
		
		XXX For official Python data types, rules for the 'Py' prefix are a problem.
		"""
		
		GeneratorGroup.__init__(self, prefix or name)
		self.name = name
		self.itselftype = itselftype
		self.objecttype = name + 'Object'
		self.typename = name + '_Type'
		self.argref = ""	# set to "*" if arg to <type>_New should be pointer
		self.static = "static " # set to "" to make <type>_New and <type>_Convert public

	def add(self, g):
		g.setselftype(self.objecttype, self.itselftype)
		GeneratorGroup.add(self, g)

	def reference(self):
		# In case we are referenced from a module
		pass

	def generate(self):
		# XXX This should use long strings and %(varname)s substitution!

		OutHeader2("Object type " + self.name)

		sf = self.static and "staticforward "
		Output("%sPyTypeObject %s;", sf, self.typename)
		Output()

		Output("#define %s_Check(x) ((x)->ob_type == &%s)",
		       self.prefix, self.typename)
		Output()

		Output("typedef struct %s {", self.objecttype)
		IndentLevel()
		Output("PyObject_HEAD")
		self.outputStructMembers()
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

	def outputStructMembers(self):
		Output("%s ob_itself;", self.itselftype)

	def outputNew(self):
		Output("%sPyObject *%s_New(itself)", self.static, self.prefix)
		IndentLevel()
		Output("const %s %sitself;", self.itselftype, self.argref)
		DedentLevel()
		OutLbrace()
		Output("%s *it;", self.objecttype)
		self.outputCheckNewArg()
		Output("it = PyObject_NEW(%s, &%s);", self.objecttype, self.typename)
		Output("if (it == NULL) return NULL;")
		self.outputInitStructMembers()
		Output("return (PyObject *)it;")
		OutRbrace()
		Output()

	def outputInitStructMembers(self):
		Output("it->ob_itself = %sitself;", self.argref)
	
	def outputCheckNewArg(self):
			"Override this method to apply additional checks/conversions"
	
	def outputConvert(self):
		Output("%s%s_Convert(v, p_itself)", self.static, self.prefix)
		IndentLevel()
		Output("PyObject *v;")
		Output("%s *p_itself;", self.itselftype)
		DedentLevel()
		OutLbrace()
		self.outputCheckConvertArg()
		Output("if (!%s_Check(v))", self.prefix)
		OutLbrace()
		Output('PyErr_SetString(PyExc_TypeError, "%s required");', self.name)
		Output("return 0;")
		OutRbrace()
		Output("*p_itself = ((%s *)v)->ob_itself;", self.objecttype)
		Output("return 1;")
		OutRbrace()

	def outputCheckConvertArg(self):
		"Override this method to apply additional conversions"

	def outputDealloc(self):
		Output("static void %s_dealloc(self)", self.prefix)
		IndentLevel()
		Output("%s *self;", self.objecttype)
		DedentLevel()
		OutLbrace()
		self.outputCleanupStructMembers()
		Output("PyMem_DEL(self);")
		OutRbrace()
		Output()

	def outputCleanupStructMembers(self):
		self.outputFreeIt("self->ob_itself")

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
		Output("%sPyTypeObject %s = {", self.static, self.typename)
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


class GlobalObjectDefinition(ObjectDefinition):
	"Same as ObjectDefinition but exports its New and Create methods"

	def __init__(self, name, prefix = None, itselftype = None):
		ObjectDefinition.__init__(self, name, prefix or name, itselftype or name)
		self.static = ""
