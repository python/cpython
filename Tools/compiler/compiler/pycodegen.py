"""Python bytecode generator

Currently contains generic ASTVisitor code, a LocalNameFinder, and a
CodeGenerator.  Eventually, this will get split into the ASTVisitor as
a generic tool and CodeGenerator as a specific tool.
"""

from p2c import transformer, ast
from pyassem import StackRef, PyAssembler
import dis
import misc
import marshal
import new
import string
import sys
import os
import stat
import struct

def parse(path):
    f = open(path)
    src = f.read()
    f.close()
    t = transformer.Transformer()
    return t.parsesuite(src)

def walk(tree, visitor, verbose=None, walker=None):
    if walker:
        w = walker()
    else:
        w = ASTVisitor()
    if verbose is not None:
        w.VERBOSE = verbose
    w.preorder(tree, visitor)
    return w.visitor

def dumpNode(node):
    print node.__class__
    for attr in dir(node):
        if attr[0] != '_':
            print "\t", "%-10.10s" % attr, getattr(node, attr)

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
    node's class, e.g. Classdef.  If the method exists, it is called
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

    XXX Perhaps I can use a postorder walk for the code generator?
    """

    VERBOSE = 0

    def __init__(self):
	self.node = None

    def preorder(self, tree, visitor):
        """Do preorder walk of tree using visitor"""
        self.visitor = visitor
        visitor.visit = self._preorder
        self._preorder(tree)

    def _preorder(self, node):
        stop = self.dispatch(node)
        if stop:
            return
        for child in node.getChildren():
            if isinstance(child, ast.Node):
                self._preorder(child)

    def postorder(self, tree, visitor):
        """Do preorder walk of tree using visitor"""
        self.visitor = visitor
        visitor.visit = self._postorder
        self._postorder(tree)

    def _postorder(self, tree):
        for child in node.getChildren():
            if isinstance(child, ast.Node):
                self._preorder(child)
        self.dispatch(node)

    def dispatch(self, node):
        self.node = node
        className = node.__class__.__name__
        meth = getattr(self.visitor, 'visit' + className, None)
        if self.VERBOSE > 0:
            if self.VERBOSE == 1:
                if meth is None:
                    print "dispatch", className
            else:
                print "dispatch", className, (meth and meth.__name__ or '')
        if meth:
            return meth(node)

class ExampleASTVisitor(ASTVisitor):
    """Prints examples of the nodes that aren't visited

    This visitor-driver is only useful for development, when it's
    helpful to develop a visitor incremently, and get feedback on what
    you still have to do.
    """
    examples = {}
    
    def dispatch(self, node):
        self.node = node
        className = node.__class__.__name__
        meth = getattr(self.visitor, 'visit' + className, None)
        if self.VERBOSE > 0:
            if self.VERBOSE == 1:
                if meth is None:
                    print "dispatch", className
            else:
                print "dispatch", className, (meth and meth.__name__ or '')
        if meth:
            return meth(node)
        else:
            klass = node.__class__
            if self.VERBOSE < 2:
                if self.examples.has_key(klass):
                    return
            self.examples[klass] = klass
            print
            print klass
            for attr in dir(node):
                if attr[0] != '_':
                    print "\t", "%-12.12s" % attr, getattr(node, attr)
            print

class CodeGenerator:
    """Generate bytecode for the Python VM"""

    OPTIMIZED = 1

    # XXX should clean up initialization and generateXXX funcs
    def __init__(self, filename="<?>"):
        self.filename = filename
	self.code = PyAssembler()
        self.code.setFlags(0)
	self.locals = misc.Stack()
        self.loops = misc.Stack()
        self.namespace = 0
        self.curStack = 0
        self.maxStack = 0

    def emit(self, *args):
	# XXX could just use self.emit = self.code.emit
	apply(self.code.emit, args)

    def _generateFunctionOrLambdaCode(self, func):
        self.name = func.name
        self.filename = filename
        args = func.argnames
	self.code = PyAssembler(args=args, name=func.name,
                                 filename=filename)
        self.namespace = self.OPTIMIZED
        if func.varargs:
            self.code.setVarArgs()
        if func.kwargs:
            self.code.setKWArgs()
        lnf = walk(func.code, LocalNameFinder(args), 0)
        self.locals.push(lnf.getLocals())
	self.emit('SET_LINENO', func.lineno)
        walk(func.code, self)

    def generateFunctionCode(self, func):
        """Generate code for a function body"""
        self._generateFunctionOrLambdaCode(func)
        self.emit('LOAD_CONST', None)
        self.emit('RETURN_VALUE')

    def generateLambdaCode(self, func):
        self._generateFunctionOrLambdaCode(func)
        self.emit('RETURN_VALUE')

    def generateClassCode(self, klass):
        self.code = PyAssembler(name=klass.name,
                                 filename=filename)
        self.emit('SET_LINENO', klass.lineno)
        lnf = walk(klass.code, LocalNameFinder(), 0)
        self.locals.push(lnf.getLocals())
        walk(klass.code, self)
        self.emit('LOAD_LOCALS')
        self.emit('RETURN_VALUE')

    def asConst(self):
        """Create a Python code object."""
        if self.namespace == self.OPTIMIZED:
            self.code.setOptimized()
        return self.code.makeCodeObject()

    def isLocalName(self, name):
	return self.locals.top().has_elt(name)

    def _nameOp(self, prefix, name):
        if self.isLocalName(name):
            if self.namespace == self.OPTIMIZED:
                self.emit(prefix + '_FAST', name)
            else:
                self.emit(prefix + '_NAME', name)
        else:
            self.emit(prefix + '_GLOBAL', name)

    def storeName(self, name):
        self._nameOp('STORE', name)

    def loadName(self, name):
        self._nameOp('LOAD', name)

    def delName(self, name):
        self._nameOp('DELETE', name)

    def visitNULL(self, node):
        """Method exists only to stop warning in -v mode"""
        pass

    visitStmt = visitNULL
    visitGlobal = visitNULL

    def visitDiscard(self, node):
        self.visit(node.expr)
        self.emit('POP_TOP')
        return 1

    def visitPass(self, node):
        self.emit('SET_LINENO', node.lineno)

    def visitModule(self, node):
	lnf = walk(node.node, LocalNameFinder(), 0)
	self.locals.push(lnf.getLocals())
        self.visit(node.node)
        self.emit('LOAD_CONST', None)
        self.emit('RETURN_VALUE')
        return 1

    def visitImport(self, node):
        self.emit('SET_LINENO', node.lineno)
        for name in node.names:
            self.emit('IMPORT_NAME', name)
            self.storeName(name)

    def visitFrom(self, node):
        self.emit('SET_LINENO', node.lineno)
        self.emit('IMPORT_NAME', node.modname)
        for name in node.names:
            if name == '*':
                self.namespace = 0
            self.emit('IMPORT_FROM', name)
        self.emit('POP_TOP')

    def visitClassdef(self, node):
        self.emit('SET_LINENO', node.lineno)
        self.emit('LOAD_CONST', node.name)
        for base in node.bases:
            self.visit(base)
        self.emit('BUILD_TUPLE', len(node.bases))
        classBody = CodeGenerator(self.filename)
        classBody.generateClassCode(node)
        self.emit('LOAD_CONST', classBody)
        self.emit('MAKE_FUNCTION', 0)
        self.emit('CALL_FUNCTION', 0)
        self.emit('BUILD_CLASS')
        self.storeName(node.name)
        return 1

    def _visitFuncOrLambda(self, node, kind):
        """Code common to Function and Lambda nodes"""
        codeBody = CodeGenerator(self.filename)
        getattr(codeBody, 'generate%sCode' % kind)(node)
        self.emit('SET_LINENO', node.lineno)
        for default in node.defaults:
            self.visit(default)
        self.emit('LOAD_CONST', codeBody)
        self.emit('MAKE_FUNCTION', len(node.defaults))

    def visitFunction(self, node):
        self._visitFuncOrLambda(node, 'Function')
        self.storeName(node.name)
        return 1

    def visitLambda(self, node):
        node.name = '<lambda>'
        node.varargs = node.kwargs = None
        self._visitFuncOrLambda(node, 'Lambda')
        return 1

    def visitCallFunc(self, node):
        pos = 0
        kw = 0
        if hasattr(node, 'lineno'):
            self.emit('SET_LINENO', node.lineno)
	self.visit(node.node)
	for arg in node.args:
	    self.visit(arg)
            if isinstance(arg, ast.Keyword):
                kw = kw + 1
            else:
                pos = pos + 1
	self.emit('CALL_FUNCTION', kw << 8 | pos)
	return 1

    def visitKeyword(self, node):
        self.emit('LOAD_CONST', node.name)
        self.visit(node.expr)
        return 1

    def visitIf(self, node):
	after = StackRef()
	for test, suite in node.tests:
            if hasattr(test, 'lineno'):
                self.emit('SET_LINENO', test.lineno)
            else:
                print "warning", "no line number"
	    self.visit(test)
	    dest = StackRef()
	    self.emit('JUMP_IF_FALSE', dest)
	    self.emit('POP_TOP')
	    self.visit(suite)
	    self.emit('JUMP_FORWARD', after)
	    dest.bind(self.code.getCurInst())
	    self.emit('POP_TOP')
	if node.else_:
	    self.visit(node.else_)
	after.bind(self.code.getCurInst())
	return 1

    def startLoop(self):
        l = Loop()
        self.loops.push(l)
        self.emit('SETUP_LOOP', l.extentAnchor)
        return l

    def finishLoop(self):
        l = self.loops.pop()
        i = self.code.getCurInst()
        l.extentAnchor.bind(self.code.getCurInst())

    def visitFor(self, node):
        # three refs needed
        anchor = StackRef()

        self.emit('SET_LINENO', node.lineno)
        l = self.startLoop()
        self.visit(node.list)
        self.visit(ast.Const(0))
        l.startAnchor.bind(self.code.getCurInst())
        self.emit('SET_LINENO', node.lineno)
        self.emit('FOR_LOOP', anchor)
        self.visit(node.assign)
        self.visit(node.body)
        self.emit('JUMP_ABSOLUTE', l.startAnchor)
        anchor.bind(self.code.getCurInst())
        self.emit('POP_BLOCK')
        if node.else_:
            self.visit(node.else_)
        self.finishLoop()
        return 1

    def visitWhile(self, node):
        self.emit('SET_LINENO', node.lineno)
        l = self.startLoop()
        if node.else_:
            lElse = StackRef()
        else:
            lElse = l.breakAnchor
        l.startAnchor.bind(self.code.getCurInst())
        self.emit('SET_LINENO', node.test.lineno)
        self.visit(node.test)
        self.emit('JUMP_IF_FALSE', lElse)
        self.emit('POP_TOP')
        self.visit(node.body)
        self.emit('JUMP_ABSOLUTE', l.startAnchor)
        # note that lElse may be an alias for l.breakAnchor
        lElse.bind(self.code.getCurInst())
        self.emit('POP_TOP')
        self.emit('POP_BLOCK')
        if node.else_:
            self.visit(node.else_)
        self.finishLoop()
        return 1

    def visitBreak(self, node):
        if not self.loops:
            raise SyntaxError, "'break' outside loop"
        self.emit('SET_LINENO', node.lineno)
        self.emit('BREAK_LOOP')

    def visitContinue(self, node):
        if not self.loops:
            raise SyntaxError, "'continue' outside loop"
        l = self.loops.top()
        self.emit('SET_LINENO', node.lineno)
        self.emit('JUMP_ABSOLUTE', l.startAnchor)

    def visitTryExcept(self, node):
        # XXX need to figure out exactly what is on the stack when an
        # exception is raised and the first handler is checked
        handlers = StackRef()
        end = StackRef()
        if node.else_:
            lElse = StackRef()
        else:
            lElse = end
        self.emit('SET_LINENO', node.lineno)
        self.emit('SETUP_EXCEPT', handlers)
        self.visit(node.body)
        self.emit('POP_BLOCK')
        self.emit('JUMP_FORWARD', lElse)
        handlers.bind(self.code.getCurInst())
        
        last = len(node.handlers) - 1
        for i in range(len(node.handlers)):
            expr, target, body = node.handlers[i]
            if hasattr(expr, 'lineno'):
                self.emit('SET_LINENO', expr.lineno)
            if expr:
                self.emit('DUP_TOP')
                self.visit(expr)
                self.emit('COMPARE_OP', "exception match")
                next = StackRef()
                self.emit('JUMP_IF_FALSE', next)
                self.emit('POP_TOP')
            self.emit('POP_TOP')
            if target:
                self.visit(target)
            else:
                self.emit('POP_TOP')
            self.emit('POP_TOP')
            self.visit(body)
            self.emit('JUMP_FORWARD', end)
            next.bind(self.code.getCurInst())
            self.emit('POP_TOP')
        self.emit('END_FINALLY')
        if node.else_:
            lElse.bind(self.code.getCurInst())
            self.visit(node.else_)
        end.bind(self.code.getCurInst())
        return 1
    
    def visitTryFinally(self, node):
        final = StackRef()
        self.emit('SET_LINENO', node.lineno)
        self.emit('SETUP_FINALLY', final)
        self.visit(node.body)
        self.emit('POP_BLOCK')
        self.emit('LOAD_CONST', None)
        final.bind(self.code.getCurInst())
        self.visit(node.final)
        self.emit('END_FINALLY')
        return 1

    def visitCompare(self, node):
	"""Comment from compile.c follows:

	The following code is generated for all but the last
	comparison in a chain:
	   
	label:	on stack:	opcode:		jump to:
	   
		a		<code to load b>
		a, b		DUP_TOP
		a, b, b		ROT_THREE
		b, a, b		COMPARE_OP
		b, 0-or-1	JUMP_IF_FALSE	L1
		b, 1		POP_TOP
		b		
	
	We are now ready to repeat this sequence for the next
	comparison in the chain.
	   
	For the last we generate:
	   
	   	b		<code to load c>
	   	b, c		COMPARE_OP
	   	0-or-1		
	   
	If there were any jumps to L1 (i.e., there was more than one
	comparison), we generate:
	   
	   	0-or-1		JUMP_FORWARD	L2
	   L1:	b, 0		ROT_TWO
	   	0, b		POP_TOP
	   	0
	   L2:	0-or-1
	"""
	self.visit(node.expr)
	# if refs are never emitted, subsequent bind call has no effect
	l1 = StackRef()
	l2 = StackRef()
	for op, code in node.ops[:-1]:
	    # emit every comparison except the last
	    self.visit(code)
	    self.emit('DUP_TOP')
	    self.emit('ROT_THREE')
	    self.emit('COMPARE_OP', op)
            # dupTop and compareOp cancel stack effect
	    self.emit('JUMP_IF_FALSE', l1)
	    self.emit('POP_TOP')
	if node.ops:
	    # emit the last comparison
	    op, code = node.ops[-1]
	    self.visit(code)
	    self.emit('COMPARE_OP', op)
	if len(node.ops) > 1:
	    self.emit('JUMP_FORWARD', l2)
	    l1.bind(self.code.getCurInst())
	    self.emit('ROT_TWO')
	    self.emit('POP_TOP')
	    l2.bind(self.code.getCurInst())
	return 1

    def visitGetattr(self, node):
        self.visit(node.expr)
        self.emit('LOAD_ATTR', node.attrname)
        return 1

    def visitSubscript(self, node):
        self.visit(node.expr)
        for sub in node.subs:
            self.visit(sub)
        if len(node.subs) > 1:
            self.emit('BUILD_TUPLE', len(node.subs))
        if node.flags == 'OP_APPLY':
            self.emit('BINARY_SUBSCR')
        elif node.flags == 'OP_ASSIGN':
            self.emit('STORE_SUBSCR')
	elif node.flags == 'OP_DELETE':
            self.emit('DELETE_SUBSCR')
        print
        return 1

    def visitSlice(self, node):
        self.visit(node.expr)
        slice = 0
        if node.lower:
            self.visit(node.lower)
            slice = slice | 1
        if node.upper:
            self.visit(node.upper)
            slice = slice | 2
        if node.flags == 'OP_APPLY':
            self.emit('SLICE+%d' % slice)
        elif node.flags == 'OP_ASSIGN':
            self.emit('STORE_SLICE+%d' % slice)
        elif node.flags == 'OP_DELETE':
            self.emit('DELETE_SLICE+%d' % slice)
        else:
            print "weird slice", node.flags
            raise
        return 1

    def visitSliceobj(self, node):
        for child in node.nodes:
            print child
            self.visit(child)
        self.emit('BUILD_SLICE', len(node.nodes))
        return 1

    def visitAssign(self, node):
        self.emit('SET_LINENO', node.lineno)
        self.visit(node.expr)
	dups = len(node.nodes) - 1
        for i in range(len(node.nodes)):
	    elt = node.nodes[i]
	    if i < dups:
		self.emit('DUP_TOP')
            if isinstance(elt, ast.Node):
                self.visit(elt)
        return 1

    def visitAssName(self, node):
        if node.flags != 'OP_ASSIGN':
            print "oops", node.flags
        self.storeName(node.name)

    def visitAssAttr(self, node):
        if node.flags != 'OP_ASSIGN':
            print "warning: unexpected flags:", node.flags
            print node
        self.visit(node.expr)
        self.emit('STORE_ATTR', node.attrname)
        return 1

    def visitAssTuple(self, node):
        self.emit('UNPACK_TUPLE', len(node.nodes))
        for child in node.nodes:
            self.visit(child)
        return 1

    visitAssList = visitAssTuple

    def binaryOp(self, node, op):
	self.visit(node.left)
	self.visit(node.right)
	self.emit(op)
	return 1

    def unaryOp(self, node, op):
        self.visit(node.expr)
        self.emit(op)
        return 1

    def visitAdd(self, node):
	return self.binaryOp(node, 'BINARY_ADD')

    def visitSub(self, node):
	return self.binaryOp(node, 'BINARY_SUBTRACT')

    def visitMul(self, node):
	return self.binaryOp(node, 'BINARY_MULTIPLY')

    def visitDiv(self, node):
	return self.binaryOp(node, 'BINARY_DIVIDE')

    def visitMod(self, node):
	return self.binaryOp(node, 'BINARY_MODULO')

    def visitPower(self, node):
	return self.binaryOp(node, 'BINARY_POWER')

    def visitLeftShift(self, node):
	return self.binaryOp(node, 'BINARY_LSHIFT')

    def visitRightShift(self, node):
	return self.binaryOp(node, 'BINARY_RSHIFT')

    def visitInvert(self, node):
        return self.unaryOp(node, 'UNARY_INVERT')

    def visitUnarySub(self, node):
        return self.unaryOp(node, 'UNARY_NEGATIVE')

    def visitUnaryAdd(self, node):
        return self.unaryOp(node, 'UNARY_POSITIVE')

    def visitUnaryInvert(self, node):
        return self.unaryOp(node, 'UNARY_INVERT')

    def visitNot(self, node):
        return self.unaryOp(node, 'UNARY_NOT')

    def visitBackquote(self, node):
        return self.unaryOp(node, 'UNARY_CONVERT')

    def bitOp(self, nodes, op):
	self.visit(nodes[0])
	for node in nodes[1:]:
	    self.visit(node)
	    self.emit(op)
	return 1

    def visitBitand(self, node):
	return self.bitOp(node.nodes, 'BINARY_AND')

    def visitBitor(self, node):
	return self.bitOp(node.nodes, 'BINARY_OR')

    def visitBitxor(self, node):
	return self.bitOp(node.nodes, 'BINARY_XOR')

    def visitTest(self, node, jump):
        end = StackRef()
        for child in node.nodes[:-1]:
            self.visit(child)
            self.emit(jump, end)
            self.emit('POP_TOP')
        self.visit(node.nodes[-1])
        end.bind(self.code.getCurInst())
        return 1

    def visitAssert(self, node):
	# XXX __debug__ and AssertionError appear to be special cases
	# -- they are always loaded as globals even if there are local
	# names.  I guess this is a sort of renaming op.
	skip = StackRef()
	self.emit('SET_LINENO', node.lineno)
	self.emit('LOAD_GLOBAL', '__debug__')
	self.emit('JUMP_IF_FALSE', skip)
	self.emit('POP_TOP')
	self.visit(node.test)
	self.emit('JUMP_IF_TRUE', skip)
	self.emit('LOAD_GLOBAL', 'AssertionError')
	self.visit(node.fail)
	self.emit('RAISE_VARARGS', 2)
	skip.bind(self.code.getCurInst())
	self.emit('POP_TOP')
	return 1

    def visitAnd(self, node):
        return self.visitTest(node, 'JUMP_IF_FALSE')

    def visitOr(self, node):
        return self.visitTest(node, 'JUMP_IF_TRUE')

    def visitName(self, node):
        self.loadName(node.name)

    def visitConst(self, node):
	self.emit('LOAD_CONST', node.value)
        return 1

    def visitEllipsis(self, node):
	self.emit('LOAD_CONST', Ellipsis)
	return 1

    def visitTuple(self, node):
        for elt in node.nodes:
            self.visit(elt)
        self.emit('BUILD_TUPLE', len(node.nodes))
        return 1

    def visitList(self, node):
        for elt in node.nodes:
            self.visit(elt)
        self.emit('BUILD_LIST', len(node.nodes))
        return 1

    def visitDict(self, node):
	self.emit('BUILD_MAP', 0)
	for k, v in node.items:
	    # XXX need to add set lineno when there aren't constants
	    self.emit('DUP_TOP')
	    self.visit(v)
	    self.emit('ROT_TWO')
	    self.visit(k)
	    self.emit('STORE_SUBSCR')
	return 1

    def visitReturn(self, node):
	self.emit('SET_LINENO', node.lineno)
	self.visit(node.value)
	self.emit('RETURN_VALUE')
	return 1

    def visitRaise(self, node):
	self.emit('SET_LINENO', node.lineno)
	n = 0
	if node.expr1:
	    self.visit(node.expr1)
	    n = n + 1
	if node.expr2:
	    self.visit(node.expr2)
	    n = n + 1
	if node.expr3:
	    self.visit(node.expr3)
	    n = n + 1
	self.emit('RAISE_VARARGS', n)
	return 1

    def visitPrint(self, node):
	self.emit('SET_LINENO', node.lineno)
	for child in node.nodes:
	    self.visit(child)
	    self.emit('PRINT_ITEM')
	return 1

    def visitPrintnl(self, node):
	self.visitPrint(node)
	self.emit('PRINT_NEWLINE')
	return 1

    def visitExec(self, node):
	self.visit(node.expr)
	if node.locals is None:
	    self.emit('LOAD_CONST', None)
	else:
	    self.visit(node.locals)
	if node.globals is None:
	    self.emit('DUP_TOP')
	else:
	    self.visit(node.globals)
	self.emit('EXEC_STMT')

class LocalNameFinder:
    def __init__(self, names=()):
	self.names = misc.Set()
        self.globals = misc.Set()
	for name in names:
	    self.names.add(name)

    def getLocals(self):
        for elt in self.globals.items():
            if self.names.has_elt(elt):
                self.names.remove(elt)
	return self.names

    def visitDict(self, node):
	return 1

    def visitGlobal(self, node):
        for name in node.names:
            self.globals.add(name)
        return 1

    def visitFunction(self, node):
        self.names.add(node.name)
	return 1

    def visitLambda(self, node):
        return 1

    def visitImport(self, node):
	for name in node.names:
	    self.names.add(name)

    def visitFrom(self, node):
	for name in node.names:
	    self.names.add(name)

    def visitClassdef(self, node):
	self.names.add(node.name)
	return 1

    def visitAssName(self, node):
	self.names.add(node.name)

class Loop:
    def __init__(self):
        self.startAnchor = StackRef()
        self.breakAnchor = StackRef()
        self.extentAnchor = StackRef()

class CompiledModule:
    """Store the code object for a compiled module

    XXX Not clear how the code objects will be stored.  Seems possible
    that a single code attribute is sufficient, because it will
    contains references to all the need code objects.  That might be
    messy, though.
    """
    MAGIC = (20121 | (ord('\r')<<16) | (ord('\n')<<24))

    def __init__(self, source, filename):
        self.source = source
        self.filename = filename

    def compile(self):
        t = transformer.Transformer()
        self.ast = t.parsesuite(self.source)
        cg = CodeGenerator(self.filename)
        walk(self.ast, cg, walker=ExampleASTVisitor)
        self.code = cg.asConst()

    def dump(self, path):
        """create a .pyc file"""
        f = open(path, 'wb')
        f.write(self._pyc_header())
        marshal.dump(self.code, f)
        f.close()
        
    def _pyc_header(self):
        # compile.c uses marshal to write a long directly, with
        # calling the interface that would also generate a 1-byte code
        # to indicate the type of the value.  simplest way to get the
        # same effect is to call marshal and then skip the code.
        magic = marshal.dumps(self.MAGIC)[1:]
        mtime = os.stat(self.filename)[stat.ST_MTIME]
        mtime = struct.pack('i', mtime)
        return magic + mtime
	
if __name__ == "__main__":
    import getopt

    opts, args = getopt.getopt(sys.argv[1:], 'vq')
    for k, v in opts:
        if k == '-v':
            ASTVisitor.VERBOSE = ASTVisitor.VERBOSE + 1
        if k == '-q':
            f = open('/dev/null', 'wb')
            sys.stdout = f
    if args:
        filename = args[0]
    else:
        filename = 'test.py'
    buf = open(filename).read()
    mod = CompiledModule(buf, filename)
    mod.compile()
    mod.dump(filename + 'c')
