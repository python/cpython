from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class ObjectDefinition(GeneratorGroup):

	def __init__(self, name, prefix, itselftype):
		import string
		GeneratorGroup.__init__(self, prefix or name)
		self.name = name
		self.itselftype = itselftype
		self.objecttype = name + 'Object'
		self.typename = self.prefix + '_' + \
				string.upper(name[:1]) + \
		                string.lower(name[1:]) + '_Type'

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

		Output("#define is_%sobject(x) ((x)->ob_type == %s)",
		       self.name, self.typename)
		Output()

		Output("typedef struct {")
		IndentLevel()
		Output("OB_HEAD")
		Output("%s ob_itself;", self.itselftype)
		DedentLevel()
		Output("} %s;", self.objecttype)
		Output()

		Output("static %s *new%s(itself)", self.objecttype, self.objecttype)
		IndentLevel()
		Output("%s itself;", self.itselftype)
		DedentLevel()
		OutLbrace()
		Output("%s *it;", self.objecttype)
		Output("it = PyObject_NEW(%s, &%s);", self.objecttype, self.typename)
		Output("if (it == NULL) return NULL;")
		Output("it->ob_itself = itself;")
		Output("return it;")
		OutRbrace()
		Output()

		Output("static void %s_dealloc(self)", self.prefix)
		IndentLevel()
		Output("%s *self;", self.objecttype)
		DedentLevel()
		OutLbrace()
##		Output("if (self->ob_itself != NULL)")
##		OutLbrace()
##		self.outputFreeIt("self->ob_itself")
##		OutRbrace()
		Output("DEL(self);")
		OutRbrace()
		Output()

##		Output("static int %s_converter(p_itself, p_it)", self.prefix)
##		IndentLevel()
##		Output("%s *p_itself;", self.itselftype)
##		Output("%s **p_it;", self.objecttype)
##		DedentLevel()
##		OutLbrace()
##		Output("if (**p_it == NULL)")
##		OutLbrace()
##		Output("*p_it = new%s(*p_itself);", self.objecttype)
##		OutRbrace()
##		Output("else")
##		OutLbrace()
##		Output("*p_itself = (*p_it)->ob_itself;")
##		OutRbrace()
##		Output("return 1;")
##		OutRbrace()
##		Output()

		GeneratorGroup.generate(self)

		self.outputGetattr()

		self.outputSetattr()

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

		OutHeader2("End object type " + self.name)

	def outputFreeIt(self, name):
		Output("DEL(%s); /* XXX */", name)

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
		Output("return findmethod(self, %s_methods, name);", self.prefix)

	def outputSetattr(self):
		Output("#define %s_setattr NULL", self.prefix)
		Output()
