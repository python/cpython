"""bytecode_helper - support tools for testing correct bytecode generation"""

import unittest
import dis
import io
import opcode
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

_UNSPECIFIED = object()

def instructions_with_positions(instrs, co_positions):
    # Return (instr, positions) pairs from the instrs list and co_positions
    # iterator. The latter contains items for cache lines and the former
    # doesn't, so those need to be skipped.

    co_positions = co_positions or iter(())
    for instr in instrs:
        yield instr, next(co_positions, ())
        for _, size, _ in (instr.cache_info or ()):
            for i in range(size):
                next(co_positions, ())

class BytecodeTestCase(unittest.TestCase):
    """Custom assertion methods for inspecting bytecode."""

    def get_disassembly_as_string(self, co):
        s = io.StringIO()
        dis.dis(co, file=s)
        return s.getvalue()

    def assertInBytecode(self, x, opname, argval=_UNSPECIFIED):
        """Returns instr if opname is found, otherwise throws AssertionError"""
        self.assertIn(opname, dis.opmap)
        for instr in dis.get_instructions(x):
            if instr.opname == opname:
                if argval is _UNSPECIFIED or instr.argval == argval:
                    return instr
        disassembly = self.get_disassembly_as_string(x)
        if argval is _UNSPECIFIED:
            msg = '%s not found in bytecode:\n%s' % (opname, disassembly)
        else:
            msg = '(%s,%r) not found in bytecode:\n%s'
            msg = msg % (opname, argval, disassembly)
        self.fail(msg)

    def assertNotInBytecode(self, x, opname, argval=_UNSPECIFIED):
        """Throws AssertionError if opname is found"""
        self.assertIn(opname, dis.opmap)
        for instr in dis.get_instructions(x):
            if instr.opname == opname:
                disassembly = self.get_disassembly_as_string(x)
                if argval is _UNSPECIFIED:
                    msg = '%s occurs in bytecode:\n%s' % (opname, disassembly)
                    self.fail(msg)
                elif instr.argval == argval:
                    msg = '(%s,%r) occurs in bytecode:\n%s'
                    msg = msg % (opname, argval, disassembly)
                    self.fail(msg)

class CompilationStepTestCase(unittest.TestCase):

    HAS_ARG = set(dis.hasarg)
    HAS_TARGET = set(dis.hasjrel + dis.hasjabs + dis.hasexc)
    HAS_ARG_OR_TARGET = HAS_ARG.union(HAS_TARGET)

    class Label:
        pass

    def assertInstructionsMatch(self, actual_seq, expected):
        # get an InstructionSequence and an expected list, where each
        # entry is a label or an instruction tuple. Construct an expected
        # instruction sequence and compare with the one given.

        self.assertIsInstance(expected, list)
        actual = actual_seq.get_instructions()
        expected = self.seq_from_insts(expected).get_instructions()
        self.assertEqual(len(actual), len(expected))

        # compare instructions
        for act, exp in zip(actual, expected):
            if isinstance(act, int):
                self.assertEqual(exp, act)
                continue
            self.assertIsInstance(exp, tuple)
            self.assertIsInstance(act, tuple)
            idx = max([p[0] for p in enumerate(exp) if p[1] != -1])
            self.assertEqual(exp[:idx], act[:idx])

    def resolveAndRemoveLabels(self, insts):
        idx = 0
        res = []
        for item in insts:
            assert isinstance(item, (self.Label, tuple))
            if isinstance(item, self.Label):
                item.value = idx
            else:
                idx += 1
                res.append(item)

        return res

    def seq_from_insts(self, insts):
        labels = {item for item in insts if isinstance(item, self.Label)}
        for i, lbl in enumerate(labels):
            lbl.value = i

        seq = _testinternalcapi.new_instruction_sequence()
        for item in insts:
            if isinstance(item, self.Label):
                seq.use_label(item.value)
            else:
                op = item[0]
                if isinstance(op, str):
                    op = opcode.opmap[op]
                arg, *loc = item[1:]
                if isinstance(arg, self.Label):
                    arg = arg.value
                loc = loc + [-1] * (4 - len(loc))
                seq.addop(op, arg or 0, *loc)
        return seq

    def check_instructions(self, insts):
        for inst in insts:
            if isinstance(inst, self.Label):
                continue
            op, arg, *loc = inst
            if isinstance(op, str):
                op = opcode.opmap[op]
            self.assertEqual(op in opcode.hasarg,
                             arg is not None,
                             f"{opcode.opname[op]=} {arg=}")
            self.assertTrue(all(isinstance(l, int) for l in loc))


@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class CodegenTestCase(CompilationStepTestCase):

    def generate_code(self, ast):
        insts, _ = _testinternalcapi.compiler_codegen(ast, "my_file.py", 0)
        return insts


@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class CfgOptimizationTestCase(CompilationStepTestCase):

    def get_optimized(self, seq, consts, nlocals=0):
        insts = _testinternalcapi.optimize_cfg(seq, consts, nlocals)
        return insts, consts

@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class AssemblerTestCase(CompilationStepTestCase):

    def get_code_object(self, filename, insts, metadata):
        co = _testinternalcapi.assemble_code_object(filename, insts, metadata)
        return co
