"""Functions."""

from framer import template
from framer.util import cstring, unindent

METH_O = "METH_O"
METH_NOARGS = "METH_NOARGS"
METH_VARARGS = "METH_VARARGS"

def parsefmt(fmt):
    for c in fmt:
        if c == '|':
            continue
        yield c

class Argument:

    def __init__(self, name):
        self.name = name
        self.ctype = "PyObject *"
        self.default = None

    def __str__(self):
        return "%s%s" % (self.ctype, self.name)

    def setfmt(self, code):
        self.ctype = self._codes[code]
        if self.ctype[-1] != "*":
            self.ctype += " "

    _codes = {"O": "PyObject *",
              "i": "int",
              }

    def decl(self):
        if self.default is None:
            return str(self) + ";"
        else:
            return "%s = %s;" % (self, self.default)

class _ArgumentList(object):

    # these instance variables should be initialized by subclasses
    ml_meth = None
    fmt = None

    def __init__(self, args):
        self.args = map(Argument, args)

    def __len__(self):
        return len(self.args)

    def __getitem__(self, i):
        return self.args[i]

    def dump_decls(self, f):
        pass

class NoArgs(_ArgumentList):

    def __init__(self, args):
        assert len(args) == 0
        super(NoArgs, self).__init__(args)
        self.ml_meth = METH_NOARGS

    def c_args(self):
        return "PyObject *self"

class OneArg(_ArgumentList):

    def __init__(self, args):
        assert len(args) == 1
        super(OneArg, self).__init__(args)
        self.ml_meth = METH_O

    def c_args(self):
        return "PyObject *self, %s" % self.args[0]

class VarArgs(_ArgumentList):

    def __init__(self, args, fmt=None):
        super(VarArgs, self).__init__(args)
        self.ml_meth = METH_VARARGS
        if fmt is not None:
            self.fmt = fmt
            i = 0
            for code in parsefmt(fmt):
                self.args[i].setfmt(code)
                i += 1

    def c_args(self):
        return "PyObject *self, PyObject *args"

    def targets(self):
        return ", ".join(["&%s" % a.name for a in self.args])

    def dump_decls(self, f):
        for a in self.args:
            print >> f, "        %s" % a.decl()

def ArgumentList(func, method):
    code = func.func_code
    args = code.co_varnames[:code.co_argcount]
    if method:
        args = args[1:]
    pyarg = getattr(func, "pyarg", None)
    if pyarg is not None:
        args = VarArgs(args, pyarg)
        if func.func_defaults:
            L = list(func.func_defaults)
            ndefault = len(L)
            i = len(args) - ndefault
            while L:
                args[i].default = L.pop(0)
        return args
    else:
        if len(args) == 0:
            return NoArgs(args)
        elif len(args) == 1:
            return OneArg(args)
        else:
            return VarArgs(args)

class Function:

    method = False

    def __init__(self, func, parent):
        self._func = func
        self._parent = parent
        self.analyze()
        self.initvars()

    def dump(self, f):
        def p(templ, vars=None): # helper function to generate output
            if vars is None:
                vars = self.vars
            print >> f, templ % vars

        if self.__doc__:
            p(template.docstring)

        d = {"name" : self.vars["CName"],
             "args" : self.args.c_args(),
             }
        p(template.funcdef_start, d)

        self.args.dump_decls(f)

        if self.args.ml_meth == METH_VARARGS:
            p(template.varargs)

        p(template.funcdef_end)

    def analyze(self):
        self.__doc__ = self._func.__doc__
        self.args = ArgumentList(self._func, self.method)

    def initvars(self):
        v = self.vars = {}
        v["PythonName"] = self._func.__name__
        s = v["CName"] = "%s_%s" % (self._parent.name, self._func.__name__)
        v["DocstringVar"] = s + "_doc"
        v["MethType"] = self.args.ml_meth
        if self.__doc__:
            v["Docstring"] = cstring(unindent(self.__doc__))
        if self.args.fmt is not None:
            v["ArgParse"] = self.args.fmt
            v["ArgTargets"] = self.args.targets()

class Method(Function):

    method = True
