from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class ObjectDefinition(GeneratorGroup):
	"Spit out code that together defines a new Python object type"
	basechain = "NULL"

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

	def add(self, g, dupcheck=0):
		g.setselftype(self.objecttype, self.itselftype)
		GeneratorGroup.add(self, g, dupcheck)

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

		self.outputNew()
		
		self.outputConvert()

		self.outputDealloc()

		GeneratorGroup.generate(self)

		Output()
		Output("%sPyMethodChain %s_chain = { %s_methods, %s };",
		        self.static,    self.prefix, self.prefix, self.basechain)

		self.outputGetattr()

		self.outputSetattr()
		
		self.outputCompare()
		
		self.outputRepr()
		
		self.outputHash()
		
		self.outputTypeObject()

		OutHeader2("End object type " + self.name)

	def outputStructMembers(self):
		Output("%s ob_itself;", self.itselftype)

	def outputNew(self):
		Output()
		Output("%sPyObject *%s_New(%s %sitself)", self.static, self.prefix,
				self.itselftype, self.argref)
		OutLbrace()
		Output("%s *it;", self.objecttype)
		self.outputCheckNewArg()
		Output("it = PyObject_NEW(%s, &%s);", self.objecttype, self.typename)
		Output("if (it == NULL) return NULL;")
		self.outputInitStructMembers()
		Output("return (PyObject *)it;")
		OutRbrace()

	def outputInitStructMembers(self):
		Output("it->ob_itself = %sitself;", self.argref)
	
	def outputCheckNewArg(self):
			"Override this method to apply additional checks/conversions"
	
	def outputConvert(self):
		Output("%sint %s_Convert(PyObject *v, %s *p_itself)", self.static, self.prefix,
				self.itselftype)
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
		Output()
		Output("static void %s_dealloc(%s *self)", self.prefix, self.objecttype)
		OutLbrace()
		self.outputCleanupStructMembers()
		Output("PyMem_DEL(self);")
		OutRbrace()

	def outputCleanupStructMembers(self):
		self.outputFreeIt("self->ob_itself")

	def outputFreeIt(self, name):
		Output("/* Cleanup of %s goes here */", name)

	def outputGetattr(self):
		Output()
		Output("static PyObject *%s_getattr(%s *self, char *name)", self.prefix, self.objecttype)
		OutLbrace()
		self.outputGetattrBody()
		OutRbrace()

	def outputGetattrBody(self):
		self.outputGetattrHook()
		Output("return Py_FindMethodInChain(&%s_chain, (PyObject *)self, name);",
		       self.prefix)

	def outputGetattrHook(self):
		pass

	def outputSetattr(self):
		Output()
		Output("#define %s_setattr NULL", self.prefix)

	def outputCompare(self):
		Output()
		Output("#define %s_compare NULL", self.prefix)

	def outputRepr(self):
		Output()
		Output("#define %s_repr NULL", self.prefix)

	def outputHash(self):
		Output()
		Output("#define %s_hash NULL", self.prefix)

	def outputTypeObject(self):
		sf = self.static and "staticforward "
		Output()
		Output("%sPyTypeObject %s = {", sf, self.typename)
		IndentLevel()
		Output("PyObject_HEAD_INIT(NULL)")
		Output("0, /*ob_size*/")
		Output("\"%s\", /*tp_name*/", self.name)
		Output("sizeof(%s), /*tp_basicsize*/", self.objecttype)
		Output("0, /*tp_itemsize*/")
		Output("/* methods */")
		Output("(destructor) %s_dealloc, /*tp_dealloc*/", self.prefix)
		Output("0, /*tp_print*/")
		Output("(getattrfunc) %s_getattr, /*tp_getattr*/", self.prefix)
		Output("(setattrfunc) %s_setattr, /*tp_setattr*/", self.prefix)
		Output("(cmpfunc) %s_compare, /*tp_compare*/", self.prefix)
		Output("(reprfunc) %s_repr, /*tp_repr*/", self.prefix)
		Output("(PyNumberMethods *)0, /* tp_as_number */")
		Output("(PySequenceMethods *)0, /* tp_as_sequence */")
		Output("(PyMappingMethods *)0, /* tp_as_mapping */")
		Output("(hashfunc) %s_hash, /*tp_hash*/", self.prefix)
		DedentLevel()
		Output("};")
		
	def outputTypeObjectInitializer(self):
		Output("""%s.ob_type = &PyType_Type;""", self.typename);
		Output("""Py_INCREF(&%s);""", self.typename);
		Output("""if (PyDict_SetItemString(d, "%sType", (PyObject *)&%s) != 0)""",
			self.name, self.typename);
		IndentLevel()
		Output("""Py_FatalError("can\'t initialize %sType");""",
		                                           self.name)
		DedentLevel()



class GlobalObjectDefinition(ObjectDefinition):
	"""Like ObjectDefinition but exports some parts.
	
	XXX Should also somehow generate a .h file for them.
	"""

	def __init__(self, name, prefix = None, itselftype = None):
		ObjectDefinition.__init__(self, name, prefix or name, itselftype or name)
		self.static = ""

class ObjectIdentityMixin:
	"""A mixin class for objects that makes the identity of ob_itself
	govern comparisons and dictionary lookups. Useful if the C object can
	be returned by library calls and it is difficult (or impossible) to find
	the corresponding Python objects. With this you can create Python object
	wrappers on the fly"""
	
	def outputCompare(self):
		Output()
		Output("static int %s_compare(%s *self, %s *other)", self.prefix, self.objecttype,
				self.objecttype)
		OutLbrace()
		Output("unsigned long v, w;")
		Output()
		Output("if (!%s_Check((PyObject *)other))", self.prefix)
		OutLbrace()
		Output("v=(unsigned long)self;")
		Output("w=(unsigned long)other;")
		OutRbrace()
		Output("else")
		OutLbrace()
		Output("v=(unsigned long)self->ob_itself;")
		Output("w=(unsigned long)other->ob_itself;")
		OutRbrace()
		Output("if( v < w ) return -1;")
		Output("if( v > w ) return 1;")
		Output("return 0;")
		OutRbrace()
		
	def outputHash(self):
		Output()
		Output("static long %s_hash(%s *self)", self.prefix, self.objecttype)
		OutLbrace()
		Output("return (long)self->ob_itself;")
		OutRbrace()
		

