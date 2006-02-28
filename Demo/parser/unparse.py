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
        print >>self.f,""
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

    def _AugAssign(self, t):
        self.fill()
        self.dispatch(t.target)
        self.write(" "+self.binop[t.op.__class__.__name__]+"= ")
        self.dispatch(t.value)

    def _Return(self, t):
        self.fill("return ")
        if t.value:
            self.dispatch(t.value)

    def _Print(self, t):
        self.fill("print ")
        do_comma = False
        if t.dest:
            self.write(">>")
            self.dispatch(t.dest)
            do_comma = True
        for e in t.values:
            if do_comma:self.write(", ")
            else:do_comma=True
            self.dispatch(e)
        if not t.nl:
            self.write(",")

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

    def _For(self, t):
        self.fill("for ")
        self.dispatch(t.target)
        self.write(" in ")
        self.dispatch(t.iter)
        self.enter()
        self.dispatch(t.body)
        self.leave()
        if t.orelse:
            self.fill("else")
            self.enter()
            self.dispatch(t.orelse)
            self.leave

    # expr
    def _Str(self, tree):
        self.write(repr(tree.s))

    def _Name(self, t):
        self.write(t.id)

    def _Num(self, t):
        self.write(repr(t.n))

    def _List(self, t):
        self.write("[")
        for e in t.elts:
            self.dispatch(e)
            self.write(", ")
        self.write("]")

    def _Dict(self, t):
        self.write("{")
        for k,v in zip(t.keys, t.values):
            self.dispatch(k)
            self.write(" : ")
            self.dispatch(v)
            self.write(", ")
        self.write("}")

    def _Tuple(self, t):
        if not t.elts:
            self.write("()")
            return
        self.write("(")
        for e in t.elts:
            self.dispatch(e)
            self.write(", ")
        self.write(")")

    unop = {"Invert":"~", "Not": "not", "UAdd":"+", "USub":"-"}
    def _UnaryOp(self, t):
        self.write(self.unop[t.op.__class__.__name__])
        self.write("(")
        self.dispatch(t.operand)
        self.write(")")

    binop = { "Add":"+", "Sub":"-", "Mult":"*", "Div":"/", "Mod":"%",
                    "RShift":"<<", "BitOr":"|", "BitXor":"^", "BitAnd":"&",
                    "FloorDiv":"//"}
    def _BinOp(self, t):
        self.write("(")
        self.dispatch(t.left)
        self.write(")" + self.binop[t.op.__class__.__name__] + "(")
        self.dispatch(t.right)
        self.write(")")

    cmpops = {"Eq":"==", "NotEq":"!=", "Lt":"<", "LtE":"<=", "Gt":">", "GtE":">=",
                        "Is":"is", "IsNot":"is not", "In":"in", "NotIn":"not in"}
    def _Compare(self, t):
        self.write("(")
        self.dispatch(t.left)
        for o, e in zip(t.ops, t.comparators):
            self.write(") " +self.cmpops[o.__class__.__name__] + " (")
            self.dispatch(e)
            self.write(")")

    def _Attribute(self,t):
        self.dispatch(t.value)
        self.write(".")
        self.write(t.attr)

    def _Call(self, t):
        self.dispatch(t.func)
        self.write("(")
        comma = False
        for e in t.args:
            if comma: self.write(", ")
            else: comma = True
            self.dispatch(e)
        for e in t.keywords:
            if comma: self.write(", ")
            else: comma = True
            self.dispatch(e)
        if t.starargs:
            if comma: self.write(", ")
            else: comma = True
            self.write("*")
            self.dispatch(t.stararg)
        if t.kwargs:
            if comma: self.write(", ")
            else: comma = True
            self.write("**")
            self.dispatch(t.stararg)
        self.write(")")

    def _Subscript(self, t):
        self.dispatch(t.value)
        self.write("[")
        self.dispatch(t.slice)
        self.write("]")

    # slice
    def _Index(self, t):
        self.dispatch(t.value)

    def _Slice(self, t):
        if t.lower:
            self.dispatch(t.lower)
        self.write(":")
        if t.upper:
            self.dispatch(t.upper)
        if t.step:
            self.write(":")
            self.dispatch(t.step)

    # others
    def _arguments(self, t):
        first = True
        nonDef = len(t.args)-len(t.defaults)
        for a in t.args[0:nonDef]:
            if first:first = False
            else: self.write(", ")
            self.dispatch(a)
        for a,d in zip(t.args[nonDef:], t.defaults):
            if first:first = False
            else: self.write(", ")
            self.dispatch(a),
            self.write("=")
            self.dispatch(d)
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
