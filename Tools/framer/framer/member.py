from framer import template
from framer.util import cstring, unindent

T_SHORT = "T_SHORT"
T_INT = "T_INT"
T_LONG = "T_LONG"
T_FLOAT = "T_FLOAT"
T_DOUBLE = "T_DOUBLE"
T_STRING = "T_STRING"
T_OBJECT = "T_OBJECT"
T_CHAR = "T_CHAR"
T_BYTE = "T_BYTE"
T_UBYTE = "T_UBYTE"
T_UINT = "T_UINT"
T_ULONG = "T_ULONG"
T_STRING_INPLACE = "T_STRING_INPLACE"
T_OBJECT_EX = "T_OBJECT_EX"

RO = READONLY = "READONLY"
READ_RESTRICTED = "READ_RESTRICTED"
WRITE_RESTRICTED = "WRITE_RESTRICTED"
RESTRICT = "RESTRICTED"

c2t = {"int" : T_INT,
       "unsigned int" : T_UINT,
       "long" : T_LONG,
       "unsigned long" : T_LONG,
       "float" : T_FLOAT,
       "double" : T_DOUBLE,
       "char *" : T_CHAR,
       "PyObject *" : T_OBJECT,
       }

class member(object):

    def __init__(self, cname=None, type=None, flags=None, doc=None):
        self.type = type
        self.flags = flags
        self.cname = cname
        self.doc = doc
        self.name = None
        self.struct = None

    def register(self, name, struct):
        self.name = name
        self.struct = struct
        self.initvars()

    def initvars(self):
        v = self.vars = {}
        v["PythonName"] = self.name
        if self.cname is not None:
            v["CName"] = self.cname
        else:
            v["CName"] = self.name
        v["Flags"] = self.flags or "0"
        v["Type"] = self.get_type()
        if self.doc is not None:
            v["Docstring"] = cstring(unindent(self.doc))
        v["StructName"] = self.struct.name

    def get_type(self):
        """Deduce type code from struct specification if not defined"""
        if self.type is not None:
            return self.type
        ctype = self.struct.get_type(self.name)
        return c2t[ctype]

    def dump(self, f):
        if self.doc is None:
            print >> f, template.memberdef_def % self.vars
        else:
            print >> f, template.memberdef_def_doc % self.vars
