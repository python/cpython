"""Assembler for Python bytecode

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
    to line numbers.  see LineAddrTable below.
co_stacksize is the required stack size (including local variables)
co_flags is an integer encoding a number of flags for the
    interpreter.  There are four flags:
    CO_OPTIMIZED -- uses load fast
    CO_NEWLOCALS -- everything?
    CO_VARARGS -- use *args
    CO_VARKEYWORDS -- uses **args

If a code object represents a function, the first item in co_consts is
the documentation string of the function, or None if undefined.
"""

import sys
import dis
import new
import string

import misc

# flags for code objects
CO_OPTIMIZED = 0x0001
CO_NEWLOCALS = 0x0002
CO_VARARGS = 0x0004
CO_VARKEYWORDS = 0x0008

class PyAssembler:
    """Creates Python code objects
    """

    # XXX this class needs to major refactoring

    def __init__(self, args=(), name='?', filename='<?>',
                 docstring=None):
        # XXX why is the default value for flags 3?
	self.insts = []
        # used by makeCodeObject
        self.argcount = len(args)
        self.code = ''
        self.consts = [docstring]
        self.filename = filename
        self.flags = CO_NEWLOCALS
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
        self.flags = self.flags | CO_OPTIMIZED

    def setVarArgs(self):
        self.flags = self.flags | CO_VARARGS

    def setKWArgs(self):
        self.flags = self.flags | CO_VARKEYWORDS

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

    def makeCodeObject(self):
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
        if self.flags & CO_VARKEYWORDS:
            self.argcount = self.argcount - 1
	stacksize = findDepth(self.insts)
        try:
            co = new.code(self.argcount, nlocals, stacksize,
                          self.flags, lnotab.getCode(), self._getConsts(),
                          tuple(self.names), tuple(self.varnames),
                          self.filename, self.name, self.firstlineno,
                          lnotab.getTable())
        except SystemError, err:
            print err
            print repr(self.argcount)
            print repr(nlocals)
            print repr(stacksize)
            print repr(self.flags)
            print repr(lnotab.getCode())
            print repr(self._getConsts())
            print repr(self.names)
            print repr(self.varnames)
            print repr(self.filename)
            print repr(self.name)
            print repr(self.firstlineno)
            print repr(lnotab.getTable())
            raise
        return co

    def _getConsts(self):
        """Return a tuple for the const slot of a code object

        Converts PythonVMCode objects to code objects
        """
        l = []
        for elt in self.consts:
	    # XXX might be clearer to just as isinstance(CodeGen)
	    if hasattr(elt, 'asConst'):
		l.append(elt.asConst())
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

class StackDepthTracker:
    # XXX need to keep track of stack depth on jumps

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
		for pat, delta in self.patterns:
		    if opname[:len(pat)] == pat:
			depth = depth + delta
			break
		# if we still haven't found a match
		if delta == 0:
		    meth = getattr(self, opname)
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
	}
    # use pattern match
    patterns = [
	('BINARY_', -1),
	('LOAD_', 1),
	('IMPORT_', 1),
	]
    # special cases

    #: UNPACK_TUPLE, UNPACK_LIST, BUILD_TUPLE,
    # BUILD_LIST, CALL_FUNCTION, MAKE_FUNCTION, BUILD_SLICE
    def UNPACK_TUPLE(self, count):
	return count
    def UNPACK_LIST(self, count):
	return count
    def BUILD_TUPLE(self, count):
	return -count
    def BUILD_LIST(self, count):
	return -count
    def CALL_FUNCTION(self, argc):
	hi, lo = divmod(argc, 256)
	return lo + hi * 2
    def MAKE_FUNCTION(self, argc):
	return -argc
    def BUILD_SLICE(self, argc):
	if argc == 2:
	    return -1
	elif argc == 3:
	    return -2
    
findDepth = StackDepthTracker().findDepth
