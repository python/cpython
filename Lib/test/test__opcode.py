import dis
from test.support.import_helper import import_module
import unittest
import opcode

_opcode = import_module("_opcode")
from _opcode import stack_effect


EXPECTED_OPLISTS = {
    'HAS_ARG': [
        'BINARY_OP', 'BUILD_CONST_KEY_MAP', 'BUILD_LIST', 'BUILD_MAP',
        'BUILD_SET', 'BUILD_SLICE', 'BUILD_STRING', 'BUILD_TUPLE', 'CALL',
        'CALL_FUNCTION_EX', 'CALL_INTRINSIC_1', 'CALL_INTRINSIC_2', 'COMPARE_OP',
        'CONTAINS_OP', 'CONVERT_VALUE', 'COPY', 'COPY_FREE_VARS', 'DELETE_ATTR',
        'DELETE_DEREF', 'DELETE_FAST', 'DELETE_GLOBAL', 'DELETE_NAME',
        'DICT_MERGE', 'DICT_UPDATE', 'ENTER_EXECUTOR', 'EXTENDED_ARG',
        'FOR_ITER', 'GET_AWAITABLE', 'IMPORT_FROM', 'IMPORT_NAME',
        'INSTRUMENTED_CALL', 'INSTRUMENTED_FOR_ITER',
        'INSTRUMENTED_JUMP_BACKWARD', 'INSTRUMENTED_JUMP_FORWARD',
        'INSTRUMENTED_LOAD_SUPER_ATTR', 'INSTRUMENTED_POP_JUMP_IF_FALSE',
        'INSTRUMENTED_POP_JUMP_IF_NONE', 'INSTRUMENTED_POP_JUMP_IF_NOT_NONE',
        'INSTRUMENTED_POP_JUMP_IF_TRUE', 'INSTRUMENTED_RESUME',
        'INSTRUMENTED_RETURN_CONST', 'INSTRUMENTED_YIELD_VALUE', 'IS_OP', 'JUMP',
        'JUMP_BACKWARD', 'JUMP_BACKWARD_NO_INTERRUPT', 'JUMP_FORWARD',
        'JUMP_NO_INTERRUPT', 'KW_NAMES', 'LIST_APPEND', 'LIST_EXTEND',
        'LOAD_ATTR', 'LOAD_CLOSURE', 'LOAD_CONST', 'LOAD_DEREF', 'LOAD_FAST',
        'LOAD_FAST_AND_CLEAR', 'LOAD_FAST_CHECK', 'LOAD_FAST_LOAD_FAST',
        'LOAD_FROM_DICT_OR_DEREF', 'LOAD_FROM_DICT_OR_GLOBALS', 'LOAD_GLOBAL',
        'LOAD_METHOD', 'LOAD_NAME', 'LOAD_SUPER_ATTR', 'LOAD_SUPER_METHOD',
        'LOAD_ZERO_SUPER_ATTR', 'LOAD_ZERO_SUPER_METHOD', 'MAKE_CELL', 'MAP_ADD',
        'MATCH_CLASS', 'POP_JUMP_IF_FALSE', 'POP_JUMP_IF_NONE',
        'POP_JUMP_IF_NOT_NONE', 'POP_JUMP_IF_TRUE', 'RAISE_VARARGS', 'RERAISE',
        'RESUME', 'RETURN_CONST', 'SEND', 'SET_ADD', 'SET_FUNCTION_ATTRIBUTE',
        'SET_UPDATE', 'STORE_ATTR', 'STORE_DEREF', 'STORE_FAST',
        'STORE_FAST_LOAD_FAST', 'STORE_FAST_MAYBE_NULL', 'STORE_FAST_STORE_FAST',
        'STORE_GLOBAL', 'STORE_NAME', 'SWAP', 'UNPACK_EX', 'UNPACK_SEQUENCE',
        'YIELD_VALUE'],

    'HAS_CONST': [
        'LOAD_CONST', 'RETURN_CONST', 'KW_NAMES', 'INSTRUMENTED_RETURN_CONST'],

    'HAS_NAME': [
        'STORE_NAME', 'DELETE_NAME', 'STORE_ATTR', 'DELETE_ATTR', 'STORE_GLOBAL',
        'DELETE_GLOBAL', 'LOAD_NAME', 'LOAD_ATTR', 'IMPORT_NAME', 'IMPORT_FROM',
        'LOAD_GLOBAL', 'LOAD_SUPER_ATTR', 'LOAD_FROM_DICT_OR_GLOBALS',
        'LOAD_METHOD', 'LOAD_SUPER_METHOD', 'LOAD_ZERO_SUPER_METHOD',
        'LOAD_ZERO_SUPER_ATTR'],

    'HAS_CONST': [
        'LOAD_CONST', 'RETURN_CONST', 'KW_NAMES', 'INSTRUMENTED_RETURN_CONST'],

    'HAS_JUMP': [
        'FOR_ITER', 'JUMP_FORWARD', 'POP_JUMP_IF_FALSE', 'POP_JUMP_IF_TRUE',
        'SEND', 'POP_JUMP_IF_NOT_NONE', 'POP_JUMP_IF_NONE',
        'JUMP_BACKWARD_NO_INTERRUPT', 'JUMP_BACKWARD', 'ENTER_EXECUTOR', 'JUMP',
        'JUMP_NO_INTERRUPT'],

    'HAS_FREE': [
        'MAKE_CELL', 'LOAD_DEREF', 'STORE_DEREF', 'DELETE_DEREF',
        'LOAD_FROM_DICT_OR_DEREF'],

    'HAS_LOCAL': [
        'LOAD_FAST', 'STORE_FAST', 'DELETE_FAST', 'LOAD_FAST_CHECK',
        'LOAD_FAST_AND_CLEAR', 'LOAD_FAST_LOAD_FAST', 'STORE_FAST_LOAD_FAST',
        'STORE_FAST_STORE_FAST', 'STORE_FAST_MAYBE_NULL', 'LOAD_CLOSURE'],

    'HAS_EXC': ['SETUP_FINALLY', 'SETUP_CLEANUP', 'SETUP_WITH'],

    'HAS_COMPARE': ['COMPARE_OP'],
}

class OpListTests(unittest.TestCase):
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

        check_function(self, _opcode.has_arg, EXPECTED_OPLISTS['HAS_ARG'])
        check_function(self, _opcode.has_const, EXPECTED_OPLISTS['HAS_CONST'])
        check_function(self, _opcode.has_name, EXPECTED_OPLISTS['HAS_NAME'])
        check_function(self, _opcode.has_jump, EXPECTED_OPLISTS['HAS_JUMP'])
        check_function(self, _opcode.has_free, EXPECTED_OPLISTS['HAS_FREE'])
        check_function(self, _opcode.has_local, EXPECTED_OPLISTS['HAS_LOCAL'])
        check_function(self, _opcode.has_exc, EXPECTED_OPLISTS['HAS_EXC'])


class OpListTests(unittest.TestCase):
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
