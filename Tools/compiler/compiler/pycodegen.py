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

def parse(path):
    f = open(path)
    src = f.read()
    f.close()
    t = transformer.Transformer()
    return t.parsesuite(src)

def walk(tree, visitor):
    w = ASTVisitor()
    w.preorder(tree, visitor)
    return w.visitor

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
        if self.VERBOSE:
            print "dispatch", className, (meth and meth.__name__ or '')
        if meth:
            return meth(node)

class CodeGenerator:
    def __init__(self):
	self.code = PythonVMCode()
	self.locals = misc.Stack()

    def visitDiscard(self, node):
        return 1

    def visitModule(self, node):
	lnf = walk(node.node, LocalNameFinder())
	self.locals.push(lnf.getLocals())
        self.visit(node.node)
        self.code.emit('LOAD_CONST', 'None')
        self.code.emit('RETURN_VALUE')
        return 1

    def visitFunction(self, node):
        codeBody = NestedCodeGenerator(node.code, node.argnames)
        walk(node.code, codeBody)
        self.code.setLineNo(node.lineno)
        self.code.emit('LOAD_CONST', codeBody.code)
        self.code.emit('MAKE_FUNCTION')
        self.code.emit('STORE_NAME', node.name)
        return 1

    def visitCallFunc(self, node):
	self.visit(node.node)
	for arg in node.args:
	    self.visit(arg)
	self.code.callFunction(len(node.args))
	return 1

    def visitIf(self, node):
	after = ForwardRef()
	for test, suite in node.tests:
	    self.code.setLineNo(test.lineno)
	    self.visit(test)
	    dest = ForwardRef()
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
	l1 = ForwardRef()
	l2 = ForwardRef()
	for op, code in node.ops[:-1]:
	    # emit every comparison except the last
	    self.visit(code)
	    self.code.dupTop()
	    self.code.rotThree()
	    self.code.compareOp(op)
	    self.code.jumpIfFalse(l1)
	    self.code.popTop()
	if node.ops:
	    # emit the last comparison
	    op, code = node.ops[-1]
	    self.visit(code)
	    self.code.compareOp(op)
	if len(node.ops) > 1:
	    self.code.jumpForward(l2)
	    l1.bind(self.code.getCurInst())
	    self.code.rotTwo()
	    self.code.popTop()
	    l2.bind(self.code.getCurInst())
	return 1

    def binaryOp(self, node, op):
	self.visit(node.left)
	self.visit(node.right)
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

    def visitName(self, node):
	locals = self.locals.top()
	if locals.has_elt(node.name):
	    self.code.loadFast(node.name)
	else:
	    self.code.loadGlobal(node.name)

    def visitConst(self, node):
	self.code.loadConst(node.value)

    def visitReturn(self, node):
	self.code.setLineNo(node.lineno)
	self.visit(node.value)
	self.code.returnValue()
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
	return 1

    def visitPrintnl(self, node):
	self.visitPrint(node)
	self.code.emit('PRINT_NEWLINE')
	return 1

class NestedCodeGenerator(CodeGenerator):
    """Generate code for a function object within another scope

    XXX not clear that this subclass is needed
    """
    super_init = CodeGenerator.__init__
    
    def __init__(self, code, args):
        """code and args of function or class being walked

        XXX need to separately pass to ASTVisitor.  the constructor
        only uses the code object to find the local names
        """
        self.super_init()
        lnf = walk(code, LocalNameFinder(args))
        self.locals.push(lnf.getLocals())

    def visitFunction(self, node):
	lnf = walk(node.code, LocalNameFinder(node.argnames))
	self.locals.push(lnf.getLocals())
        # XXX need to handle def foo((a, b)):
	self.code.setLineNo(node.lineno)
        self.visit(node.code)
        self.code.emit('LOAD_CONST', 'None')
        self.code.emit('RETURN_VALUE')
        return 1
	

class LocalNameFinder:
    def __init__(self, names=()):
	self.names = misc.Set()
	for name in names:
	    self.names.add(name)

    def getLocals(self):
	return self.names

    def visitFunction(self, node):
        self.names.add(node.name)
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

class Label:
    def __init__(self, num):
	self.num = num
    def __repr__(self):
	return "Label(%d)" % self.num

class ForwardRef:
    count = 0

    def __init__(self, id=None, val=None):
	if id is None:
	    id = ForwardRef.count
	    ForwardRef.count = ForwardRef.count + 1
	self.id = id
	self.val = val

    def __repr__(self):
	if self.val:
	    return "ForwardRef(val=%d)" % self.val
	else:
	    return "ForwardRef(id=%d)" % self.id

    def bind(self, inst):
	self.val = inst

    def resolve(self):
	return self.val

class CompiledModule:
    """Store the code object for a compiled module

    XXX Not clear how the code objects will be stored.  Seems possible
    that a single code attribute is sufficient, because it will
    contains references to all the need code objects.  That might be
    messy, though.
    """
    MAGIC = (20121 | (ord('\r')<<16) | (ord('\n')<<24))

    def __init__(self):
        self.code = None

    def addCode(self, code):
        """addCode(self: SelfType, code: PythonVMCode)"""
	
    def dump(self, path):
        """create a .pyc file"""
        f = open(path, 'wb')
        f.write(self._pyc_header())
        marshal.dump(self.code, f)
        f.close()
        
    def _pyc_header(self, path):
        # compile.c uses marshal to write a long directly, with
        # calling the interface that would also generate a 1-byte code
        # to indicate the type of the value.  simplest way to get the
        # same effect is to call marshal and then skip the code.
        buf = marshal.dumps(self.MAGIC)[1:]
        # skip the mtime for now, since I don't have the write
        # structure to pass the filename being compiled into this
        # instance 
        return buf + chr(0) * 4
	
class PythonVMCode:

    def __init__(self):
	self.insts = []
        # used by makeCodeObject
        self.argcount = 0
        self.code = ''
        self.consts = []
        self.filename = ''
        self.firstlineno = 0
        self.flags = 0
        self.lnotab = None
        self.name = ''
        self.names = []
        self.nlocals = 0
        self.stacksize = 2
        self.varnames = []

    def __repr__(self):
        return "<bytecode: %d instrs>" % len(self.insts)

    def emit(self, *args):
	print "emit", args
	self.insts.append(args)

    def getCurInst(self):
	return len(self.insts)

    def getNextInst(self):
	return len(self.insts) + 1

    def convert(self):
	"""Convert human-readable names to real bytecode"""
	pass

    def makeCodeObject(self):
        """Make a Python code object"""
        code = []
        self._findOffsets()
        for t in self.insts:
            opname = t[0]
            if len(t) == 1:
                code.append(chr(self.opnum[opname]))
            elif len(t) == 2:
                oparg = self._convertArg(opname, t[1])
                hi, lo = divmod(oparg, 256)
                code.append(chr(self.opnum[opname]) + chr(lo) + chr(hi))
        return string.join(code, '')

    def _findOffsets(self):
        """Find offsets for use in resolving ForwardRefs"""
        self.offsets = []
        cur = 0
        for t in self.insts:
            self.offsets.append(cur)
            l = len(t)
            if l == 1:
                cur = cur + 1
            elif l == 2:
                arg = t[1]
                if isinstance(arg, ForwardRef):
                    arg.__offset = cur
                cur = cur + 3

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
        if op == 'LOAD_FAST':
            return self._lookupName(arg, self.varnames, self.names)
        if op == 'LOAD_GLOBAL':
            return self._lookupName(arg, self.names)
        if op == 'STORE_NAME':
            return self._lookupName(arg, self.names)
        if op == 'COMPARE_OP':
            return self.cmp_op.index(arg)
        if self.hasjrel.has_elt(op):
            return self.offsets[arg.resolve()]
        if self.hasjabs.has_elt(op):
            return self.offsets[arg.resolve()] - arg.__offset
        print op, arg
        return arg

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

if __name__ == "__main__":
    tree = parse('test.py')
    cg = CodeGenerator()
    ASTVisitor.VERBOSE = 1
    w = walk(tree, cg)
    w.VERBOSE = 1
    for i in range(len(cg.code.insts)):
	inst = cg.code.insts[i]
	if inst[0] == 'SET_LINENO':
	    print
	print "%4d" % i, inst
    code = cg.code.makeCodeObject()
