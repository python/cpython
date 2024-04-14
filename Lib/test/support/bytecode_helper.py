"""bytecode_helper - support tools for testing correct bytecode generation"""

import unittest
import dis
import io
from _testinternalcapi import compiler_codegen, optimize_cfg, assemble_code_object

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

    def assertInstructionsMatch(self, actual_, expected_):
        # get two lists where each entry is a label or
        # an instruction tuple. Normalize the labels to the
        # instruction count of the target, and compare the lists.

        self.assertIsInstance(actual_, list)
        self.assertIsInstance(expected_, list)

        actual = self.normalize_insts(actual_)
        expected = self.normalize_insts(expected_)
        self.assertEqual(len(actual), len(expected))

        # compare instructions
        for act, exp in zip(actual, expected):
            if isinstance(act, int):
                self.assertEqual(exp, act)
                continue
            self.assertIsInstance(exp, tuple)
            self.assertIsInstance(act, tuple)
            # crop comparison to the provided expected values
            if len(act) > len(exp):
                act = act[:len(exp)]
            self.assertEqual(exp, act)

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

    def normalize_insts(self, insts):
        """ Map labels to instruction index.
            Map opcodes to opnames.
        """
        insts = self.resolveAndRemoveLabels(insts)
        res = []
        for item in insts:
            assert isinstance(item, tuple)
            opcode, oparg, *loc = item
            opcode = dis.opmap.get(opcode, opcode)
            if isinstance(oparg, self.Label):
                arg = oparg.value
            else:
                arg = oparg if opcode in self.HAS_ARG else None
            opcode = dis.opname[opcode]
            res.append((opcode, arg, *loc))
        return res

    def complete_insts_info(self, insts):
        # fill in omitted fields in location, and oparg 0 for ops with no arg.
        res = []
        for item in insts:
            assert isinstance(item, tuple)
            inst = list(item)
            opcode = dis.opmap[inst[0]]
            oparg = inst[1]
            loc = inst[2:] + [-1] * (6 - len(inst))
            res.append((opcode, oparg, *loc))
        return res


class CodegenTestCase(CompilationStepTestCase):

    def generate_code(self, ast):
        insts, _ = compiler_codegen(ast, "my_file.py", 0)
        return insts


class CfgOptimizationTestCase(CompilationStepTestCase):

    def get_optimized(self, insts, consts, nlocals=0):
        insts = self.normalize_insts(insts)
        insts = self.complete_insts_info(insts)
        insts = optimize_cfg(insts, consts, nlocals)
        return insts, consts

class AssemblerTestCase(CompilationStepTestCase):

    def get_code_object(self, filename, insts, metadata):
        co = assemble_code_object(filename, insts, metadata)
        return co
