from compiler import ast

class ASTVisitor:
    """Performs a depth-first walk of the AST

    The ASTVisitor will walk the AST, performing either a preorder or
    postorder traversal depending on which method is called.

    methods:
    preorder(tree, visitor)
    postorder(tree, visitor)
        tree: an instance of ast.Node
        visitor: an instance with visitXXX methods

    The ASTVisitor is responsible for walking over the tree in the
    correct order.  For each node, it checks the visitor argument for
    a method named 'visitNodeType' where NodeType is the name of the
    node's class, e.g. Class.  If the method exists, it is called
    with the node as its sole argument.

    The visitor method for a particular node type can control how
    child nodes are visited during a preorder walk.  (It can't control
    the order during a postorder walk, because it is called _after_
    the walk has occurred.)  The ASTVisitor modifies the visitor
    argument by adding a visit method to the visitor; this method can
    be used to visit a particular child node.  If the visitor method
    returns a true value, the ASTVisitor will not traverse the child
    nodes.

    XXX The interface for controlling the preorder walk needs to be
    re-considered.  The current interface is convenient for visitors
    that mostly let the ASTVisitor do everything.  For something like
    a code generator, where you want to walk to occur in a specific
    order, it's a pain to add "return 1" to the end of each method.
    """

    VERBOSE = 0

    def __init__(self):
        self.node = None
        self._cache = {}

    def preorder(self, tree, visitor):
        """Do preorder walk of tree using visitor"""
        self.visitor = visitor
        visitor.visit = self._preorder
        self._preorder(tree)

    def _preorder(self, node, *args):
        return apply(self.dispatch, (node,) + args)

    def default(self, node, *args):
        for child in node.getChildren():
            if isinstance(child, ast.Node):
                apply(self._preorder, (child,) + args)

    def dispatch(self, node, *args):
        self.node = node
        meth = self._cache.get(node.__class__, None)
        className = node.__class__.__name__
        if meth is None:
            meth = getattr(self.visitor, 'visit' + className, self.default)
            self._cache[node.__class__] = meth
        if self.VERBOSE > 0:
            if self.VERBOSE == 1:
                if meth == 0:
                    print "dispatch", className
            else:
                print "dispatch", className, (meth and meth.__name__ or '')
        return apply(meth, (node,) + args)

class ExampleASTVisitor(ASTVisitor):
    """Prints examples of the nodes that aren't visited

    This visitor-driver is only useful for development, when it's
    helpful to develop a visitor incremently, and get feedback on what
    you still have to do.
    """
    examples = {}
    
    def dispatch(self, node, *args):
        self.node = node
        meth = self._cache.get(node.__class__, None)
        className = node.__class__.__name__
        if meth is None:
            meth = getattr(self.visitor, 'visit' + className, 0)
            self._cache[node.__class__] = meth
        if self.VERBOSE > 1:
            print "dispatch", className, (meth and meth.__name__ or '')
        if meth:
            return apply(meth, (node,) + args)
        elif self.VERBOSE > 0:
            klass = node.__class__
            if not self.examples.has_key(klass):
                self.examples[klass] = klass
                print
                print self.visitor
                print klass
                for attr in dir(node):
                    if attr[0] != '_':
                        print "\t", "%-12.12s" % attr, getattr(node, attr)
                print
            return apply(self.default, (node,) + args)

_walker = ASTVisitor
def walk(tree, visitor, verbose=None):
    w = _walker()
    if verbose is not None:
        w.VERBOSE = verbose
    w.preorder(tree, visitor)
    return w.visitor

def dumpNode(node):
    print node.__class__
    for attr in dir(node):
        if attr[0] != '_':
            print "\t", "%-10.10s" % attr, getattr(node, attr)
