"""Python bytecode generator

Currently contains generic ASTVisitor code, a LocalNameFinder, and a
CodeGenerator.  Eventually, this will get split into the ASTVisitor as
a generic tool and CodeGenerator as a specific tool.
"""

from p2c import transformer, ast
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
    """Prints examples of the nodes that aren't visited"""
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
    # XXX this should be combined with PythonVMCode.  there is no
    # clear way to split the functionality into two classes.

    OPTIMIZED = 1

    def __init__(self, filename="<?>"):
        self.filename = filename
	self.code = PythonVMCode()
        self.code.setFlags(0)
	self.locals = misc.Stack()
        self.loops = misc.Stack()
        self.namespace = 0
        self.curStack = 0
        self.maxStack = 0

    def _generateFunctionOrLambdaCode(self, func):
        self.name = func.name
        self.filename = filename
        args = func.argnames
	self.code = PythonVMCode(args=args, name=func.name,
                                 filename=filename)
        self.namespace = self.OPTIMIZED
        if func.varargs:
            self.code.setVarArgs()
        if func.kwargs:
            self.code.setKWArgs()
        lnf = walk(func.code, LocalNameFinder(args), 0)
        self.locals.push(lnf.getLocals())
	self.code.setLineNo(func.lineno)
        walk(func.code, self)

    def generateFunctionCode(self, func):
        """Generate code for a function body"""
        self._generateFunctionOrLambdaCode(func)
        self.code.emit('LOAD_CONST', None)
        self.code.emit('RETURN_VALUE')

    def generateLambdaCode(self, func):
        self._generateFunctionOrLambdaCode(func)
        self.code.emit('RETURN_VALUE')

    def generateClassCode(self, klass):
        self.code = PythonVMCode(name=klass.name,
                                 filename=filename)
        self.code.setLineNo(klass.lineno)
        lnf = walk(klass.code, LocalNameFinder(), 0)
        self.locals.push(lnf.getLocals())
        walk(klass.code, self)
        self.code.emit('LOAD_LOCALS')
        self.code.emit('RETURN_VALUE')

    def emit(self):
        """Create a Python code object

        XXX It is confusing that this method isn't related to the
        method named emit in the PythonVMCode.
        """
        if self.namespace == self.OPTIMIZED:
            self.code.setOptimized()
        return self.code.makeCodeObject(self.maxStack)

    def isLocalName(self, name):
	return self.locals.top().has_elt(name)

    def _nameOp(self, prefix, name):
        if self.isLocalName(name):
            if self.namespace == self.OPTIMIZED:
                self.code.emit(prefix + '_FAST', name)
            else:
                self.code.emit(prefix + '_NAME', name)
        else:
            self.code.emit(prefix + '_GLOBAL', name)

    def storeName(self, name):
        self._nameOp('STORE', name)

    def loadName(self, name):
        self._nameOp('LOAD', name)

    def delName(self, name):
        self._nameOp('DELETE', name)

    def push(self, n):
        self.curStack = self.curStack + n
        if self.curStack > self.maxStack:
            self.maxStack = self.curStack

    def pop(self, n):
        if n >= self.curStack:
            self.curStack = self.curStack - n
        else:
            self.curStack = 0

    def assertStackEmpty(self):
        if self.curStack != 0:
            print "warning: stack should be empty"

    def visitNULL(self, node):
        """Method exists only to stop warning in -v mode"""
        pass

    visitStmt = visitNULL
    visitGlobal = visitNULL

    def visitDiscard(self, node):
        self.visit(node.expr)
        self.code.emit('POP_TOP')
        self.pop(1)
        return 1

    def visitPass(self, node):
        self.code.setLineNo(node.lineno)

    def visitModule(self, node):
	lnf = walk(node.node, LocalNameFinder(), 0)
	self.locals.push(lnf.getLocals())
        self.visit(node.node)
        self.code.emit('LOAD_CONST', None)
        self.code.emit('RETURN_VALUE')
        return 1

    def visitImport(self, node):
        self.code.setLineNo(node.lineno)
        for name in node.names:
            self.code.emit('IMPORT_NAME', name)
            self.storeName(name)

    def visitFrom(self, node):
        self.code.setLineNo(node.lineno)
        self.code.emit('IMPORT_NAME', node.modname)
        for name in node.names:
            self.code.emit('IMPORT_FROM', name)
        self.code.emit('POP_TOP')

    def visitClassdef(self, node):
        self.code.emit('SET_LINENO', node.lineno)
        self.code.emit('LOAD_CONST', node.name)
        for base in node.bases:
            self.visit(base)
        self.code.emit('BUILD_TUPLE', len(node.bases))
        classBody = CodeGenerator(self.filename)
        classBody.generateClassCode(node)
        self.code.emit('LOAD_CONST', classBody)
        self.code.emit('MAKE_FUNCTION', 0)
        self.code.emit('CALL_FUNCTION', 0)
        self.code.emit('BUILD_CLASS')
        self.storeName(node.name)
        return 1

    def _visitFuncOrLambda(self, node, kind):
        """Code common to Function and Lambda nodes"""
        codeBody = CodeGenerator(self.filename)
        getattr(codeBody, 'generate%sCode' % kind)(node)
        self.code.setLineNo(node.lineno)
        for default in node.defaults:
            self.visit(default)
        self.code.emit('LOAD_CONST', codeBody)
        self.code.emit('MAKE_FUNCTION', len(node.defaults))

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
            self.code.emit('SET_LINENO', node.lineno)
	self.visit(node.node)
	for arg in node.args:
	    self.visit(arg)
            if isinstance(arg, ast.Keyword):
                kw = kw + 1
            else:
                pos = pos + 1
	self.code.callFunction(kw << 8 | pos)
	return 1

    def visitKeyword(self, node):
        self.code.emit('LOAD_CONST', node.name)
        self.visit(node.expr)
        return 1

    def visitIf(self, node):
	after = StackRef()
	for test, suite in node.tests:
            if hasattr(test, 'lineno'):
                self.code.setLineNo(test.lineno)
            else:
                print "warning", "no line number"
	    self.visit(test)
	    dest = StackRef()
	    self.code.jumpIfFalse(dest)
	    self.code.popTop()
	    self.visit(suite)
	    self.code.jumpForward(after)
	    dest.bind(self.code.getCurInst())
	    self.code.popTop()
	if node.else_:
	    self.visit(node.else_)
	after.bind(self.code.getCurInst())
	return 1

    def startLoop(self):
        l = Loop()
        self.loops.push(l)
        self.code.emit('SETUP_LOOP', l.extentAnchor)
        return l

    def finishLoop(self):
        l = self.loops.pop()
        i = self.code.getCurInst()
        l.extentAnchor.bind(self.code.getCurInst())

    def visitFor(self, node):
        # three refs needed
        anchor = StackRef()

        self.code.emit('SET_LINENO', node.lineno)
        l = self.startLoop()
        self.visit(node.list)
        self.visit(ast.Const(0))
        l.startAnchor.bind(self.code.getCurInst())
        self.code.setLineNo(node.lineno)
        self.code.emit('FOR_LOOP', anchor)
        self.push(1)
        self.visit(node.assign)
        self.visit(node.body)
        self.code.emit('JUMP_ABSOLUTE', l.startAnchor)
        anchor.bind(self.code.getCurInst())
        self.code.emit('POP_BLOCK')
        if node.else_:
            self.visit(node.else_)
        self.finishLoop()
        return 1

    def visitWhile(self, node):
        self.code.emit('SET_LINENO', node.lineno)
        l = self.startLoop()
        if node.else_:
            lElse = StackRef()
        else:
            lElse = l.breakAnchor
        l.startAnchor.bind(self.code.getCurInst())
        self.code.emit('SET_LINENO', node.test.lineno)
        self.visit(node.test)
        self.code.emit('JUMP_IF_FALSE', lElse)
        self.code.emit('POP_TOP')
        self.visit(node.body)
        self.code.emit('JUMP_ABSOLUTE', l.startAnchor)
        # note that lElse may be an alias for l.breakAnchor
        lElse.bind(self.code.getCurInst())
        self.code.emit('POP_TOP')
        self.code.emit('POP_BLOCK')
        if node.else_:
            self.visit(node.else_)
        self.finishLoop()
        return 1

    def visitBreak(self, node):
        if not self.loops:
            raise SyntaxError, "'break' outside loop"
        self.code.emit('SET_LINENO', node.lineno)
        self.code.emit('BREAK_LOOP')

    def visitContinue(self, node):
        if not self.loops:
            raise SyntaxError, "'continue' outside loop"
        l = self.loops.top()
        self.code.emit('SET_LINENO', node.lineno)
        self.code.emit('JUMP_ABSOLUTE', l.startAnchor)
        

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
	    self.code.dupTop()
	    self.code.rotThree()
	    self.code.compareOp(op)
            # dupTop and compareOp cancel stack effect
	    self.code.jumpIfFalse(l1)
	    self.code.popTop()
            self.pop(1)
	if node.ops:
	    # emit the last comparison
	    op, code = node.ops[-1]
	    self.visit(code)
	    self.code.compareOp(op)
            self.pop(1)
	if len(node.ops) > 1:
	    self.code.jumpForward(l2)
	    l1.bind(self.code.getCurInst())
	    self.code.rotTwo()
	    self.code.popTop()
            self.pop(1)
	    l2.bind(self.code.getCurInst())
	return 1

    def visitGetattr(self, node):
        self.visit(node.expr)
        self.code.emit('LOAD_ATTR', node.attrname)
        self.push(1)
        return 1

    def visitSubscript(self, node):
        self.visit(node.expr)
        for sub in node.subs[:-1]:
            self.visit(sub)
            self.code.emit('BINARY_SUBSCR')
        self.visit(node.subs[-1])
        if node.flags == 'OP_APPLY':
            self.code.emit('BINARY_SUBSCR')
        else:
            self.code.emit('STORE_SUBSCR')
            
        return 1

    def visitSlice(self, node):
        self.visit(node.expr)
        slice = 0
        if node.lower:
            self.visit(node.lower)
            slice = slice | 1
            self.pop(1)
        if node.upper:
            self.visit(node.upper)
            slice = slice | 2
            self.pop(1)
        if node.flags == 'OP_APPLY':
            self.code.emit('SLICE+%d' % slice)
        elif node.flags == 'OP_ASSIGN':
            self.code.emit('STORE_SLICE+%d' % slice)
        elif node.flags == 'OP_DELETE':
            self.code.emit('DELETE_SLICE+%d' % slice)
        else:
            print node.flags
            raise
        return 1

    def visitAssign(self, node):
        self.code.setLineNo(node.lineno)
        self.visit(node.expr)
        for elt in node.nodes:
            if isinstance(elt, ast.Node):
                self.visit(elt)
        return 1

    def visitAssName(self, node):
        if node.flags != 'OP_ASSIGN':
            print "oops", node.flags
        self.storeName(node.name)
        self.pop(1)

    def visitAssAttr(self, node):
        if node.flags != 'OP_ASSIGN':
            print "warning: unexpected flags:", node.flags
            print node
        self.visit(node.expr)
        self.code.emit('STORE_ATTR', node.attrname)
        return 1

    def visitAssTuple(self, node):
        self.code.emit('UNPACK_TUPLE', len(node.nodes))
        for child in node.nodes:
            self.visit(child)
        return 1

    visitAssList = visitAssTuple

    def binaryOp(self, node, op):
	self.visit(node.left)
	self.visit(node.right)
	self.code.emit(op)
        self.pop(1)
	return 1

    def unaryOp(self, node, op):
        self.visit(node.expr)
        self.code.emit(op)
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

    def visitTest(self, node, jump):
        end = StackRef()
        for child in node.nodes[:-1]:
            self.visit(child)
            self.code.emit(jump, end)
            self.code.emit('POP_TOP')
        self.visit(node.nodes[-1])
        end.bind(self.code.getCurInst())
        return 1

    def visitAnd(self, node):
        return self.visitTest(node, 'JUMP_IF_FALSE')

    def visitOr(self, node):
        return self.visitTest(node, 'JUMP_IF_TRUE')

    def visitName(self, node):
        self.loadName(node.name)
        self.push(1)

    def visitConst(self, node):
	self.code.loadConst(node.value)
        self.push(1)
        return 1

    def visitTuple(self, node):
        for elt in node.nodes:
            self.visit(elt)
        self.code.emit('BUILD_TUPLE', len(node.nodes))
        self.pop(len(node.nodes))
        return 1

    def visitList(self, node):
        for elt in node.nodes:
            self.visit(elt)
        self.code.emit('BUILD_LIST', len(node.nodes))
        self.pop(len(node.nodes))
        return 1

    def visitReturn(self, node):
	self.code.setLineNo(node.lineno)
	self.visit(node.value)
	self.code.returnValue()
        self.pop(1)
        self.assertStackEmpty()
	return 1

    def visitRaise(self, node):
	self.code.setLineNo(node.lineno)
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
	self.code.raiseVarargs(n)
	return 1

    def visitPrint(self, node):
	self.code.setLineNo(node.lineno)
	for child in node.nodes:
	    self.visit(child)
	    self.code.emit('PRINT_ITEM')
        self.pop(len(node.nodes))
	return 1

    def visitPrintnl(self, node):
	self.visitPrint(node)
	self.code.emit('PRINT_NEWLINE')
	return 1

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

class StackRef:
    """Manage stack locations for jumps, loops, etc."""
    count = 0

    def __init__(self, id=None, val=None):
	if id is None:
	    id = StackRef.count
	    StackRef.count = StackRef.count + 1
	self.id = id
	self.val = val

    def __repr__(self):
	if self.val:
	    return "StackRef(val=%d)" % self.val
	else:
	    return "StackRef(id=%d)" % self.id

    def bind(self, inst):
	self.val = inst

    def resolve(self):
        if self.val is None:
            print "UNRESOLVE REF", self
            return 0
	return self.val

def add_hook(hooks, type, meth):
    """Helper function for PythonVMCode _emit_hooks"""
    l = hooks.get(type, [])
    l.append(meth)
    hooks[type] = l

class PythonVMCode:
    """Creates Python code objects
    
    The new module is used to create the code object.  The following
    attribute definitions are included from the reference manual:
        
    co_name gives the function name
    co_argcount is the number of positional arguments (including
        arguments with default values) 
    co_nlocals is the number of local variables used by the function
        (including arguments)  
    co_varnames is a tuple containing the names of the local variables
        (starting with the argument names) 
    co_code is a string representing the sequence of bytecode instructions 
    co_consts is a tuple containing the literals used by the bytecode
    co_names is a tuple containing the names used by the bytecode
    co_filename is the filename from which the code was compiled
    co_firstlineno is the first line number of the function
    co_lnotab is a string encoding the mapping from byte code offsets
        to line numbers (for detais see the source code of the
        interpreter)
        see code com_set_lineno and com_add_lnotab
        it's a string with 2bytes per set_lineno
        
    co_stacksize is the required stack size (including local variables)
    co_flags is an integer encoding a number of flags for the
        interpreter.

    The following flag bits are defined for co_flags: bit 2 is set if
    the function uses the "*arguments" syntax to accept an arbitrary
    number of positional arguments; bit 3 is set if the function uses
    the "**keywords" syntax to accept arbitrary keyword arguments;
    other bits are used internally or reserved for future use.

    If a code object represents a function, the first item in
    co_consts is the documentation string of the function, or None if
    undefined.
    """

    # XXX flag bits
    CO_OPTIMIZED = 0x0001  # uses LOAD_FAST!
    CO_NEWLOCALS = 0x0002  # everybody uses this?
    CO_VARARGS = 0x0004
    CO_VARKEYWORDS = 0x0008

    def __init__(self, args=(), name='?', filename='<?>',
                 docstring=None):
        # XXX why is the default value for flags 3?
	self.insts = []
        # used by makeCodeObject
        self.argcount = len(args)
        self.code = ''
        self.consts = [docstring]
        self.filename = filename
        self.flags = self.CO_NEWLOCALS
        self.name = name
        self.names = []
        self.varnames = list(args) or []
        # lnotab support
        self.firstlineno = 0
        self.lastlineno = 0
        self.last_addr = 0
        self.lnotab = ''

    def __repr__(self):
        return "<bytecode: %d instrs>" % len(self.insts)

    def setFlags(self, val):
        """XXX for module's function"""
        self.flags = val

    def setOptimized(self):
        self.flags = self.flags | self.CO_OPTIMIZED

    def setVarArgs(self):
        self.flags = self.flags | self.CO_VARARGS

    def setKWArgs(self):
        self.flags = self.flags | self.CO_VARKEYWORDS

    def getCurInst(self):
	return len(self.insts)

    def getNextInst(self):
	return len(self.insts) + 1

    def dump(self, io=sys.stdout):
        i = 0
        for inst in self.insts:
            if inst[0] == 'SET_LINENO':
                io.write("\n")
            io.write("    %3d " % i)
            if len(inst) == 1:
                io.write("%s\n" % inst)
            else:
                io.write("%-15.15s\t%s\n" % inst)
            i = i + 1

    def makeCodeObject(self, stacksize):
        """Make a Python code object

        This creates a Python code object using the new module.  This
        seems simpler than reverse-engineering the way marshal dumps
        code objects into .pyc files.  One of the key difficulties is
        figuring out how to layout references to code objects that
        appear on the VM stack; e.g.
          3 SET_LINENO          1
          6 LOAD_CONST          0 (<code object fact at 8115878 [...]
          9 MAKE_FUNCTION       0
         12 STORE_NAME          0 (fact)
        """
        
        self._findOffsets()
        lnotab = LineAddrTable()
        for t in self.insts:
            opname = t[0]
            if len(t) == 1:
                lnotab.addCode(chr(self.opnum[opname]))
            elif len(t) == 2:
                oparg = self._convertArg(opname, t[1])
                if opname == 'SET_LINENO':
                    lnotab.nextLine(oparg)
                try:
                    hi, lo = divmod(oparg, 256)
                except TypeError:
                    raise TypeError, "untranslated arg: %s, %s" % (opname, oparg)
                lnotab.addCode(chr(self.opnum[opname]) + chr(lo) +
                               chr(hi))
        # why is a module a special case?
        if self.flags == 0:
            nlocals = 0
        else:
            nlocals = len(self.varnames)
        # XXX danger! can't pass through here twice
        if self.flags & self.CO_VARKEYWORDS:
            self.argcount = self.argcount - 1
        co = new.code(self.argcount, nlocals, stacksize,
                      self.flags, lnotab.getCode(), self._getConsts(),
                      tuple(self.names), tuple(self.varnames),
                      self.filename, self.name, self.firstlineno,
                      lnotab.getTable())
        return co

    def _getConsts(self):
        """Return a tuple for the const slot of a code object

        Converts PythonVMCode objects to code objects
        """
        l = []
        for elt in self.consts:
            if isinstance(elt, CodeGenerator):
                l.append(elt.emit())
            else:
                l.append(elt)
        return tuple(l)

    def _findOffsets(self):
        """Find offsets for use in resolving StackRefs"""
        self.offsets = []
        cur = 0
        for t in self.insts:
            self.offsets.append(cur)
            l = len(t)
            if l == 1:
                cur = cur + 1
            elif l == 2:
                cur = cur + 3
                arg = t[1]
                # XXX this is a total hack: for a reference used
                # multiple times, we create a list of offsets and
                # expect that we when we pass through the code again
                # to actually generate the offsets, we'll pass in the
                # same order.
                if isinstance(arg, StackRef):
                    try:
                        arg.__offset.append(cur)
                    except AttributeError:
                        arg.__offset = [cur]

    def _convertArg(self, op, arg):
        """Convert the string representation of an arg to a number

        The specific handling depends on the opcode.

        XXX This first implementation isn't going to be very
        efficient. 
        """
        if op == 'SET_LINENO':
            return arg
        if op == 'LOAD_CONST':
            return self._lookupName(arg, self.consts)
        if op in self.localOps:
            # make sure it's in self.names, but use the bytecode offset
            self._lookupName(arg, self.names)
            return self._lookupName(arg, self.varnames)
        if op in self.globalOps:
            return self._lookupName(arg, self.names)
        if op in self.nameOps:
            return self._lookupName(arg, self.names)
        if op == 'COMPARE_OP':
            return self.cmp_op.index(arg)
        if self.hasjrel.has_elt(op):
            offset = arg.__offset[0]
            del arg.__offset[0]
            return self.offsets[arg.resolve()] - offset
        if self.hasjabs.has_elt(op):
            return self.offsets[arg.resolve()]
        return arg

    nameOps = ('STORE_NAME', 'IMPORT_NAME', 'IMPORT_FROM',
               'STORE_ATTR', 'LOAD_ATTR', 'LOAD_NAME', 'DELETE_NAME')
    localOps = ('LOAD_FAST', 'STORE_FAST', 'DELETE_FAST')
    globalOps = ('LOAD_GLOBAL', 'STORE_GLOBAL', 'DELETE_GLOBAL')

    def _lookupName(self, name, list, list2=None):
        """Return index of name in list, appending if necessary

        Yicky hack: Second list can be used for lookup of local names
        where the name needs to be added to varnames and names.
        """
        if name in list:
            return list.index(name)
        else:
            end = len(list)
            list.append(name)
            if list2 is not None:
                list2.append(name)
            return end

    # Convert some stuff from the dis module for local use
    
    cmp_op = list(dis.cmp_op)
    hasjrel = misc.Set()
    for i in dis.hasjrel:
        hasjrel.add(dis.opname[i])
    hasjabs = misc.Set()
    for i in dis.hasjabs:
        hasjabs.add(dis.opname[i])
    
    opnum = {}
    for num in range(len(dis.opname)):
	opnum[dis.opname[num]] = num

    # the interface below here seemed good at first.  upon real use,
    # it seems redundant to add a function for each opcode,
    # particularly because the method and opcode basically have the
    # same name.
    # on the other hand, we need to track things like stack depth in
    # order to generator code objects.  if we wrap instructions in a
    # method, we get an easy way to track these.  a simpler
    # approach, however, would be to define hooks that can be called
    # by emit.

    def setLineNo(self, num):
	self.emit('SET_LINENO', num)

    def popTop(self):
	self.emit('POP_TOP')

    def dupTop(self):
	self.emit('DUP_TOP')

    def rotTwo(self):
	self.emit('ROT_TWO')

    def rotThree(self):
	self.emit('ROT_THREE')

    def jumpIfFalse(self, dest):
	self.emit('JUMP_IF_FALSE', dest)

    def loadFast(self, name):
	self.emit('LOAD_FAST', name)

    def loadGlobal(self, name):
	self.emit('LOAD_GLOBAL', name)

    def binaryAdd(self):
	self.emit('BINARY_ADD')

    def compareOp(self, op):
	self.emit('COMPARE_OP', op)

    def loadConst(self, val):
	self.emit('LOAD_CONST', val)

    def returnValue(self):
	self.emit('RETURN_VALUE')

    def jumpForward(self, dest):
	self.emit('JUMP_FORWARD', dest)

    def raiseVarargs(self, num):
	self.emit('RAISE_VARARGS', num)

    def callFunction(self, num):
	self.emit('CALL_FUNCTION', num)

    # this version of emit + arbitrary hooks might work, but it's damn
    # messy.

    def emit(self, *args):
        self._emitDispatch(args[0], args[1:])
	self.insts.append(args)

    def _emitDispatch(self, type, args):
        for func in self._emit_hooks.get(type, []):
            func(self, args)

    _emit_hooks = {}

class LineAddrTable:
    """lnotab
    
    This class builds the lnotab, which is undocumented but described
    by com_set_lineno in compile.c.  Here's an attempt at explanation:

    For each SET_LINENO instruction after the first one, two bytes are
    added to lnotab.  (In some cases, multiple two-byte entries are
    added.)  The first byte is the distance in bytes between the
    instruction for the last SET_LINENO and the current SET_LINENO.
    The second byte is offset in line numbers.  If either offset is
    greater than 255, multiple two-byte entries are added -- one entry
    for each factor of 255.
    """

    def __init__(self):
        self.code = []
        self.codeOffset = 0
        self.firstline = 0
        self.lastline = 0
        self.lastoff = 0
        self.lnotab = []

    def addCode(self, code):
        self.code.append(code)
        self.codeOffset = self.codeOffset + len(code)

    def nextLine(self, lineno):
        if self.firstline == 0:
            self.firstline = lineno
            self.lastline = lineno
        else:
            # compute deltas
            addr = self.codeOffset - self.lastoff
            line = lineno - self.lastline
            while addr > 0 or line > 0:
                # write the values in 1-byte chunks that sum
                # to desired value
                trunc_addr = addr
                trunc_line = line
                if trunc_addr > 255:
                    trunc_addr = 255
                if trunc_line > 255:
                    trunc_line = 255
                self.lnotab.append(trunc_addr)
                self.lnotab.append(trunc_line)
                addr = addr - trunc_addr
                line = line - trunc_line
            self.lastline = lineno
            self.lastoff = self.codeOffset

    def getCode(self):
        return string.join(self.code, '')

    def getTable(self):
        return string.join(map(chr, self.lnotab), '')
    
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
        self.code = cg.emit()

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
            print k
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
