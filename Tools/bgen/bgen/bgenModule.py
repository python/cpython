from bgenOutput import *
from bgenGeneratorGroup import GeneratorGroup

class Module(GeneratorGroup):

	def __init__(self, name, prefix = None,
		     includestuff = None,
		     initstuff = None):
		GeneratorGroup.__init__(self, prefix or name)
		self.name = name
		self.includestuff = includestuff
		self.initstuff = initstuff

	def addobject(self, od):
		self.generators.append(od)

	def generate(self):
		OutHeader1("Module " + self.name)
		Output("#include <Python.h>")
		Output()

		if self.includestuff:
			Output()
			Output("%s", self.includestuff)

		self.declareModuleVariables()

		GeneratorGroup.generate(self)

		Output()
		Output("void init%s()", self.name)
		OutLbrace()
		Output("object *m;")
		Output("object *d;")
		Output()

		if self.initstuff:
			Output("%s", self.initstuff)
			Output()

		Output("m = initmodule(\"%s\", %s_methods);",
		       self.name, self.prefix)
		Output("d = getmoduledict(m);")
		self.createModuleVariables()
		OutRbrace()
		OutHeader1("End module " + self.name)

	def declareModuleVariables(self):
		self.errorname = self.prefix + "_Error"
		Output("static object *%s;", self.errorname)

	def createModuleVariables(self):
		Output("""if ((%s = newstringobject("%s.Error")) == NULL ||""",
		               self.errorname,       self.name)
		Output("""    dictinsert(d, "Error", %s) != 0)""",
		                                     self.errorname)
		Output("""\tfatal("can't initialize %s.Error");""",
		                                    self.name)


def _test():
	from bgenGenerator import Generator
	m = Module("spam", "", "#include <stdio.h>")
	g = Generator(None, "bacon")
	m.add(g)
	m.generate()

if __name__ == "__main__":
	_test()
