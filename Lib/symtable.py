"""Interface to the compiler's internal symbol tables"""

import _symtable
from _symtable import USE, DEF_GLOBAL, DEF_LOCAL, DEF_PARAM, \
     DEF_STAR, DEF_DOUBLESTAR, DEF_INTUPLE, DEF_FREE, \
     DEF_FREE_GLOBAL, DEF_FREE_CLASS, DEF_IMPORT, DEF_BOUND, \
     OPT_IMPORT_STAR

import weakref

__all__ = ["symtable", "SymbolTable", "newSymbolTable", "Class",
           "Function", "Symbol"]

def symtable(code, filename, compile_type):
    raw = _symtable.symtable(code, filename, compile_type)
    for top in raw.values():
        if top.name == 'top':
            break
    return newSymbolTable(top, filename)

class SymbolTableFactory:
    def __init__(self):
        self.__memo = weakref.WeakValueDictionary()

    def new(self, table, filename):
        if table.type == _symtable.TYPE_FUNCTION:
            return Function(table, filename)
        if table.type == _symtable.TYPE_CLASS:
            return Class(table, filename)
        return SymbolTable(table, filename)

    def __call__(self, table, filename):
        key = table, filename
        obj = self.__memo.get(key, None)
        if obj is None:
            obj = self.__memo[key] = self.new(table, filename)
        return obj

newSymbolTable = SymbolTableFactory()

def is_free(flags):
    if (flags & (USE | DEF_FREE)) \
       and (flags & (DEF_LOCAL | DEF_PARAM | DEF_GLOBAL)):
        return True
    if flags & DEF_FREE_CLASS:
        return True
    return False

class SymbolTable:
    def __init__(self, raw_table, filename):
        self._table = raw_table
        self._filename = filename
        self._symbols = {}

    def __repr__(self):
        if self.__class__ == SymbolTable:
            kind = ""
        else:
            kind = "%s " % self.__class__.__name__

        if self._table.name == "global":
            return "<%sSymbolTable for module %s>" % (kind, self._filename)
        else:
            return "<%sSymbolTable for %s in %s>" % (kind, self._table.name,
                                                     self._filename)

    def get_type(self):
        if self._table.type == _symtable.TYPE_MODULE:
            return "module"
        if self._table.type == _symtable.TYPE_FUNCTION:
            return "function"
        if self._table.type == _symtable.TYPE_CLASS:
            return "class"
        assert self._table.type in (1, 2, 3), \
               "unexpected type: %s" % self._table.type

    def get_id(self):
        return self._table.id

    def get_name(self):
        return self._table.name

    def get_lineno(self):
        return self._table.lineno

    def is_optimized(self):
        return bool(self._table.type == _symtable.TYPE_FUNCTION
                    and not self._table.optimized)

    def is_nested(self):
        return bool(self._table.nested)

    def has_children(self):
        return bool(self._table.children)

    def has_exec(self):
        """Return true if the scope uses exec.  Deprecated method."""
        return False

    def has_import_star(self):
        """Return true if the scope uses import *"""
        return bool(self._table.optimized & OPT_IMPORT_STAR)

    def get_identifiers(self):
        return self._table.symbols.keys()

    def lookup(self, name):
        sym = self._symbols.get(name)
        if sym is None:
            flags = self._table.symbols[name]
            namespaces = self.__check_children(name)
            sym = self._symbols[name] = Symbol(name, flags, namespaces)
        return sym

    def get_symbols(self):
        return [self.lookup(ident) for ident in self.get_identifiers()]

    def __check_children(self, name):
        return [newSymbolTable(st, self._filename)
                for st in self._table.children
                if st.name == name]

    def get_children(self):
        return [newSymbolTable(st, self._filename)
                for st in self._table.children]

class Function(SymbolTable):

    # Default values for instance variables
    __params = None
    __locals = None
    __frees = None
    __globals = None

    def __idents_matching(self, test_func):
        return tuple([ident for ident in self.get_identifiers()
                      if test_func(self._table.symbols[ident])])

    def get_parameters(self):
        if self.__params is None:
            self.__params = self.__idents_matching(lambda x:x & DEF_PARAM)
        return self.__params

    def get_locals(self):
        if self.__locals is None:
            self.__locals = self.__idents_matching(lambda x:x & DEF_BOUND)
        return self.__locals

    def get_globals(self):
        if self.__globals is None:
            glob = DEF_GLOBAL | DEF_FREE_GLOBAL
            self.__globals = self.__idents_matching(lambda x:x & glob)
        return self.__globals

    def get_frees(self):
        if self.__frees is None:
            self.__frees = self.__idents_matching(is_free)
        return self.__frees

class Class(SymbolTable):

    __methods = None

    def get_methods(self):
        if self.__methods is None:
            d = {}
            for st in self._table.children:
                d[st.name] = 1
            self.__methods = tuple(d)
        return self.__methods

class Symbol:
    def __init__(self, name, flags, namespaces=None):
        self.__name = name
        self.__flags = flags
        self.__namespaces = namespaces or ()

    def __repr__(self):
        return "<symbol '%s'>" % self.__name

    def get_name(self):
        return self.__name

    def is_referenced(self):
        return bool(self.__flags & _symtable.USE)

    def is_parameter(self):
        return bool(self.__flags & DEF_PARAM)

    def is_global(self):
        return bool((self.__flags & DEF_GLOBAL)
                    or (self.__flags & DEF_FREE_GLOBAL))

    def is_vararg(self):
        return bool(self.__flags & DEF_STAR)

    def is_keywordarg(self):
        return bool(self.__flags & DEF_DOUBLESTAR)

    def is_local(self):
        return bool(self.__flags & DEF_BOUND)

    def is_free(self):
        if (self.__flags & (USE | DEF_FREE)) \
            and (self.__flags & (DEF_LOCAL | DEF_PARAM | DEF_GLOBAL)):
            return True
        if self.__flags & DEF_FREE_CLASS:
            return True
        return False

    def is_imported(self):
        return bool(self.__flags & DEF_IMPORT)

    def is_assigned(self):
        return bool(self.__flags & DEF_LOCAL)

    def is_in_tuple(self):
        return bool(self.__flags & DEF_INTUPLE)

    def is_namespace(self):
        """Returns true if name binding introduces new namespace.

        If the name is used as the target of a function or class
        statement, this will be true.

        Note that a single name can be bound to multiple objects.  If
        is_namespace() is true, the name may also be bound to other
        objects, like an int or list, that does not introduce a new
        namespace.
        """
        return bool(self.__namespaces)

    def get_namespaces(self):
        """Return a list of namespaces bound to this name"""
        return self.__namespaces

    def get_namespace(self):
        """Returns the single namespace bound to this name.

        Raises ValueError if the name is bound to multiple namespaces.
        """
        if len(self.__namespaces) != 1:
            raise ValueError("name is bound to multiple namespaces")
        return self.__namespaces[0]

if __name__ == "__main__":
    import os, sys
    src = open(sys.argv[0]).read()
    mod = symtable(src, os.path.split(sys.argv[0])[1], "exec")
    for ident in mod.get_identifiers():
        info = mod.lookup(ident)
        print(info, info.is_local(), info.is_namespace())
