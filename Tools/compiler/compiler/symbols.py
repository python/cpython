"""Module symbol-table generator"""

from compiler import ast
import types

MANGLE_LEN = 256

class Scope:
    # XXX how much information do I need about each name?
    def __init__(self, name, module, klass=None):
        self.name = name
        self.module = module
        self.defs = {}
        self.uses = {}
        self.globals = {}
        self.params = {}
        self.children = []
        self.klass = None
        if klass is not None:
            for i in range(len(klass)):
                if klass[i] != '_':
                    self.klass = klass[i:]
                    break

    def __repr__(self):
        return "<%s: %s>" % (self.__class__.__name__, self.name)

    def mangle(self, name):
        if self.klass is None:
            return name
        if not name.startswith('__'):
            return name
        if len(name) + 2 >= MANGLE_LEN:
            return name
        if name.endswith('__'):
            return name
        return "_%s%s" % (self.klass, name)

    def add_def(self, name):
        self.defs[self.mangle(name)] = 1

    def add_use(self, name):
        self.uses[self.mangle(name)] = 1

    def add_global(self, name):
        name = self.mangle(name)
        if self.uses.has_key(name) or self.defs.has_key(name):
            pass # XXX warn about global following def/use
        if self.params.has_key(name):
            raise SyntaxError, "%s in %s is global and parameter" % \
                  (name, self.name)
        self.globals[name] = 1
        self.module.add_def(name)

    def add_param(self, name):
        name = self.mangle(name)
        self.defs[name] = 1
        self.params[name] = 1

    def get_names(self):
        d = {}
        d.update(self.defs)
        d.update(self.uses)
        d.update(self.globals)
        return d.keys()

    def add_child(self, child):
        self.children.append(child)

    def get_children(self):
        return self.children

class ModuleScope(Scope):
    __super_init = Scope.__init__
    
    def __init__(self):
        self.__super_init("global", self)

class LambdaScope(Scope):
    __super_init = Scope.__init__

    __counter = 1
    
    def __init__(self, module, klass=None):
        i = self.__counter
        self.__counter += 1
        self.__super_init("lambda.%d" % i, module, klass)

class FunctionScope(Scope):
    pass

class ClassScope(Scope):
    __super_init = Scope.__init__

    def __init__(self, name, module):
        self.__super_init(name, module, name)

class SymbolVisitor:
    def __init__(self):
        self.scopes = {}
        self.klass = None
        
    # node that define new scopes

    def visitModule(self, node):
        scope = self.module = self.scopes[node] = ModuleScope()
        self.visit(node.node, scope)

    def visitFunction(self, node, parent):
        parent.add_def(node.name)
        for n in node.defaults:
            self.visit(n, parent)
        scope = FunctionScope(node.name, self.module, self.klass)
        self.scopes[node] = scope
        self._do_args(scope, node.argnames)
        self.visit(node.code, scope)

    def visitLambda(self, node, parent):
        for n in node.defaults:
            self.visit(n, parent)
        scope = LambdaScope(self.module, self.klass)
        self.scopes[node] = scope
        self._do_args(scope, node.argnames)
        self.visit(node.code, scope)

    def _do_args(self, scope, args):
        for name in args:
            if type(name) == types.TupleType:
                self._do_args(scope, name)
            else:
                scope.add_param(name)

    def visitClass(self, node, parent):
        parent.add_def(node.name)
        for n in node.bases:
            self.visit(n, parent)
        scope = ClassScope(node.name, self.module)
        self.scopes[node] = scope
        prev = self.klass
        self.klass = node.name
        self.visit(node.code, scope)
        self.klass = prev

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

    # prune if statements if tests are false

    _const_types = types.StringType, types.IntType, types.FloatType

    def visitIf(self, node, scope):
        for test, body in node.tests:
            if isinstance(test, ast.Const):
                if type(test.value) in self._const_types:
                    if not test.value:
                        continue
            self.visit(test, scope)
            self.visit(body, scope)
        if node.else_:
            self.visit(node.else_, scope)

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

    def get_names(syms):
        return [s for s in [s.get_name() for s in syms.get_symbols()]
                if not (s.startswith('_[') or s.startswith('.'))]        
    
    for file in sys.argv[1:]:
        print file
        f = open(file)
        buf = f.read()
        f.close()
        syms = symtable.symtable(buf, file, "exec")
        mod_names = get_names(syms)
        tree = parseFile(file)
        s = SymbolVisitor()
        walk(tree, s)

        # compare module-level symbols
        names2 = s.scopes[tree].get_names()

        if not list_eq(mod_names, names2):
            print
            print "oops", file
            print sort(mod_names)
            print sort(names2)
            sys.exit(-1)

        d = {}
        d.update(s.scopes)
        del d[tree]
        scopes = d.values()
        del d

        for s in syms.get_symbols():
            if s.is_namespace():
                l = [sc for sc in scopes
                     if sc.name == s.get_name()]
                if len(l) > 1:
                    print "skipping", s.get_name()
                else:
                    if not list_eq(get_names(s.get_namespace()),
                                   l[0].get_names()):
                        print s.get_name()
                        print sort(get_names(s.get_namespace()))
                        print sort(l[0].get_names())
                        sys.exit(-1)
