"""A flow graph representation for Python bytecode"""

import dis
import new
import string
import types

from compiler import misc

class FlowGraph:
    def __init__(self):
        self.current = self.entry = Block()
        self.exit = Block("exit")
        self.blocks = misc.Set()
        self.blocks.add(self.entry)
        self.blocks.add(self.exit)

    def startBlock(self, block):
        self.current = block

    def nextBlock(self, block=None):
        if block is None:
            block = self.newBlock()
        # XXX think we need to specify when there is implicit transfer
        # from one block to the next
        #
        # I think this strategy works: each block has a child
        # designated as "next" which is returned as the last of the
        # children.  because the nodes in a graph are emitted in
        # reverse post order, the "next" block will always be emitted
        # immediately after its parent.
        # Worry: maintaining this invariant could be tricky
        self.current.addNext(block)
        self.startBlock(block)

    def newBlock(self):
        b = Block()
        self.blocks.add(b)
        return b

    def startExitBlock(self):
        self.startBlock(self.exit)

    def emit(self, *inst):
        # XXX should jump instructions implicitly call nextBlock?
        if inst[0] == 'RETURN_VALUE':
            self.current.addOutEdge(self.exit)
        self.current.emit(inst)

    def getBlocks(self):
        """Return the blocks in reverse postorder

        i.e. each node appears before all of its successors
        """
        # XXX make sure every node that doesn't have an explicit next
        # is set so that next points to exit
        for b in self.blocks.elements():
            if b is self.exit:
                continue
            if not b.next:
                b.addNext(self.exit)
        order = dfs_postorder(self.entry, {})
        order.reverse()
        # hack alert
        if not self.exit in order:
            order.append(self.exit)
        return order

def dfs_postorder(b, seen):
    """Depth-first search of tree rooted at b, return in postorder"""
    order = []
    seen[b] = b
    for c in b.children():
        if seen.has_key(c):
            continue
        order = order + dfs_postorder(c, seen)
    order.append(b)
    return order

class Block:
    _count = 0

    def __init__(self, label=''):
        self.insts = []
        self.inEdges = misc.Set()
        self.outEdges = misc.Set()
        self.label = label
        self.bid = Block._count
        self.next = []
        Block._count = Block._count + 1

    def __repr__(self):
        if self.label:
            return "<block %s id=%d len=%d>" % (self.label, self.bid,
                                                len(self.insts)) 
        else:
            return "<block id=%d len=%d>" % (self.bid, len(self.insts))

    def __str__(self):
        insts = map(str, self.insts)
        return "<block %s %d:\n%s>" % (self.label, self.bid,
                                       string.join(insts, '\n')) 

    def emit(self, inst):
        op = inst[0]
        if op[:4] == 'JUMP':
            self.outEdges.add(inst[1])
        self.insts.append(inst)

    def getInstructions(self):
        return self.insts

    def addInEdge(self, block):
        self.inEdges.add(block)

    def addOutEdge(self, block):
        self.outEdges.add(block)

    def addNext(self, block):
        self.next.append(block)
        assert len(self.next) == 1, map(str, self.next)

    def children(self):
        return self.outEdges.elements() + self.next

# flags for code objects
CO_OPTIMIZED = 0x0001
CO_NEWLOCALS = 0x0002
CO_VARARGS = 0x0004
CO_VARKEYWORDS = 0x0008

# the FlowGraph is transformed in place; it exists in one of these states
RAW = "RAW"
FLAT = "FLAT"
CONV = "CONV"
DONE = "DONE"

class PyFlowGraph(FlowGraph):
    super_init = FlowGraph.__init__

    def __init__(self, name, filename, args=(), optimized=0):
        self.super_init()
        self.name = name
        self.filename = filename
        self.docstring = None
        self.args = args # XXX
        self.argcount = getArgCount(args)
        if optimized:
            self.flags = CO_OPTIMIZED | CO_NEWLOCALS 
        else:
            self.flags = 0
        self.consts = []
        self.names = []
        self.varnames = list(args) or []
        for i in range(len(self.varnames)):
            var = self.varnames[i]
            if isinstance(var, TupleArg):
                self.varnames[i] = var.getName()
        self.stage = RAW

    def setDocstring(self, doc):
        self.docstring = doc
        self.consts.insert(0, doc)

    def setFlag(self, flag):
        self.flags = self.flags | flag
        if flag == CO_VARARGS:
            self.argcount = self.argcount - 1

    def getCode(self):
        """Get a Python code object"""
        if self.stage == RAW:
            self.flattenGraph()
        if self.stage == FLAT:
            self.convertArgs()
        if self.stage == CONV:
            self.makeByteCode()
        if self.stage == DONE:
            return self.newCodeObject()
        raise RuntimeError, "inconsistent PyFlowGraph state"

    def dump(self, io=None):
        if io:
            save = sys.stdout
            sys.stdout = io
        pc = 0
        for t in self.insts:
            opname = t[0]
            if opname == "SET_LINENO":
                print
            if len(t) == 1:
                print "\t", "%3d" % pc, opname
                pc = pc + 1
            else:
                print "\t", "%3d" % pc, opname, t[1]
                pc = pc + 3
        if io:
            sys.stdout = save

    def flattenGraph(self):
        """Arrange the blocks in order and resolve jumps"""
        assert self.stage == RAW
        self.insts = insts = []
        pc = 0
        begin = {}
        end = {}
        for b in self.getBlocks():
            begin[b] = pc
            for inst in b.getInstructions():
                insts.append(inst)
                if len(inst) == 1:
                    pc = pc + 1
                else:
                    # arg takes 2 bytes
                    pc = pc + 3
            end[b] = pc
        pc = 0
        for i in range(len(insts)):
            inst = insts[i]
            if len(inst) == 1:
                pc = pc + 1
            else:
                pc = pc + 3
            opname = inst[0]
            if self.hasjrel.has_elt(opname):
                oparg = inst[1]
                offset = begin[oparg] - pc
                insts[i] = opname, offset
            elif self.hasjabs.has_elt(opname):
                insts[i] = opname, begin[inst[1]]
        self.stacksize = findDepth(self.insts)
        self.stage = FLAT

    hasjrel = misc.Set()
    for i in dis.hasjrel:
        hasjrel.add(dis.opname[i])
    hasjabs = misc.Set()
    for i in dis.hasjabs:
        hasjabs.add(dis.opname[i])

    def convertArgs(self):
        """Convert arguments from symbolic to concrete form"""
        assert self.stage == FLAT
        for i in range(len(self.insts)):
            t = self.insts[i]
            if len(t) == 2:
                opname = t[0]
                oparg = t[1]
                conv = self._converters.get(opname, None)
                if conv:
                    self.insts[i] = opname, conv(self, oparg)
        self.stage = CONV

    def _lookupName(self, name, list):
        """Return index of name in list, appending if necessary"""
        found = None
        t = type(name)
        for i in range(len(list)):
            # must do a comparison on type first to prevent UnicodeErrors 
            if t == type(list[i]) and list[i] == name:
                found = 1
                break
        if found:
            # this is cheap, but incorrect in some cases, e.g 2 vs. 2L
            if type(name) == type(list[i]):
                return i
            for i in range(len(list)):
                elt = list[i]
                if type(elt) == type(name) and elt == name:
                    return i
        end = len(list)
        list.append(name)
        return end

    _converters = {}
    def _convert_LOAD_CONST(self, arg):
        return self._lookupName(arg, self.consts)

    def _convert_LOAD_FAST(self, arg):
        self._lookupName(arg, self.names)
        return self._lookupName(arg, self.varnames)
    _convert_STORE_FAST = _convert_LOAD_FAST
    _convert_DELETE_FAST = _convert_LOAD_FAST

    def _convert_NAME(self, arg):
        return self._lookupName(arg, self.names)
    _convert_LOAD_NAME = _convert_NAME
    _convert_STORE_NAME = _convert_NAME
    _convert_DELETE_NAME = _convert_NAME
    _convert_IMPORT_NAME = _convert_NAME
    _convert_IMPORT_FROM = _convert_NAME
    _convert_STORE_ATTR = _convert_NAME
    _convert_LOAD_ATTR = _convert_NAME
    _convert_DELETE_ATTR = _convert_NAME
    _convert_LOAD_GLOBAL = _convert_NAME
    _convert_STORE_GLOBAL = _convert_NAME
    _convert_DELETE_GLOBAL = _convert_NAME

    _cmp = list(dis.cmp_op)
    def _convert_COMPARE_OP(self, arg):
        return self._cmp.index(arg)

    # similarly for other opcodes...

    for name, obj in locals().items():
        if name[:9] == "_convert_":
            opname = name[9:]
            _converters[opname] = obj            
    del name, obj, opname

    def makeByteCode(self):
        assert self.stage == CONV
        self.lnotab = lnotab = LineAddrTable()
        for t in self.insts:
            opname = t[0]
            if len(t) == 1:
                lnotab.addCode(self.opnum[opname])
            else:
                oparg = t[1]
                if opname == "SET_LINENO":
                    lnotab.nextLine(oparg)
                hi, lo = twobyte(oparg)
                try:
                    lnotab.addCode(self.opnum[opname], lo, hi)
                except ValueError:
                    print opname, oparg
                    print self.opnum[opname], lo, hi
                    raise
        self.stage = DONE

    opnum = {}
    for num in range(len(dis.opname)):
        opnum[dis.opname[num]] = num
    del num

    def newCodeObject(self):
        assert self.stage == DONE
        if self.flags == 0:
            nlocals = 0
        else:
            nlocals = len(self.varnames)
        argcount = self.argcount
        if self.flags & CO_VARKEYWORDS:
            argcount = argcount - 1
        return new.code(argcount, nlocals, self.stacksize, self.flags,
                        self.lnotab.getCode(), self.getConsts(),
                        tuple(self.names), tuple(self.varnames),
                        self.filename, self.name, self.lnotab.firstline,
                        self.lnotab.getTable())

    def getConsts(self):
        """Return a tuple for the const slot of the code object

        Must convert references to code (MAKE_FUNCTION) to code
        objects recursively.
        """
        l = []
        for elt in self.consts:
            if isinstance(elt, PyFlowGraph):
                elt = elt.getCode()
            l.append(elt)
        return tuple(l)
            
def isJump(opname):
    if opname[:4] == 'JUMP':
        return 1

class TupleArg:
    """Helper for marking func defs with nested tuples in arglist"""
    def __init__(self, count, names):
        self.count = count
        self.names = names
    def __repr__(self):
        return "TupleArg(%s, %s)" % (self.count, self.names)
    def getName(self):
        return ".nested%d" % self.count

def getArgCount(args):
    argcount = len(args)
    if args:
        for arg in args:
            if isinstance(arg, TupleArg):
                numNames = len(misc.flatten(arg.names))
                argcount = argcount - numNames
    return argcount

def twobyte(val):
    """Convert an int argument into high and low bytes"""
    assert type(val) == types.IntType
    return divmod(val, 256)

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

    def addCode(self, *args):
        for arg in args:
            self.code.append(chr(arg))
        self.codeOffset = self.codeOffset + len(args)

    def nextLine(self, lineno):
        if self.firstline == 0:
            self.firstline = lineno
            self.lastline = lineno
        else:
            # compute deltas
            addr = self.codeOffset - self.lastoff
            line = lineno - self.lastline
            # Python assumes that lineno always increases with
            # increasing bytecode address (lnotab is unsigned char).
            # Depending on when SET_LINENO instructions are emitted
            # this is not always true.  Consider the code:
            #     a = (1,
            #          b)
            # In the bytecode stream, the assignment to "a" occurs
            # after the loading of "b".  This works with the C Python
            # compiler because it only generates a SET_LINENO instruction
            # for the assignment.
            if line > 0:
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
    
class StackDepthTracker:
    # XXX 1. need to keep track of stack depth on jumps
    # XXX 2. at least partly as a result, this code is broken

    def findDepth(self, insts):
        depth = 0
        maxDepth = 0
        for i in insts:
            opname = i[0]
            delta = self.effect.get(opname, 0)
            if delta > 1:
                depth = depth + delta
            elif delta < 0:
                if depth > maxDepth:
                    maxDepth = depth
                depth = depth + delta
            else:
                if depth > maxDepth:
                    maxDepth = depth
                # now check patterns
                for pat, pat_delta in self.patterns:
                    if opname[:len(pat)] == pat:
                        delta = pat_delta
                        depth = depth + delta
                        break
                # if we still haven't found a match
                if delta == 0:
                    meth = getattr(self, opname, None)
                    if meth is not None:
                        depth = depth + meth(i[1])
            if depth < 0:
                depth = 0
        return maxDepth

    effect = {
        'POP_TOP': -1,
        'DUP_TOP': 1,
        'SLICE+1': -1,
        'SLICE+2': -1,
        'SLICE+3': -2,
        'STORE_SLICE+0': -1,
        'STORE_SLICE+1': -2,
        'STORE_SLICE+2': -2,
        'STORE_SLICE+3': -3,
        'DELETE_SLICE+0': -1,
        'DELETE_SLICE+1': -2,
        'DELETE_SLICE+2': -2,
        'DELETE_SLICE+3': -3,
        'STORE_SUBSCR': -3,
        'DELETE_SUBSCR': -2,
        # PRINT_EXPR?
        'PRINT_ITEM': -1,
        'LOAD_LOCALS': 1,
        'RETURN_VALUE': -1,
        'EXEC_STMT': -2,
        'BUILD_CLASS': -2,
        'STORE_NAME': -1,
        'STORE_ATTR': -2,
        'DELETE_ATTR': -1,
        'STORE_GLOBAL': -1,
        'BUILD_MAP': 1,
        'COMPARE_OP': -1,
        'STORE_FAST': -1,
        'IMPORT_STAR': -1,
        'IMPORT_NAME': 0,
        'IMPORT_FROM': 1,
        }
    # use pattern match
    patterns = [
        ('BINARY_', -1),
        ('LOAD_', 1),
        ]
    
    # special cases:
    # UNPACK_SEQUENCE, BUILD_TUPLE,
    # BUILD_LIST, CALL_FUNCTION, MAKE_FUNCTION, BUILD_SLICE
    def UNPACK_SEQUENCE(self, count):
        return count
    def BUILD_TUPLE(self, count):
        return -count
    def BUILD_LIST(self, count):
        return -count
    def CALL_FUNCTION(self, argc):
        hi, lo = divmod(argc, 256)
        return lo + hi * 2
    def CALL_FUNCTION_VAR(self, argc):
        return self.CALL_FUNCTION(argc)+1
    def CALL_FUNCTION_KW(self, argc):
        return self.CALL_FUNCTION(argc)+1
    def CALL_FUNCTION_VAR_KW(self, argc):
        return self.CALL_FUNCTION(argc)+2
    def MAKE_FUNCTION(self, argc):
        return -argc
    def BUILD_SLICE(self, argc):
        if argc == 2:
            return -1
        elif argc == 3:
            return -2
    
findDepth = StackDepthTracker().findDepth
