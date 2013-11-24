import dis
from test.support import run_unittest, import_module
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

def test_main():
    run_unittest(OpcodeTests)

if __name__ == "__main__":
    test_main()
