import dis
from test.support.import_helper import import_module
import unittest
import opcode

_opcode = import_module("_opcode")
from _opcode import stack_effect


class OpcodeTests(unittest.TestCase):

    def test_stack_effect(self):
        self.assertEqual(stack_effect(dis.opmap['POP_TOP']), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 0), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 1), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 3), -2)
        self.assertRaises(ValueError, stack_effect, 30000)
        self.assertRaises(ValueError, stack_effect, dis.opmap['BUILD_SLICE'])
        self.assertRaises(ValueError, stack_effect, dis.opmap['POP_TOP'], 0)
        # All defined opcodes
        has_arg = dis.hasarg
        for name, code in filter(lambda item: item[0] not in dis.deoptmap, dis.opmap.items()):
            if code >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(opname=name):
                if code not in has_arg:
                    stack_effect(code)
                    self.assertRaises(ValueError, stack_effect, code, 0)
                else:
                    stack_effect(code, 0)
                    self.assertRaises(ValueError, stack_effect, code)
        # All not defined opcodes
        for code in set(range(256)) - set(dis.opmap.values()):
            with self.subTest(opcode=code):
                self.assertRaises(ValueError, stack_effect, code)
                self.assertRaises(ValueError, stack_effect, code, 0)

    def test_stack_effect_jump(self):
        FOR_ITER = dis.opmap['FOR_ITER']
        self.assertEqual(stack_effect(FOR_ITER, 0), 1)
        self.assertEqual(stack_effect(FOR_ITER, 0, jump=True), 1)
        self.assertEqual(stack_effect(FOR_ITER, 0, jump=False), 1)
        JUMP_FORWARD = dis.opmap['JUMP_FORWARD']
        self.assertEqual(stack_effect(JUMP_FORWARD, 0), 0)
        self.assertEqual(stack_effect(JUMP_FORWARD, 0, jump=True), 0)
        self.assertEqual(stack_effect(JUMP_FORWARD, 0, jump=False), 0)
        # All defined opcodes
        has_arg = dis.hasarg
        has_exc = dis.hasexc
        has_jump = dis.hasjabs + dis.hasjrel
        for name, code in filter(lambda item: item[0] not in dis.deoptmap, dis.opmap.items()):
            if code >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(opname=name):
                if code not in has_arg:
                    common = stack_effect(code)
                    jump = stack_effect(code, jump=True)
                    nojump = stack_effect(code, jump=False)
                else:
                    common = stack_effect(code, 0)
                    jump = stack_effect(code, 0, jump=True)
                    nojump = stack_effect(code, 0, jump=False)
                if code in has_jump or code in has_exc:
                    self.assertEqual(common, max(jump, nojump))
                else:
                    self.assertEqual(jump, common)
                    self.assertEqual(nojump, common)


class SpecializationStatsTests(unittest.TestCase):
    def test_specialization_stats(self):
        stat_names = ["success", "failure", "hit", "deferred", "miss", "deopt"]
        specialized_opcodes = [
            op.lower()
            for op in opcode._specializations
            if opcode._inline_cache_entries[opcode.opmap[op]]
        ]
        self.assertIn('load_attr', specialized_opcodes)
        self.assertIn('binary_subscr', specialized_opcodes)

        stats = _opcode.get_specialization_stats()
        if stats is not None:
            self.assertIsInstance(stats, dict)
            self.assertCountEqual(stats.keys(), specialized_opcodes)
            self.assertCountEqual(
                stats['load_attr'].keys(),
                stat_names + ['failure_kinds'])
            for sn in stat_names:
                self.assertIsInstance(stats['load_attr'][sn], int)
            self.assertIsInstance(
                stats['load_attr']['failure_kinds'],
                tuple)
            for v in stats['load_attr']['failure_kinds']:
                self.assertIsInstance(v, int)


if __name__ == "__main__":
    unittest.main()
