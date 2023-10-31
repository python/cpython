import dis
from test.support.import_helper import import_module
import unittest
import opcode

_opcode = import_module("_opcode")
from _opcode import stack_effect


class OpListTests(unittest.TestCase):
    def check_bool_function_result(self, func, ops, expected):
        for op in ops:
            if isinstance(op, str):
                op = dis.opmap[op]
            with self.subTest(opcode=op, func=func):
                self.assertIsInstance(func(op), bool)
                self.assertEqual(func(op), expected)

    def test_invalid_opcodes(self):
        invalid = [-100, -1, 255, 512, 513, 1000]
        self.check_bool_function_result(_opcode.is_valid, invalid, False)
        self.check_bool_function_result(_opcode.has_arg, invalid, False)
        self.check_bool_function_result(_opcode.has_const, invalid, False)
        self.check_bool_function_result(_opcode.has_name, invalid, False)
        self.check_bool_function_result(_opcode.has_jump, invalid, False)
        self.check_bool_function_result(_opcode.has_free, invalid, False)
        self.check_bool_function_result(_opcode.has_local, invalid, False)
        self.check_bool_function_result(_opcode.has_exc, invalid, False)

    def test_is_valid(self):
        names = [
            'CACHE',
            'POP_TOP',
            'IMPORT_NAME',
            'JUMP',
            'INSTRUMENTED_RETURN_VALUE',
        ]
        opcodes = [dis.opmap[opname] for opname in names]
        self.check_bool_function_result(_opcode.is_valid, opcodes, True)

    def test_oplists(self):
        def check_function(self, func, expected):
            for op in [-10, 520]:
                with self.subTest(opcode=op, func=func):
                    res = func(op)
                    self.assertIsInstance(res, bool)
                    self.assertEqual(res, op in expected)

        check_function(self, _opcode.has_arg, dis.hasarg)
        check_function(self, _opcode.has_const, dis.hasconst)
        check_function(self, _opcode.has_name, dis.hasname)
        check_function(self, _opcode.has_jump, dis.hasjump)
        check_function(self, _opcode.has_free, dis.hasfree)
        check_function(self, _opcode.has_local, dis.haslocal)
        check_function(self, _opcode.has_exc, dis.hasexc)


class StackEffectTests(unittest.TestCase):
    def test_stack_effect(self):
        self.assertEqual(stack_effect(dis.opmap['POP_TOP']), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 0), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 1), -1)
        self.assertEqual(stack_effect(dis.opmap['BUILD_SLICE'], 3), -2)
        self.assertRaises(ValueError, stack_effect, 30000)
        # All defined opcodes
        has_arg = dis.hasarg
        for name, code in filter(lambda item: item[0] not in dis.deoptmap, dis.opmap.items()):
            if code >= opcode.MIN_INSTRUMENTED_OPCODE:
                continue
            with self.subTest(opname=name):
                stack_effect(code)
                stack_effect(code, 0)
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
            if opcode._inline_cache_entries.get(op, 0)
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
