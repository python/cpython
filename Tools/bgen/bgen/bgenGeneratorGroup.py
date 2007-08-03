from bgenOutput import *

class GeneratorGroup:

    def __init__(self, prefix):
        self.prefix = prefix
        self.generators = []

    def add(self, g, dupcheck=0):
        if dupcheck:
            if g in self.generators:
                print('DUP', g.name)
                return
        g.setprefix(self.prefix)
        self.generators.append(g)

    def generate(self):
        for g in self.generators:
            g.generate()
        Output()
        Output("static PyMethodDef %s_methods[] = {", self.prefix)
        IndentLevel()
        for g in self.generators:
            g.reference()
        Output("{NULL, NULL, 0}")
        DedentLevel()
        Output("};")


def _test():
    void = None
    from bgenGenerator import FunctionGenerator
    group = GeneratorGroup("spam")
    eggs = FunctionGenerator(void, "eggs")
    group.add(eggs)
    print("/* START */")
    group.generate()

if __name__ == "__main__":
    _test()
