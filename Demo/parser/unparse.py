"Usage: unparse.py <path to source file>"
import sys

class Unparser:
    """Methods in this class recursively traverse an AST and
    output source code for the abstract syntax; original formatting
    is disregarged. """

    def __init__(self, tree, file = sys.stdout):
        """Unparser(tree, file=sys.stdout) -> None.
         Print the source for tree to file."""
        self.f = file
        self._indent = 0
        self.dispatch(tree)
        self.f.flush()

    def fill(self, text = ""):
        "Indent a piece of text, according to the current indentation level"
        self.f.write("\n"+"    "*self._indent + text)

    def write(self, text):
        "Append a piece of text to the current line."
        self.f.write(text)

    def enter(self):
        "Print ':', and increase the indentation."
        self.write(":")
        self._indent += 1

    def leave(self):
        "Decrease the indentation level."
        self._indent -= 1

    def dispatch(self, tree):
        "Dispatcher function, dispatching tree type T to method _T."
        if isinstance(tree, list):
            for t in tree:
                self.dispatch(t)
            return
        meth = getattr(self, "_"+tree.__class__.__name__)
        meth(tree)


    ############### Unparsing methods ######################
    # There should be one method per concrete grammar type #
    # Constructors should be grouped by sum type. Ideally, #
    # this would follow the order in the grammar, but      #
    # currently doesn't.                                   #
    ########################################################

    def _Module(self, tree):
        for stmt in tree.body:
            self.dispatch(stmt)

    # stmt
    def _Expr(self, tree):
        self.fill()
        self.dispatch(tree.value)

    def _Import(self, t):
        self.fill("import ")
        first = True
        for a in t.names:
            if first:
                first = False
            else:
                self.write(", ")
            self.write(a.name)
            if a.asname:
                self.write(" as "+a.asname)

    def _Assign(self, t):
        self.fill()
        for target in t.targets:
            self.dispatch(target)
            self.write(" = ")
        self.dispatch(t.value)

    def _ClassDef(self, t):
        self.write("\n")
        self.fill("class "+t.name)
        if t.bases:
            self.write("(")
            for a in t.bases:
                self.dispatch(a)
                self.write(", ")
            self.write(")")
        self.enter()
        self.dispatch(t.body)
        self.leave()

    def _FunctionDef(self, t):
        self.write("\n")
        self.fill("def "+t.name + "(")
        self.dispatch(t.args)
        self.enter()
        self.dispatch(t.body)
        self.leave()

    def _If(self, t):
        self.fill("if ")
        self.dispatch(t.test)
        self.enter()
        # XXX elif?
        self.dispatch(t.body)
        self.leave()
        if t.orelse:
            self.fill("else")
            self.enter()
            self.dispatch(t.orelse)
            self.leave()

    # expr
    def _Str(self, tree):
        self.write(repr(tree.s))

    def _Name(self, t):
        self.write(t.id)

    def _List(self, t):
        self.write("[")
        for e in t.elts:
            self.dispatch(e)
            self.write(", ")
        self.write("]")

    unop = {"Invert":"~", "Not": "not", "UAdd":"+", "USub":"-"}
    def _UnaryOp(self, t):
        self.write(self.unop[t.op.__class__.__name__])
        self.write("(")
        self.dispatch(t.operand)
        self.write(")")

    # others
    def _arguments(self, t):
        first = True
        # XXX t.defaults
        for a in t.args:
            if first:first = False
            else: self.write(", ")
            self.dispatch(a)
        if t.vararg:
            if first:first = False
            else: self.write(", ")
            self.write("*"+t.vararg)
        if t.kwarg:
            if first:first = False
            else: self.write(", ")
            self.write("**"+self.kwarg)
        self.write(")")

def roundtrip(filename):
    source = open(filename).read()
    tree = compile(source, filename, "exec", 0x400)
    Unparser(tree)

if __name__=='__main__':
    roundtrip(sys.argv[1])
