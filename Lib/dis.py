"""Disassembler of Python byte code into mnemonics."""

import sys
import types
import collections
import io

__all__ = ["code_info", "dis", "disassemble", "distb", "disco",
           "findlinestarts", "findlabels", "show_code",
           "get_instructions", "Instruction", "Bytecode"]

_have_code = (types.MethodType, types.FunctionType, types.CodeType,
              classmethod, staticmethod, type)

try:
    import opcode as opcode_mod
    _opname = ['<%s>' % i if op is None else op for i, op in enumerate(opcode_mod.opname)]
    _opmap = {op: i for i, op in enumerate(opcode_mod.opname) if op is not None}
    _OPNAME_WIDTH = max(map(len, _opname))
    EXTENDED_ARG = _opmap['EXTENDED_ARG']
    FORMAT_VALUE = _opmap['FORMAT_VALUE']
    MAKE_FUNCTION = _opmap['MAKE_FUNCTION']
except ImportError:
    pass
FORMAT_VALUE_CONVERTERS = (
    (None, ''),
    (str, 'str'),
    (repr, 'repr'),
    (ascii, 'ascii'),
)
MAKE_FUNCTION_FLAGS = ('defaults', 'kwdefaults', 'annotations', 'closure')


def _try_compile(source, name):
    """Attempts to compile the given source, first as an expression and
       then as a statement if the first approach fails.

       Utility function to accept strings in functions that otherwise
       expect code objects
    """
    try:
        c = compile(source, name, 'eval')
    except SyntaxError:
        c = compile(source, name, 'exec')
    return c

def dis(x=None, *, file=None, depth=None):
    """Disassemble classes, methods, functions, and other compiled objects.

    With no argument, disassemble the last traceback.

    Compiled objects currently include generator objects, async generator
    objects, and coroutine objects, all of which store their code object
    in a special attribute.
    """
    if x is None:
        distb(file=file)
        return
    # Extract functions from methods.
    if hasattr(x, '__func__'):
        x = x.__func__
    # Extract compiled code objects from...
    if hasattr(x, '__code__'):  # ...a function, or
        x = x.__code__
    elif hasattr(x, 'gi_code'):  #...a generator object, or
        x = x.gi_code
    elif hasattr(x, 'ag_code'):  #...an asynchronous generator object, or
        x = x.ag_code
    elif hasattr(x, 'cr_code'):  #...a coroutine.
        x = x.cr_code
    # Perform the disassembly.
    if hasattr(x, '__dict__'):  # Class or module
        items = sorted(x.__dict__.items())
        for name, x1 in items:
            if isinstance(x1, _have_code):
                print("Disassembly of %s:" % name, file=file)
                try:
                    dis(x1, file=file, depth=depth)
                except TypeError as msg:
                    print("Sorry:", msg, file=file)
                print(file=file)
    elif hasattr(x, 'co_code'): # Code object
        _disassemble_recursive(x, file=file, depth=depth)
    elif isinstance(x, (bytes, bytearray)): # Raw bytecode
        _disassemble_bytes(None, x, file=file)
    elif isinstance(x, str):    # Source code
        _disassemble_recursive(_try_compile(x, '<dis>'), file=file, depth=depth)
    else:
        raise TypeError("don't know how to disassemble %s objects" %
                        type(x).__name__)

def distb(tb=None, *, file=None):
    """Disassemble a traceback (default: last traceback)."""
    if tb is None:
        try:
            tb = sys.last_traceback
        except AttributeError:
            raise RuntimeError("no last traceback to disassemble") from None
        while tb.tb_next: tb = tb.tb_next
    disassemble(tb.tb_frame.f_code, tb.tb_lasti, file=file)

# The inspect module interrogates this dictionary to build its
# list of CO_* constants. It is also used by pretty_flags to
# turn the co_flags field into a human readable list.
COMPILER_FLAG_NAMES = {
     1: "OPTIMIZED",
     2: "NEWLOCALS",
     4: "VARARGS",
     8: "VARKEYWORDS",
    16: "NESTED",
    32: "GENERATOR",
    64: "NOFREE",
   128: "COROUTINE",
   256: "ITERABLE_COROUTINE",
   512: "ASYNC_GENERATOR",
}

def pretty_flags(flags):
    """Return pretty representation of code flags."""
    names = []
    for i in range(32):
        flag = 1<<i
        if flags & flag:
            names.append(COMPILER_FLAG_NAMES.get(flag, hex(flag)))
            flags ^= flag
            if not flags:
                break
    else:
        names.append(hex(flags))
    return ", ".join(names)

def _get_code_object(x):
    """Helper to handle methods, compiled or raw code objects, and strings."""
    # Extract functions from methods.
    if hasattr(x, '__func__'):
        x = x.__func__
    # Extract compiled code objects from...
    if hasattr(x, '__code__'):  # ...a function, or
        x = x.__code__
    elif hasattr(x, 'gi_code'):  #...a generator object, or
        x = x.gi_code
    elif hasattr(x, 'ag_code'):  #...an asynchronous generator object, or
        x = x.ag_code
    elif hasattr(x, 'cr_code'):  #...a coroutine.
        x = x.cr_code
    # Handle source code.
    if isinstance(x, str):
        x = _try_compile(x, "<disassembly>")
    # By now, if we don't have a code object, we can't disassemble x.
    if hasattr(x, 'co_code'):
        return x
    raise TypeError("don't know how to disassemble %s objects" %
                    type(x).__name__)

def code_info(x):
    """Formatted details of methods, functions, or code."""
    return _format_code_info(_get_code_object(x))

def _format_code_info(co):
    lines = []
    lines.append("Name:              %s" % co.co_name)
    lines.append("Filename:          %s" % co.co_filename)
    lines.append("Argument count:    %s" % co.co_argcount)
    lines.append("Positional-only arguments: %s" % co.co_posonlyargcount)
    lines.append("Kw-only arguments: %s" % co.co_kwonlyargcount)
    lines.append("Number of locals:  %s" % co.co_nlocals)
    lines.append("Stack size:        %s" % co.co_stacksize)
    lines.append("Flags:             %s" % pretty_flags(co.co_flags))
    if co.co_consts:
        lines.append("Constants:")
        for i_c in enumerate(co.co_consts):
            lines.append("%4d: %r" % i_c)
    if co.co_names:
        lines.append("Names:")
        for i_n in enumerate(co.co_names):
            lines.append("%4d: %s" % i_n)
    if co.co_varnames:
        lines.append("Variable names:")
        for i_n in enumerate(co.co_varnames):
            lines.append("%4d: %s" % i_n)
    if co.co_freevars:
        lines.append("Free variables:")
        for i_n in enumerate(co.co_freevars):
            lines.append("%4d: %s" % i_n)
    if co.co_cellvars:
        lines.append("Cell variables:")
        for i_n in enumerate(co.co_cellvars):
            lines.append("%4d: %s" % i_n)
    return "\n".join(lines)

def show_code(co, *, file=None):
    """Print details of methods, functions, or code to *file*.

    If *file* is not provided, the output is printed on stdout.
    """
    print(code_info(co), file=file)

Instruction = collections.namedtuple('Instruction',
        'opcode opname opargs opargs_repr offset starts_line is_jump_target')
Instruction.opcode.__doc__ = 'Numeric code for operation'
Instruction.opname.__doc__ = 'Human readable name for operation'
Instruction.opargs.__doc__ = 'Numeric arguments to operation'
Instruction.opargs_repr.__doc__ = 'Human readable description of operation arguments'
Instruction.offset.__doc__ = 'Start index of operation within bytecode sequence'
Instruction.starts_line.__doc__ = 'Line started by this opcode (if any), otherwise None'
Instruction.is_jump_target.__doc__ = 'True if other code jumps to here, otherwise False'

def get_instructions(x, *, first_line=None):
    """Iterator for the opcodes in methods, functions or code

    Generates a series of Instruction named tuples giving the details of
    each operations in the supplied code.

    If *first_line* is not None, it indicates the line number that should
    be reported for the first source line in the disassembled code.
    Otherwise, the source line information (if any) is taken directly from
    the disassembled code object.
    """
    co = _get_code_object(x)
    linestarts = dict(findlinestarts(co))
    if first_line is not None:
        line_offset = first_line - co.co_firstlineno
    else:
        line_offset = 0
    return _get_instructions_bytes(co, None, linestarts, line_offset)

def _get_instructions_bytes(codeobj, code_bytes, linestarts, line_offset):
    """Iterate over the instructions in a bytecode string.

    Generates a sequence of Instruction namedtuples giving the details of each
    opcode_mod.  Additional information about the code's runtime environment
    (e.g. variable names, constants) can be specified using optional
    arguments.

    """
    if codeobj is not None:
        code_bytes = codeobj.co_code if code_bytes is None else code_bytes
        varnames = codeobj.co_varnames
        constants = codeobj.co_consts
        cells = codeobj.co_cellvars + codeobj.co_freevars
        names = codeobj.co_names
        tmp_offset = len(varnames)
        const_offset = tmp_offset + codeobj.co_ntmps

    labels = findlabels(code_bytes)
    starts_line = None
    for offset, opcode, opargs in _unpack_opargs(code_bytes):
        if linestarts is not None:
            starts_line = linestarts.get(offset, None)
            if starts_line is not None:
                starts_line += line_offset
        opargs_repr = []
        neg_addr = False
        for oparg, kind in zip(opargs, opcode_mod.oparg_kind_table[opcode]):
            match kind:
                case opcode_mod.OPARG_KIND_REG | opcode_mod.OPARG_KIND_REGS:
                    if codeobj is None:
                        arg_repr = '$%d' % oparg
                    elif oparg < tmp_offset:
                        arg_repr = varnames[oparg]
                    elif oparg < const_offset:
                        arg_repr = '$%d' % (oparg - tmp_offset)
                    else:
                        arg_repr = repr(constants[oparg - const_offset])
                    if kind == opcode_mod.OPARG_KIND_REGS:
                        arg_repr = arg_repr + '*'
                case opcode_mod.OPARG_KIND_FREE:
                    if codeobj is None:
                        arg_repr = '<cell-%d>' % oparg
                    else:
                        arg_repr = cells[oparg]
                case opcode_mod.OPARG_KIND_NAME:
                    if codeobj is None:
                        arg_repr = '<name-%d>' % oparg
                    else:
                        arg_repr = names[oparg]
                case opcode_mod.OPARG_KIND_ADDR:
                    arg_repr = '.' if neg_addr else '<to %d>' % (offset + (1 + oparg) * 4)
                case opcode_mod.OPARG_KIND_NEG_ADDR:
                    if oparg:
                        neg_addr = True
                        arg_repr = '<to %d>' % (offset + (1 - oparg) * 4)
                    else:
                        arg_repr = '.'
                case opcode_mod.OPARG_KIND_FLAG:
                    arg_repr = '<%s>' % hex(oparg)
                case _:
                    arg_repr = '.'
            opargs_repr.append(arg_repr)
        yield Instruction(opcode, _opname[opcode],
                          opargs, tuple(opargs_repr),
                          offset, starts_line, offset in labels)

def disassemble(co, lasti=-1, *, file=None):
    """Disassemble a code object."""
    _disassemble_bytes(co, None, lasti, dict(findlinestarts(co)), file=file)

def _disassemble_recursive(co, *, file=None, depth=None):
    disassemble(co, file=file)
    if depth is None or depth > 0:
        if depth is not None:
            depth = depth - 1
        for x in co.co_consts:
            if hasattr(x, 'co_code'):
                print(file=file)
                print("Disassembly of %r:" % (x,), file=file)
                _disassemble_recursive(x, file=file, depth=depth)

def _disassemble_bytes(codeobj, code_bytes, lasti=-1, linestarts=None,
                       *, file=None, line_offset=0):
    # Omit the line number column entirely if we have no line number info
    show_lineno = bool(linestarts)
    lineno_width = len(str(max(linestarts.values()) + line_offset)) if show_lineno else 0
    offset_width = len(str(len(code_bytes or codeobj.co_code) - 4))
    for instr in _get_instructions_bytes(codeobj, code_bytes, linestarts, line_offset):
        if show_lineno and instr.starts_line is not None and instr.offset > 0:
            print(file=file)
        dis_str = ' '.join([
            # Column: Source code line number
            str(instr.starts_line or '').rjust(lineno_width),
            # Column: Current instruction indicator
            '-->' if instr.offset == lasti else '   ',
            # Column: Jump target marker
            '>>' if instr.is_jump_target else '  ',
            # Column: Instruction offset from start of code sequence
            str(instr.offset).rjust(offset_width),
            # Column: Opcode name
            instr.opname.ljust(_OPNAME_WIDTH),
            # Column: Opcode argument
            ' '.join('%2d' % x for x in instr.opargs),
            # Column: Opcode argument details
            '(' + '  '.join(instr.opargs_repr) + ')'
        ])
        print(dis_str, file=file)

disco = disassemble                     # XXX For backwards compatibility

def _unpack_opargs(code):
    extended_args = (0, 0, 0)
    for i in range(0, len(code), 4):
        opcode = code[i]
        opargs = tuple(a | b for a, b in zip(code[i + 1: i + 4], extended_args))
        extended_args = (a << 8 for a in opargs) if opcode == EXTENDED_ARG else (0, 0, 0)
        yield (i, opcode, opargs)

def findlabels(code):
    """Detect all offsets in a byte code which are jump targets.

    Return the list of offsets.

    """
    labels = set()
    for offset, opcode, opargs in _unpack_opargs(code):
        oparg_kind = opcode_mod.oparg_kind_table[opcode]
        if oparg_kind[1] == opcode_mod.OPARG_KIND_NEG_ADDR:
            rel_addr = opargs[2] - opargs[1]
        elif oparg_kind[2] == opcode_mod.OPARG_KIND_ADDR:
            rel_addr = opargs[2]
        else:
            continue
        labels.add(offset + (1 + rel_addr) * 4)
    return labels

def findlinestarts(code):
    """Find the offsets in a byte code which are start of lines in the source.

    Generate pairs (offset, lineno)
    """
    lastline = None
    for start, end, line in code.co_lines():
        if line is not None and line != lastline:
            lastline = line
            yield start, line
    return


class Bytecode:
    """The bytecode operations of a piece of code

    Instantiate this with a function, method, other compiled object, string of
    code, or a code object (as returned by compile()).

    Iterating over this yields the bytecode operations as Instruction instances.
    """
    def __init__(self, x, *, first_line=None, current_offset=None):
        self.codeobj = co = _get_code_object(x)
        if first_line is None:
            self.first_line = co.co_firstlineno
            self._line_offset = 0
        else:
            self.first_line = first_line
            self._line_offset = first_line - co.co_firstlineno
        self._linestarts = dict(findlinestarts(co))
        self._original_object = x
        self.current_offset = current_offset

    def __iter__(self):
        return _get_instructions_bytes(self.codeobj, None,
                                       self._linestarts,
                                       self._line_offset)

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__,
                                 self._original_object)

    @classmethod
    def from_traceback(cls, tb):
        """ Construct a Bytecode from the given traceback """
        while tb.tb_next:
            tb = tb.tb_next
        return cls(tb.tb_frame.f_code, current_offset=tb.tb_lasti)

    def info(self):
        """Return formatted information about the code object."""
        return _format_code_info(self.codeobj)

    def dis(self):
        """Return a formatted view of the bytecode operations."""
        if self.current_offset is not None:
            offset = self.current_offset
        else:
            offset = -1
        with io.StringIO() as output:
            _disassemble_bytes(self.codeobj, None,
                               offset,
                               linestarts=self._linestarts,
                               line_offset=self._line_offset,
                               file=output)
            return output.getvalue()


def _test():
    """Simple test program to disassemble a file."""
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('infile', type=argparse.FileType('rb'), nargs='?', default='-')
    args = parser.parse_args()
    with args.infile as infile:
        source = infile.read()
    code = compile(source, args.infile.name, "exec")
    dis(code)

if __name__ == "__main__":
    _test()
