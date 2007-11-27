"""Disassembler of Python byte code into mnemonics."""

import sys
import types

from opcode import *
from opcode import __all__ as _opcodes_all

__all__ = ["dis","disassemble","distb","disco"] + _opcodes_all
del _opcodes_all

def dis(x=None):
    """Disassemble classes, methods, functions, or code.

    With no argument, disassemble the last traceback.

    """
    if x is None:
        distb()
        return
    if hasattr(x, '__func__'):
        x = x.__func__
    if hasattr(x, '__code__'):
        x = x.__code__
    if hasattr(x, '__dict__'):
        items = sorted(x.__dict__.items())
        for name, x1 in items:
            if isinstance(x1, (types.MethodType, types.FunctionType,
                               types.CodeType, type)):
                print("Disassembly of %s:" % name)
                try:
                    dis(x1)
                except TypeError as msg:
                    print("Sorry:", msg)
                print()
    elif hasattr(x, 'co_code'):
        disassemble(x)
    elif isinstance(x, str):
        disassemble_string(x)
    else:
        raise TypeError("don't know how to disassemble %s objects" %
                        type(x).__name__)

def distb(tb=None):
    """Disassemble a traceback (default: last traceback)."""
    if tb is None:
        try:
            tb = sys.last_traceback
        except AttributeError:
            raise RuntimeError("no last traceback to disassemble")
        while tb.tb_next: tb = tb.tb_next
    disassemble(tb.tb_frame.f_code, tb.tb_lasti)

# XXX This duplicates information from code.h, also duplicated in inspect.py.
# XXX Maybe this ought to be put in a central location, like opcode.py?
flag2name = {
     1: "OPTIMIZED",
     2: "NEWLOCALS",
     4: "VARARGS",
     8: "VARKEYWORDS",
    16: "NESTED",
    32: "GENERATOR",
    64: "NOFREE",
}

def pretty_flags(flags):
    """Return pretty representation of code flags."""
    names = []
    for i in range(32):
        flag = 1<<i
        if flags & flag:
            names.append(flag2name.get(flag, hex(flag)))
            flags ^= flag
            if not flags:
                break
    else:
        names.append(hex(flags))
    return ", ".join(names)

def show_code(co):
    """Show details about a code object."""
    print("Name:             ", co.co_name)
    print("Filename:         ", co.co_filename)
    print("Argument count:   ", co.co_argcount)
    print("Kw-only arguments:", co.co_kwonlyargcount)
    print("Number of locals: ", co.co_nlocals)
    print("Stack size:       ", co.co_stacksize)
    print("Flags:            ", pretty_flags(co.co_flags))
    if co.co_consts:
        print("Constants:")
        for i_c in enumerate(co.co_consts):
            print("%4d: %r" % i_c)
    if co.co_names:
        print("Names:")
        for i_n in enumerate(co.co_names):
            print("%4d: %s" % i_n)
    if co.co_varnames:
        print("Variable names:")
        for i_n in enumerate(co.co_varnames):
            print("%4d: %s" % i_n)
    if co.co_freevars:
        print("Free variables:")
        for i_n in enumerate(co.co_freevars):
            print("%4d: %s" % i_n)
    if co.co_cellvars:
        print("Cell variables:")
        for i_n in enumerate(co.co_cellvars):
            print("%4d: %s" % i_n)

def disassemble(co, lasti=-1):
    """Disassemble a code object."""
    code = co.co_code
    labels = findlabels(code)
    linestarts = dict(findlinestarts(co))
    n = len(code)
    i = 0
    extended_arg = 0
    free = None
    while i < n:
        op = code[i]
        if i in linestarts:
            if i > 0:
                print()
            print("%3d" % linestarts[i], end=' ')
        else:
            print('   ', end=' ')

        if i == lasti: print('-->', end=' ')
        else: print('   ', end=' ')
        if i in labels: print('>>', end=' ')
        else: print('  ', end=' ')
        print(repr(i).rjust(4), end=' ')
        print(opname[op].ljust(20), end=' ')
        i = i+1
        if op >= HAVE_ARGUMENT:
            oparg = code[i] + code[i+1]*256 + extended_arg
            extended_arg = 0
            i = i+2
            if op == EXTENDED_ARG:
                extended_arg = oparg*65536
            print(repr(oparg).rjust(5), end=' ')
            if op in hasconst:
                print('(' + repr(co.co_consts[oparg]) + ')', end=' ')
            elif op in hasname:
                print('(' + co.co_names[oparg] + ')', end=' ')
            elif op in hasjrel:
                print('(to ' + repr(i + oparg) + ')', end=' ')
            elif op in haslocal:
                print('(' + co.co_varnames[oparg] + ')', end=' ')
            elif op in hascompare:
                print('(' + cmp_op[oparg] + ')', end=' ')
            elif op in hasfree:
                if free is None:
                    free = co.co_cellvars + co.co_freevars
                print('(' + free[oparg] + ')', end=' ')
        print()

def disassemble_string(code, lasti=-1, varnames=None, names=None,
                       constants=None):
    labels = findlabels(code)
    n = len(code)
    i = 0
    while i < n:
        op = code[i]
        if i == lasti: print('-->', end=' ')
        else: print('   ', end=' ')
        if i in labels: print('>>', end=' ')
        else: print('  ', end=' ')
        print(repr(i).rjust(4), end=' ')
        print(opname[op].ljust(15), end=' ')
        i = i+1
        if op >= HAVE_ARGUMENT:
            oparg = code[i] + code[i+1]*256
            i = i+2
            print(repr(oparg).rjust(5), end=' ')
            if op in hasconst:
                if constants:
                    print('(' + repr(constants[oparg]) + ')', end=' ')
                else:
                    print('(%d)'%oparg, end=' ')
            elif op in hasname:
                if names is not None:
                    print('(' + names[oparg] + ')', end=' ')
                else:
                    print('(%d)'%oparg, end=' ')
            elif op in hasjrel:
                print('(to ' + repr(i + oparg) + ')', end=' ')
            elif op in haslocal:
                if varnames:
                    print('(' + varnames[oparg] + ')', end=' ')
                else:
                    print('(%d)' % oparg, end=' ')
            elif op in hascompare:
                print('(' + cmp_op[oparg] + ')', end=' ')
        print()

disco = disassemble                     # XXX For backwards compatibility

def findlabels(code):
    """Detect all offsets in a byte code which are jump targets.

    Return the list of offsets.

    """
    labels = []
    n = len(code)
    i = 0
    while i < n:
        op = code[i]
        i = i+1
        if op >= HAVE_ARGUMENT:
            oparg = code[i] + code[i+1]*256
            i = i+2
            label = -1
            if op in hasjrel:
                label = i+oparg
            elif op in hasjabs:
                label = oparg
            if label >= 0:
                if label not in labels:
                    labels.append(label)
    return labels

def findlinestarts(code):
    """Find the offsets in a byte code which are start of lines in the source.

    Generate pairs (offset, lineno) as described in Python/compile.c.

    """
    byte_increments = list(code.co_lnotab[0::2])
    line_increments = list(code.co_lnotab[1::2])

    lastlineno = None
    lineno = code.co_firstlineno
    addr = 0
    for byte_incr, line_incr in zip(byte_increments, line_increments):
        if byte_incr:
            if lineno != lastlineno:
                yield (addr, lineno)
                lastlineno = lineno
            addr += byte_incr
        lineno += line_incr
    if lineno != lastlineno:
        yield (addr, lineno)

def _test():
    """Simple test program to disassemble a file."""
    if sys.argv[1:]:
        if sys.argv[2:]:
            sys.stderr.write("usage: python dis.py [-|file]\n")
            sys.exit(2)
        fn = sys.argv[1]
        if not fn or fn == "-":
            fn = None
    else:
        fn = None
    if fn is None:
        f = sys.stdin
    else:
        f = open(fn)
    source = f.read()
    if fn is not None:
        f.close()
    else:
        fn = "<stdin>"
    code = compile(source, fn, "exec")
    dis(code)

if __name__ == "__main__":
    _test()
