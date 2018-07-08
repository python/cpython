import dis
from test.support import import_module
import unittest

_opcode = import_module("_opcode")

class OpcodeTests(unittest.TestCase):

    def test_stack_effect(self):
        self.assertEqual(_opcode.stack_effect(dis.opmap['POP_TOP']), -1)
        self.assertEqual(_opcode.stack_effect(dis.opmap['DUP_TOP_TWO']), 2)
        self.assertEqual(_opcode.stack_effect(dis.opmap['BUILD_SLICE'], 0), -1)
        self.assertEqual(_opcode.stack_effect(dis.opmap['BUILD_SLICE'], 1), -1)
        self.assertEqual(_opcode.stack_effect(dis.opmap['BUILD_SLICE'], 3), -2)
        self.assertRaises(ValueError, _opcode.stack_effect, 30000)
        self.assertRaises(ValueError, _opcode.stack_effect, dis.opmap['BUILD_SLICE'])
        self.assertRaises(ValueError, _opcode.stack_effect, dis.opmap['POP_TOP'], 0)
        # All defined opcodes
        for name, code in dis.opmap.items():
            with self.subTest(opname=name):
                if code < dis.HAVE_ARGUMENT:
                    _opcode.stack_effect(code)
                    self.assertRaises(ValueError, _opcode.stack_effect, code, 0)
                else:
                    _opcode.stack_effect(code, 0)
                    self.assertRaises(ValueError, _opcode.stack_effect, code)
        # All not defined opcodes
        for code in set(range(256)) - set(dis.opmap.values()):
            with self.subTest(opcode=code):
                self.assertRaises(ValueError, _opcode.stack_effect, code)
                self.assertRaises(ValueError, _opcode.stack_effect, code, 0)


if __name__ == "__main__":
    unittest.main()
