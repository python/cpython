from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class Module(GeneratorGroup):

	def __init__(self, name, prefix = None,
		     includestuff = None,
		     finalstuff = None,
		     initstuff = None,
		     variablestuff = None):
		GeneratorGroup.__init__(self, prefix or name)
		self.name = name
		self.includestuff = includestuff
		self.initstuff = initstuff
		self.finalstuff = finalstuff
		self.variablestuff = variablestuff
		self.typeobjects = []

	def addobject(self, od):
		self.generators.append(od)
		self.typeobjects.append(od)

	def generate(self):
		OutHeader1("Module " + self.name)
		Output("#include \"Python.h\"")
		Output()

		if self.includestuff:
			Output()
			Output("%s", self.includestuff)

		self.declareModuleVariables()

		GeneratorGroup.generate(self)
		
		if self.finalstuff:
			Output()
			Output("%s", self.finalstuff)

		Output()
		Output("void init%s()", self.name)
		OutLbrace()
		Output("PyObject *m;")
		Output("PyObject *d;")
		Output()

		if self.initstuff:
			Output("%s", self.initstuff)
			Output()

		Output("m = Py_InitModule(\"%s\", %s_methods);",
		       self.name, self.prefix)
		Output("d = PyModule_GetDict(m);")
		self.createModuleVariables()
		OutRbrace()
		OutHeader1("End module " + self.name)

	def declareModuleVariables(self):
		self.errorname = self.prefix + "_Error"
		Output("static PyObject *%s;", self.errorname)

	def createModuleVariables(self):
		Output("""%s = %s;""", self.errorname, self.exceptionInitializer())
		Output("""if (%s == NULL ||""", self.errorname)
		Output("""    PyDict_SetItemString(d, "Error", %s) != 0)""",
		                                               self.errorname)
		IndentLevel()
		Output("""Py_FatalError("can't initialize %s.Error");""",
		                                           self.name)
		DedentLevel()
		for tp in self.typeobjects:
			tp.outputTypeObjectInitializer()
		if self.variablestuff:
			Output("%s", self.variablestuff)
			Output()

	def exceptionInitializer(self):
		return """PyErr_NewException("%s.Error", NULL, NULL)""" % self.name


def _test():
	from bgenGenerator import Generator
	m = Module("spam", "", "#include <stdio.h>")
	g = Generator(None, "bacon")
	m.add(g)
	m.generate()

if __name__ == "__main__":
	_test()
