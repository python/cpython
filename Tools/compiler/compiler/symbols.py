"""Module symbol-table generator"""

from compiler import ast

module_scope = None

class Scope:
    # XXX how much information do I need about each name?
    def __init__(self, name):
        self.name = name
        self.defs = {}
        self.uses = {}
        self.globals = {}
        self.params = {}

    def __repr__(self):
        return "<%s: %s>" % (self.__class__.__name__, self.name)

    def add_def(self, name):
        self.defs[name] = 1

    def add_use(self, name):
        self.uses[name] = 1

    def add_global(self, name):
        if self.uses.has_key(name) or self.defs.has_key(name):
            pass # XXX warn about global following def/use
        if self.params.has_key(name):
            raise SyntaxError, "%s in %s is global and parameter" % \
                  (name, self.name)
        self.globals[name] = 1
        module_scope.add_def(name)

    def add_param(self, name):
        self.defs[name] = 1
        self.params[name] = 1

    def get_names(self):
        d = {}
        d.update(self.defs)
        d.update(self.uses)
        return d.keys()

class ModuleScope(Scope):
    __super_init = Scope.__init__
    
    def __init__(self):
        self.__super_init("global")
        global module_scope
        assert module_scope is None 
        module_scope = self

class LambdaScope(Scope):
    __super_init = Scope.__init__

    __counter = 1
    
    def __init__(self):
        i = self.__counter
        self.__counter += 1
        self.__super_init("lambda.%d" % i)

class FunctionScope(Scope):
    pass

class ClassScope(Scope):
    pass

class SymbolVisitor:
    def __init__(self):
        self.scopes = {}

    # node that define new scopes

    def visitModule(self, node):
        scope = self.scopes[node] = ModuleScope()
        self.visit(node.node, scope)

    def visitFunction(self, node, parent):
        parent.add_def(node.name)
        for n in node.defaults:
            self.visit(n, parent)
        scope = FunctionScope(node.name)
        self.scopes[node] = scope
        for name in node.argnames:
            scope.add_param(name)
        self.visit(node.code, scope)

    def visitLambda(self, node, parent):
        for n in node.defaults:
            self.visit(n, parent)
        scope = LambdaScope()
        self.scopes[node] = scope
        for name in node.argnames:
            scope.add_param(name)
        self.visit(node.code, scope)

    def visitClass(self, node, parent):
        parent.add_def(node.name)
        for n in node.bases:
            self.visit(n, parent)
        scope = ClassScope(node.name)
        self.scopes[node] = scope
        self.visit(node.code, scope)

    # name can be a def or a use

    # XXX a few calls and nodes expect a third "assign" arg that is
    # true if the name is being used as an assignment.  only
    # expressions contained within statements may have the assign arg.

    def visitName(self, node, scope, assign=0):
        if assign:
            scope.add_def(node.name)
        else:
            scope.add_use(node.name)

    # operations that bind new names

    def visitFor(self, node, scope):
        self.visit(node.assign, scope, 1)
        self.visit(node.list, scope)
        self.visit(node.body, scope)
        if node.else_:
            self.visit(node.else_, scope)

    def visitFrom(self, node, scope):
        for name, asname in node.names:
            if name == "*":
                continue
            scope.add_def(asname or name)

    def visitImport(self, node, scope):
        for name, asname in node.names:
            i = name.find(".")
            if i > -1:
                name = name[:i]
            scope.add_def(asname or name)

    def visitAssName(self, node, scope, assign=1):
        scope.add_def(node.name)

    def visitAugAssign(self, node, scope):
        # basically, the node is referenced and defined by the same expr
        self.visit(node.node, scope)
        self.visit(node.node, scope, 1)
        self.visit(node.expr, scope)

    def visitAssign(self, node, scope):
        for n in node.nodes:
            self.visit(n, scope, 1)
        self.visit(node.expr, scope)

    def visitGlobal(self, node, scope):
        for name in node.names:
            scope.add_global(name)

def sort(l):
    l = l[:]
    l.sort()
    return l

def list_eq(l1, l2):
    return sort(l1) == sort(l2)

if __name__ == "__main__":
    import sys
    from compiler import parseFile, walk
    import symtable

    for file in sys.argv[1:]:
        print file
        f = open(file)
        buf = f.read()
        f.close()
        syms = symtable.symtable(buf, file, "exec")
        mod_names = [s for s in [s.get_name()
                                 for s in syms.get_symbols()]
                     if not s.startswith('_[')]
        tree = parseFile(file)
        s = SymbolVisitor()
        walk(tree, s)
        for node, scope in s.scopes.items():
            print node.__class__.__name__, id(node)
            print scope
            print scope.get_names()

        names2 = s.scopes[tree].get_names()
        if not list_eq(mod_names, names2):
            print "oops", file
            print sort(mod_names)
            print sort(names2)
            sys.exit(-1)
