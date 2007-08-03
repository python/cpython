"""Provides the Module and Type base classes that user code inherits from."""

__all__ = ["Module", "Type", "member"]

from framer import struct, template
from framer.function import Function, Method
from framer.member import member
from framer.slots import *
from framer.util import cstring, unindent

from types import FunctionType

def sortitems(dict):
    L = dict.items()
    L.sort()
    return L

# The Module and Type classes are implemented using metaclasses,
# because most of the methods are class methods.  It is easier to use
# metaclasses than the cumbersome classmethod() builtin.  They have
# class methods because they are exposed to user code as base classes.

class BaseMetaclass(type):
    """Shared infrastructure for generating modules and types."""

    # just methoddef so far

    def dump_methoddef(self, f, functions, vars):
        def p(templ, vars=vars): # helper function to generate output
            print(templ % vars, file=f)

        if not functions:
            return
        p(template.methoddef_start)
        for name, func in sortitems(functions):
            if func.__doc__:
                p(template.methoddef_def_doc, func.vars)
            else:
                p(template.methoddef_def, func.vars)
        p(template.methoddef_end)

class ModuleMetaclass(BaseMetaclass):
    """Provides methods for Module class."""

    def gen(self):
        self.analyze()
        self.initvars()
        f = open(self.__filename, "w")
        self.dump(f)
        f.close()

    def analyze(self):
        self.name = getattr(self, "abbrev", self.__name__)
        self.__functions = {}
        self.__types = {}
        self.__members = False

        for name, obj in self.__dict__.iteritems():
            if isinstance(obj, FunctionType):
                self.__functions[name] = Function(obj, self)
            elif isinstance(obj, TypeMetaclass):
                obj._TypeMetaclass__module = self.name
                obj.analyze()
                self.__types[name] = obj
                if obj.has_members():
                    self.__members = True

    def initvars(self):
        v = self.__vars = {}
        filename = getattr(self, "__file__", None)
        if filename is None:
            filename = self.__name__ + "module.c"
        self.__filename = v["FileName"] = filename
        name = v["ModuleName"] = self.__name__
        v["MethodDefName"] = "%s_methods" % name
        v["ModuleDocstring"] = cstring(unindent(self.__doc__))

    def dump(self, f):
        def p(templ, vars=self.__vars): # helper function to generate output
            print(templ % vars, file=f)

        p(template.module_start)
        if self.__members:
            p(template.member_include)
        print(file=f)

        if self.__doc__:
            p(template.module_doc)

        for name, type in sortitems(self.__types):
            type.dump(f)

        for name, func  in sortitems(self.__functions):
            func.dump(f)

        self.dump_methoddef(f, self.__functions, self.__vars)

        p(template.module_init_start)
        for name, type in sortitems(self.__types):
            type.dump_init(f)

        p("}")

class Module:
    __metaclass__ = ModuleMetaclass

class TypeMetaclass(BaseMetaclass):

    def dump(self, f):
        self.initvars()

        # defined after initvars() so that __vars is defined
        def p(templ, vars=self.__vars):
            print(templ % vars, file=f)

        if self.struct is not None:
            print(unindent(self.struct, False), file=f)

        if self.__doc__:
            p(template.docstring)

        for name, func in sortitems(self.__methods):
            func.dump(f)

        self.dump_methoddef(f, self.__methods, self.__vars)
        self.dump_memberdef(f)
        self.dump_slots(f)

    def has_members(self):
        if self.__members:
            return True
        else:
            return False

    def analyze(self):
        # called by ModuleMetaclass analyze()
        self.name = getattr(self, "abbrev", self.__name__)
        src = getattr(self, "struct", None)
        if src is not None:
            self.__struct = struct.parse(src)
        else:
            self.__struct = None
        self.__methods = {}
        self.__members = {}
        for cls in self.__mro__:
            for k, v in cls.__dict__.iteritems():
                if isinstance(v, FunctionType):
                    self.__methods[k] = Method(v, self)
                if isinstance(v, member):
                    self.__members[k] = v
                    assert self.__struct is not None
                    v.register(k, self.__struct)
        self.analyze_slots()

    def analyze_slots(self):
        self.__slots = {}
        for s in Slots:
            if s.special is not None:
                meth = self.__methods.get(s.special)
                if meth is not None:
                    self.__slots[s] = meth
        self.__slots[TP_NAME] = '"%s.%s"' % (self.__module, self.__name__)
        if self.__doc__:
            self.__slots[TP_DOC] = "%s_doc" % self.name
        if self.__struct is not None:
            self.__slots[TP_BASICSIZE] = "sizeof(%s)" % self.__struct.name
            self.__slots[TP_DEALLOC] = "%s_dealloc" % self.name
        if self.__methods:
            self.__slots[TP_METHODS] = "%s_methods" % self.name
        if self.__members:
            self.__slots[TP_MEMBERS] = "%s_members" % self.name

    def initvars(self):
        v = self.__vars = {}
        v["TypeName"] = self.__name__
        v["CTypeName"] = "Py%s_Type" % self.__name__
        v["MethodDefName"] = self.__slots[TP_METHODS]
        if self.__doc__:
            v["DocstringVar"] = self.__slots[TP_DOC]
            v["Docstring"] = cstring(unindent(self.__doc__))
        if self.__struct is not None:
            v["StructName"] = self.__struct.name
        if self.__members:
            v["MemberDefName"] = self.__slots[TP_MEMBERS]

    def dump_memberdef(self, f):
        def p(templ, vars=self.__vars):
            print(templ % vars, file=f)

        if not self.__members:
            return
        p(template.memberdef_start)
        for name, slot in sortitems(self.__members):
            slot.dump(f)
        p(template.memberdef_end)

    def dump_slots(self, f):
        def p(templ, vars=self.__vars):
            print(templ % vars, file=f)

        if self.struct:
            p(template.dealloc_func, {"name" : self.__slots[TP_DEALLOC]})

        p(template.type_struct_start)
        for s in Slots[:-5]: # XXX
            val = self.__slots.get(s, s.default)
            ntabs = 4 - (4 + len(val)) / 8
            line = "        %s,%s/* %s */" % (val, "\t" * ntabs, s.name)
            print(line, file=f)
        p(template.type_struct_end)

    def dump_init(self, f):
        def p(templ):
            print(templ % self.__vars, file=f)

        p(template.type_init_type)
        p(template.module_add_type)

class Type:
    __metaclass__ = TypeMetaclass
