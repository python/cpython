"""bytecode_helper - support tools for testing correct bytecode generation"""

import unittest
import dis
import io

_UNSPECIFIED = object()

class BytecodeTestCase(unittest.TestCase):
    """Custom assertion methods for inspecting bytecode."""

    def get_disassembly_as_string(self, co):
        s = io.StringIO()
        dis.dis(co, file=s)
        return s.getvalue()

    def assertInstructionMatches(self, instr, expected, *, line_offset=0):
        # Deliberately test opname first, since that gives a more
        # meaningful error message than testing opcode
        self.assertEqual(instr.opname, expected.opname)
        self.assertEqual(instr.opcode, expected.opcode)
        self.assertEqual(instr.arg, expected.arg)
        self.assertEqual(instr.argval, expected.argval)
        self.assertEqual(instr.argrepr, expected.argrepr)
        self.assertEqual(instr.offset, expected.offset)
        if expected.starts_line is None:
            self.assertIsNone(instr.starts_line)
        else:
            self.assertEqual(instr.starts_line,
                                expected.starts_line + line_offset)
        self.assertEqual(instr.is_jump_target, expected.is_jump_target)


    def assertBytecodeExactlyMatches(self, x, expected, *, line_offset=0):
        """Throws AssertionError if any discrepancy is found in bytecode

        *x* is the object to be introspected
        *expected* is a list of dis.Instruction objects

        Set *line_offset* as appropriate to adjust for the location of the
        object to be disassembled within the test file. If the expected list
        assumes the first line is line 1, then an appropriate offset would be
        ``1 - f.__code__.co_firstlineno``.
        """
        actual = dis.get_instructions(x, line_offset=line_offset)
        self.assertEqual(list(actual), expected)

    def assertInBytecode(self, x, opname, argval=_UNSPECIFIED):
        """Returns instr if op is found, otherwise throws AssertionError"""
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
        """Throws AssertionError if op is found"""
        for instr in dis.get_instructions(x):
            if instr.opname == opname:
                disassembly = self.get_disassembly_as_string(co)
                if opargval is _UNSPECIFIED:
                    msg = '%s occurs in bytecode:\n%s' % (opname, disassembly)
                elif instr.argval == argval:
                    msg = '(%s,%r) occurs in bytecode:\n%s'
                    msg = msg % (opname, argval, disassembly)
                self.fail(msg)
