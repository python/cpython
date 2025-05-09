import contextlib
import os
import re
import sys
import tempfile
import unittest

from io import StringIO
from test import support
from test import test_tools


def skip_if_different_mount_drives():
    if sys.platform != "win32":
        return
    ROOT = os.path.dirname(os.path.dirname(__file__))
    root_drive = os.path.splitroot(ROOT)[0]
    cwd_drive = os.path.splitroot(os.getcwd())[0]
    if root_drive != cwd_drive:
        # May raise ValueError if ROOT and the current working
        # different have different mount drives (on Windows).
        raise unittest.SkipTest(
            f"the current working directory and the Python source code "
            f"directory have different mount drives "
            f"({cwd_drive} and {root_drive})"
        )


skip_if_different_mount_drives()


test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    from analyzer import analyze_forest, StackItem
    from cwriter import CWriter
    import parser
    from stack import Local, Stack
    import tier1_generator
    import opcode_metadata_generator
    import optimizer_generator


def handle_stderr():
    if support.verbose > 1:
        return contextlib.nullcontext()
    else:
        return support.captured_stderr()


def parse_src(src):
    p = parser.Parser(src, "test.c")
    nodes = []
    while node := p.definition():
        nodes.append(node)
    return nodes


class TestEffects(unittest.TestCase):
    def test_effect_sizes(self):
        stack = Stack()
        inputs = [
            x := StackItem("x", None, "1"),
            y := StackItem("y", None, "oparg"),
            z := StackItem("z", None, "oparg*2"),
        ]
        outputs = [
            StackItem("x", None, "1"),
            StackItem("b", None, "oparg*4"),
            StackItem("c", None, "1"),
        ]
        null = CWriter.null()
        stack.pop(z, null)
        stack.pop(y, null)
        stack.pop(x, null)
        for out in outputs:
            stack.push(Local.undefined(out))
        self.assertEqual(stack.base_offset.to_c(), "-1 - oparg - oparg*2")
        self.assertEqual(stack.physical_sp.to_c(), "0")
        self.assertEqual(stack.logical_sp.to_c(), "1 - oparg - oparg*2 + oparg*4")


class TestGeneratedCases(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.maxDiff = None

        self.temp_dir = tempfile.gettempdir()
        self.temp_input_filename = os.path.join(self.temp_dir, "input.txt")
        self.temp_output_filename = os.path.join(self.temp_dir, "output.txt")
        self.temp_metadata_filename = os.path.join(self.temp_dir, "metadata.txt")
        self.temp_pymetadata_filename = os.path.join(self.temp_dir, "pymetadata.txt")
        self.temp_executor_filename = os.path.join(self.temp_dir, "executor.txt")

    def tearDown(self) -> None:
        for filename in [
            self.temp_input_filename,
            self.temp_output_filename,
            self.temp_metadata_filename,
            self.temp_pymetadata_filename,
            self.temp_executor_filename,
        ]:
            try:
                os.remove(filename)
            except:
                pass
        super().tearDown()

    def run_cases_test(self, input: str, expected: str):
        with open(self.temp_input_filename, "w+") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
            temp_input.flush()

        with handle_stderr():
            tier1_generator.generate_tier1_from_files(
                [self.temp_input_filename], self.temp_output_filename, False
            )

        with open(self.temp_output_filename) as temp_output:
            lines = temp_output.read()
            _, rest = lines.split(tier1_generator.INSTRUCTION_START_MARKER)
            instructions, labels_with_prelude_and_postlude = rest.split(tier1_generator.INSTRUCTION_END_MARKER)
            _, labels_with_postlude = labels_with_prelude_and_postlude.split(tier1_generator.LABEL_START_MARKER)
            labels, _ = labels_with_postlude.split(tier1_generator.LABEL_END_MARKER)
            actual = instructions.strip() + "\n\n        " + labels.strip()

        self.assertEqual(actual.strip(), expected.strip())

    def test_inst_no_args(self):
        input = """
        inst(OP, (--)) {
            SPAM();
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            SPAM();
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_pop(self):
        input = """
        inst(OP, (value --)) {
            SPAM(value);
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            value = stack_pointer[-1];
            SPAM(value);
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push(self):
        input = """
        inst(OP, (-- res)) {
            res = SPAM();
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef res;
            res = SPAM();
            stack_pointer[0] = res;
            stack_pointer += 1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push_one_pop(self):
        input = """
        inst(OP, (value -- res)) {
            res = SPAM(value);
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            _PyStackRef res;
            value = stack_pointer[-1];
            res = SPAM(value);
            stack_pointer[-1] = res;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_binary_op(self):
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(left, right);
            INPUTS_DEAD();

        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            res = SPAM(left, right);
            stack_pointer[-2] = res;
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_overlap(self):
        input = """
        inst(OP, (left, right -- left, result)) {
            result = SPAM(left, right);
            INPUTS_DEAD();
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef result;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            result = SPAM(left, right);
            stack_pointer[-1] = result;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_predictions(self):
        input = """
        inst(OP1, (arg -- res)) {
            DEAD(arg);
            res = Py_None;
        }
        inst(OP3, (arg -- res)) {
            DEAD(arg);
            DEOPT_IF(xxx);
            res = Py_None;
        }
        family(OP1, INLINE_CACHE_ENTRIES_OP1) = { OP3 };
    """
        output = """
        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            PREDICTED_OP1:;
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            res = Py_None;
            stack_pointer[-1] = res;
            DISPATCH();
        }

        TARGET(OP3) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP3;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP1 == 0, "incorrect cache size");
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            if (xxx) {
                UPDATE_MISS_STATS(OP1);
                assert(_PyOpcode_Deopt[opcode] == (OP1));
                JUMP_TO_PREDICTED(OP1);
            }
            res = Py_None;
            stack_pointer[-1] = res;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_sync_sp(self):
        input = """
        inst(A, (arg -- res)) {
            DEAD(arg);
            SYNC_SP();
            escaping_call();
            res = Py_None;
        }
        inst(B, (arg -- res)) {
            DEAD(arg);
            res = Py_None;
            SYNC_SP();
            escaping_call();
        }
    """
        output = """
        TARGET(A) {
            #if Py_TAIL_CALL_INTERP
            int opcode = A;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(A);
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            escaping_call();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            res = Py_None;
            stack_pointer[0] = res;
            stack_pointer += 1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }

        TARGET(B) {
            #if Py_TAIL_CALL_INTERP
            int opcode = B;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(B);
            _PyStackRef arg;
            _PyStackRef res;
            arg = stack_pointer[-1];
            res = Py_None;
            stack_pointer[-1] = res;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            escaping_call();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)


    def test_pep7_condition(self):
        input = """
        inst(OP, (arg1 -- out)) {
            if (arg1)
                out = 0;
            else {
                out = 1;
            }
        }
        """
        output = ""
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_error_if_plain(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) {
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_plain_with_comment(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond);  // Comment is ok
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) {
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_pop(self):
        input = """
        inst(OP, (left, right -- res)) {
            SPAM(left, right);
            INPUTS_DEAD();
            ERROR_IF(cond);
            res = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            SPAM(left, right);
            if (cond) {
                JUMP_TO_LABEL(pop_2_error);
            }
            res = 0;
            stack_pointer[-2] = res;
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_pop_with_result(self):
        input = """
        inst(OP, (left, right -- res)) {
            res = SPAM(left, right);
            INPUTS_DEAD();
            ERROR_IF(cond);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            res = SPAM(left, right);
            if (cond) {
                JUMP_TO_LABEL(pop_2_error);
            }
            stack_pointer[-2] = res;
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_cache_effect(self):
        input = """
        inst(OP, (counter/1, extra/2, value --)) {
            DEAD(value);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 4;
            INSTRUCTION_STATS(OP);
            _PyStackRef value;
            value = stack_pointer[-1];
            uint16_t counter = read_u16(&this_instr[1].cache);
            (void)counter;
            uint32_t extra = read_u32(&this_instr[2].cache);
            (void)extra;
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_suppress_dispatch(self):
        input = """
        label(somewhere) {
        }

        inst(OP, (--)) {
            goto somewhere;
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            JUMP_TO_LABEL(somewhere);
        }

        LABEL(somewhere)
        {
        }
    """
        self.run_cases_test(input, output)

    def test_macro_instruction(self):
        input = """
        inst(OP1, (counter/1, left, right -- left, right)) {
            op1(left, right);
        }
        op(OP2, (extra/2, arg2, left, right -- res)) {
            res = op2(arg2, left, right);
            INPUTS_DEAD();
        }
        macro(OP) = OP1 + cache/2 + OP2;
        inst(OP3, (unused/5, arg2, left, right -- res)) {
            res = op3(arg2, left, right);
            INPUTS_DEAD();
        }
        family(OP, INLINE_CACHE_ENTRIES_OP) = { OP3 };
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP);
            PREDICTED_OP:;
            _Py_CODEUNIT* const this_instr = next_instr - 6;
            (void)this_instr;
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef arg2;
            _PyStackRef res;
            // _OP1
            {
                right = stack_pointer[-1];
                left = stack_pointer[-2];
                uint16_t counter = read_u16(&this_instr[1].cache);
                (void)counter;
                _PyFrame_SetStackPointer(frame, stack_pointer);
                op1(left, right);
                stack_pointer = _PyFrame_GetStackPointer(frame);
            }
            /* Skip 2 cache entries */
            // OP2
            {
                arg2 = stack_pointer[-3];
                uint32_t extra = read_u32(&this_instr[4].cache);
                (void)extra;
                _PyFrame_SetStackPointer(frame, stack_pointer);
                res = op2(arg2, left, right);
                stack_pointer = _PyFrame_GetStackPointer(frame);
            }
            stack_pointer[-3] = res;
            stack_pointer += -2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }

        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 2;
            INSTRUCTION_STATS(OP1);
            _PyStackRef left;
            _PyStackRef right;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            uint16_t counter = read_u16(&this_instr[1].cache);
            (void)counter;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            op1(left, right);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }

        TARGET(OP3) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP3;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP == 5, "incorrect cache size");
            _PyStackRef arg2;
            _PyStackRef left;
            _PyStackRef right;
            _PyStackRef res;
            /* Skip 5 cache entries */
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            arg2 = stack_pointer[-3];
            _PyFrame_SetStackPointer(frame, stack_pointer);
            res = op3(arg2, left, right);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            stack_pointer[-3] = res;
            stack_pointer += -2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_unused_caches(self):
        input = """
        inst(OP, (unused/1, unused/2 --)) {
            body;
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 4;
            INSTRUCTION_STATS(OP);
            /* Skip 1 cache entry */
            /* Skip 2 cache entries */
            body;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_no_flags(self):
        input = """
        pseudo(OP, (in -- out1, out2)) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_with_flags(self):
        input = """
        pseudo(OP, (in1, in2 --), (HAS_ARG, HAS_JUMP)) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_as_sequence(self):
        input = """
        pseudo(OP, (in -- out1, out2)) = [
            OP1, OP2
        ];

        inst(OP1, (--)) {
        }

        inst(OP2, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }

        TARGET(OP2) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP2;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP2);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)


    def test_array_input(self):
        input = """
        inst(OP, (below, values[oparg*2], above --)) {
            SPAM(values, oparg);
            DEAD(below);
            DEAD(values);
            DEAD(above);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef below;
            _PyStackRef *values;
            _PyStackRef above;
            above = stack_pointer[-1];
            values = &stack_pointer[-1 - oparg*2];
            below = stack_pointer[-2 - oparg*2];
            SPAM(values, oparg);
            stack_pointer += -2 - oparg*2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_output(self):
        input = """
        inst(OP, (unused, unused -- below, values[oparg*3], above)) {
            SPAM(values, oparg);
            below = 0;
            above = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef below;
            _PyStackRef *values;
            _PyStackRef above;
            values = &stack_pointer[-1];
            SPAM(values, oparg);
            below = 0;
            above = 0;
            stack_pointer[-2] = below;
            stack_pointer[-1 + oparg*3] = above;
            stack_pointer += oparg*3;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_input_output(self):
        input = """
        inst(OP, (values[oparg] -- values[oparg], above)) {
            SPAM(values, oparg);
            above = 0;
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef *values;
            _PyStackRef above;
            values = &stack_pointer[-oparg];
            SPAM(values, oparg);
            above = 0;
            stack_pointer[0] = above;
            stack_pointer += 1;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_error_if(self):
        input = """
        inst(OP, (extra, values[oparg] --)) {
            DEAD(extra);
            DEAD(values);
            ERROR_IF(oparg == 0);
        }
    """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef extra;
            _PyStackRef *values;
            values = &stack_pointer[-oparg];
            extra = stack_pointer[-1 - oparg];
            if (oparg == 0) {
                stack_pointer += -1 - oparg;
                assert(WITHIN_STACK_BOUNDS());
                JUMP_TO_LABEL(error);
            }
            stack_pointer += -1 - oparg;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_macro_push_push(self):
        input = """
        op(A, (-- val1)) {
            val1 = SPAM();
        }
        op(B, (-- val2)) {
            val2 = SPAM();
        }
        macro(M) = A + B;
        """
        output = """
        TARGET(M) {
            #if Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            _PyStackRef val1;
            _PyStackRef val2;
            // A
            {
                val1 = SPAM();
            }
            // B
            {
                val2 = SPAM();
            }
            stack_pointer[0] = val1;
            stack_pointer[1] = val2;
            stack_pointer += 2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_inst(self):
        input = """
        inst(OP, (--)) {
            spam;
        }
        override inst(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_op(self):
        input = """
        op(OP, (--)) {
            spam;
        }
        macro(M) = OP;
        override op(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(M) {
            #if Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_inst(self):
        input = """
        pure inst(OP, (--)) {
            ham;
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_op(self):
        input = """
        pure op(OP, (--)) {
            SPAM();
        }
        macro(M) = OP;
        """
        output = """
        TARGET(M) {
            #if Py_TAIL_CALL_INTERP
            int opcode = M;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            SPAM();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

        input = """
        pure register specializing op(OP, (--)) {
            SPAM();
        }
        macro(M) = OP;
        """
        self.run_cases_test(input, output)

    def test_deopt_and_exit(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            DEOPT_IF(1);
            EXIT_IF(1);
        }
        """
        output = ""
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_array_of_one(self):
        input = """
        inst(OP, (arg[1] -- out[1])) {
            out[0] = arg[0];
            DEAD(arg);
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef *arg;
            _PyStackRef *out;
            arg = &stack_pointer[-1];
            out = &stack_pointer[-1];
            out[0] = arg[0];
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_pointer_to_stackref(self):
        input = """
        inst(OP, (arg: _PyStackRef * -- out)) {
            out = *arg;
            DEAD(arg);
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef *arg;
            _PyStackRef out;
            arg = (_PyStackRef *)stack_pointer[-1].bits;
            out = *arg;
            stack_pointer[-1] = out;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_unused_cached_value(self):
        input = """
        op(FIRST, (arg1 -- out)) {
            out = arg1;
        }

        op(SECOND, (unused -- unused)) {
        }

        macro(BOTH) = FIRST + SECOND;
        """
        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_unused_named_values(self):
        input = """
        op(OP, (named -- named)) {
        }

        macro(INST) = OP;
        """
        output = """
        TARGET(INST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = INST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INST);
            DISPATCH();
        }

        """
        self.run_cases_test(input, output)

    def test_used_unused_used(self):
        input = """
        op(FIRST, (w -- w)) {
            USE(w);
        }

        op(SECOND, (x -- x)) {
        }

        op(THIRD, (y -- y)) {
            USE(y);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef w;
            _PyStackRef y;
            // FIRST
            {
                w = stack_pointer[-1];
                USE(w);
            }
            // SECOND
            {
            }
            // THIRD
            {
                y = w;
                USE(y);
            }
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_unused_used_used(self):
        input = """
        op(FIRST, (w -- w)) {
        }

        op(SECOND, (x -- x)) {
            USE(x);
        }

        op(THIRD, (y -- y)) {
            USE(y);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef x;
            _PyStackRef y;
            // FIRST
            {
            }
            // SECOND
            {
                x = stack_pointer[-1];
                USE(x);
            }
            // THIRD
            {
                y = x;
                USE(y);
            }
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_flush(self):
        input = """
        op(FIRST, ( -- a, b)) {
            a = 0;
            b = 1;
        }

        op(SECOND, (a, b -- )) {
            USE(a, b);
            INPUTS_DEAD();
        }

        macro(TEST) = FIRST + flush + SECOND;
        """
        output = """
        TARGET(TEST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                a = 0;
                b = 1;
            }
            // flush
            stack_pointer[0] = a;
            stack_pointer[1] = b;
            stack_pointer += 2;
            assert(WITHIN_STACK_BOUNDS());
            // SECOND
            {
                USE(a, b);
            }
            stack_pointer += -2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_pop_on_error_peeks(self):

        input = """
        op(FIRST, (x, y -- a, b)) {
            a = x;
            DEAD(x);
            b = y;
            DEAD(y);
        }

        op(SECOND, (a, b -- a, b)) {
        }

        op(THIRD, (j, k --)) {
            INPUTS_DEAD(); // Mark j and k as used
            ERROR_IF(cond);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef x;
            _PyStackRef y;
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                y = stack_pointer[-1];
                x = stack_pointer[-2];
                a = x;
                b = y;
            }
            // SECOND
            {
            }
            // THIRD
            {
                if (cond) {
                    JUMP_TO_LABEL(pop_2_error);
                }
            }
            stack_pointer += -2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_push_then_error(self):

        input = """
        op(FIRST, ( -- a)) {
            a = 1;
        }

        op(SECOND, (a -- a, b)) {
            b = 1;
            ERROR_IF(cond);
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        TARGET(TEST) {
            #if Py_TAIL_CALL_INTERP
            int opcode = TEST;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            _PyStackRef a;
            _PyStackRef b;
            // FIRST
            {
                a = 1;
            }
            // SECOND
            {
                b = 1;
                if (cond) {
                    stack_pointer[0] = a;
                    stack_pointer[1] = b;
                    stack_pointer += 2;
                    assert(WITHIN_STACK_BOUNDS());
                    JUMP_TO_LABEL(error);
                }
            }
            stack_pointer[0] = a;
            stack_pointer[1] = b;
            stack_pointer += 2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_error_if_true(self):

        input = """
        inst(OP1, ( --)) {
            ERROR_IF(true);
        }
        inst(OP2, ( --)) {
            ERROR_IF(1);
        }
        """
        output = """
        TARGET(OP1) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP1;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            JUMP_TO_LABEL(error);
        }

        TARGET(OP2) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP2;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP2);
            JUMP_TO_LABEL(error);
        }
        """
        self.run_cases_test(input, output)

    def test_scalar_array_inconsistency(self):

        input = """
        op(FIRST, ( -- a)) {
            a = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_array_size_inconsistency(self):

        input = """
        op(FIRST, ( -- a[2])) {
            a[0] = 1;
        }

        op(SECOND, (a[1] -- b)) {
            b = 1;
        }

        macro(TEST) = FIRST + SECOND;
        """

        output = """
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_stack_save_reload(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
            code();
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            code();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_stack_save_reload_paired(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_stack_reload_only(self):

        input = """
        inst(BALANCED, ( -- )) {
            RELOAD_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_stack_save_only(self):

        input = """
        inst(BALANCED, ( -- )) {
            SAVE_STACK();
        }
        """

        output = """
        TARGET(BALANCED) {
            #if Py_TAIL_CALL_INTERP
            int opcode = BALANCED;
            (void)(opcode);
            #endif
            _Py_CODEUNIT* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(BALANCED);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, output)

    def test_instruction_size_macro(self):
        input = """
        inst(OP, (--)) {
            frame->return_offset = INSTRUCTION_SIZE;
        }
        """

        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            frame->return_offset = 1 ;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

        # Two instructions of different sizes referencing the same
        # uop containing the `INSTRUCTION_SIZE` macro is not allowed.
        input = """
        inst(OP, (--)) {
            frame->return_offset = INSTRUCTION_SIZE;
        }
        macro(OP2) = unused/1 + OP;
        """

        output = ""  # No output needed as this should raise an error.
        with self.assertRaisesRegex(SyntaxError, "All instructions containing a uop"):
            self.run_cases_test(input, output)

    def test_escaping_call_next_to_cmacro(self):
        input = """
        inst(OP, (--)) {
            #ifdef Py_GIL_DISABLED
            escaping_call();
            #else
            another_escaping_call();
            #endif
            yet_another_escaping_call();
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            #ifdef Py_GIL_DISABLED
            _PyFrame_SetStackPointer(frame, stack_pointer);
            escaping_call();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            #else
            _PyFrame_SetStackPointer(frame, stack_pointer);
            another_escaping_call();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            #endif
            _PyFrame_SetStackPointer(frame, stack_pointer);
            yet_another_escaping_call();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_pystackref_frompyobject_new_next_to_cmacro(self):
        input = """
        inst(OP, (-- out1, out2)) {
            PyObject *obj = SPAM();
            #ifdef Py_GIL_DISABLED
            out1 = PyStackRef_FromPyObjectNew(obj);
            #else
            out1 = PyStackRef_FromPyObjectNew(obj);
            #endif
            out2 = PyStackRef_FromPyObjectNew(obj);
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef out1;
            _PyStackRef out2;
            PyObject *obj = SPAM();
            #ifdef Py_GIL_DISABLED
            out1 = PyStackRef_FromPyObjectNew(obj);
            #else
            out1 = PyStackRef_FromPyObjectNew(obj);
            #endif
            out2 = PyStackRef_FromPyObjectNew(obj);
            stack_pointer[0] = out1;
            stack_pointer[1] = out2;
            stack_pointer += 2;
            assert(WITHIN_STACK_BOUNDS());
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_no_escaping_calls_in_branching_macros(self):

        input = """
        inst(OP, ( -- )) {
            DEOPT_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

        input = """
        inst(OP, ( -- )) {
            EXIT_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

        input = """
        inst(OP, ( -- )) {
            ERROR_IF(escaping_call());
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

    def test_kill_in_wrong_order(self):
        input = """
        inst(OP, (a, b -- c)) {
            c = b;
            PyStackRef_CLOSE(a);
            PyStackRef_CLOSE(b);
        }
        """
        with self.assertRaises(SyntaxError):
            self.run_cases_test(input, "")

    def test_complex_label(self):
        input = """
        label(other_label) {
        }

        label(other_label2) {
        }

        label(my_label) {
            // Comment
            do_thing();
            if (complex) {
                goto other_label;
            }
            goto other_label2;
        }
        """

        output = """
        LABEL(other_label)
        {
        }

        LABEL(other_label2)
        {
        }

        LABEL(my_label)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            do_thing();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            if (complex) {
                JUMP_TO_LABEL(other_label);
            }
            JUMP_TO_LABEL(other_label2);
        }
        """
        self.run_cases_test(input, output)

    def test_spilled_label(self):
        input = """
        spilled label(one) {
            RELOAD_STACK();
            goto two;
        }

        label(two) {
            SAVE_STACK();
            goto one;
        }
        """

        output = """
        LABEL(one)
        {
            stack_pointer = _PyFrame_GetStackPointer(frame);
            JUMP_TO_LABEL(two);
        }

        LABEL(two)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMP_TO_LABEL(one);
        }
        """
        self.run_cases_test(input, output)


    def test_incorrect_spills(self):
        input1 = """
        spilled label(one) {
            goto two;
        }

        label(two) {
        }
        """

        input2 = """
        spilled label(one) {
        }

        label(two) {
            goto one;
        }
        """
        with self.assertRaisesRegex(SyntaxError, ".*reload.*"):
            self.run_cases_test(input1, "")
        with self.assertRaisesRegex(SyntaxError, ".*spill.*"):
            self.run_cases_test(input2, "")


    def test_multiple_labels(self):
        input = """
        label(my_label_1) {
            // Comment
            do_thing1();
            goto my_label_2;
        }

        label(my_label_2) {
            // Comment
            do_thing2();
            goto my_label_1;
        }
        """

        output = """
        LABEL(my_label_1)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            do_thing1();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            JUMP_TO_LABEL(my_label_2);
        }

        LABEL(my_label_2)
        {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            do_thing2();
            stack_pointer = _PyFrame_GetStackPointer(frame);
            JUMP_TO_LABEL(my_label_1);
        }
        """
        self.run_cases_test(input, output)

    def test_reassigning_live_inputs(self):
        input = """
        inst(OP, (in -- in)) {
            in = 0;
        }
        """

        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef in;
            in = stack_pointer[-1];
            in = 0;
            stack_pointer[-1] = in;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_reassigning_dead_inputs(self):
        input = """
        inst(OP, (in -- )) {
            temp = use(in);
            DEAD(in);
            in = temp;
            PyStackRef_CLOSE(in);
        }
        """
        output = """
        TARGET(OP) {
            #if Py_TAIL_CALL_INTERP
            int opcode = OP;
            (void)(opcode);
            #endif
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            _PyStackRef in;
            in = stack_pointer[-1];
            _PyFrame_SetStackPointer(frame, stack_pointer);
            temp = use(in);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            in = temp;
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            PyStackRef_CLOSE(in);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)


class TestGeneratedAbstractCases(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.maxDiff = None

        self.temp_dir = tempfile.gettempdir()
        self.temp_input_filename = os.path.join(self.temp_dir, "input.txt")
        self.temp_input2_filename = os.path.join(self.temp_dir, "input2.txt")
        self.temp_output_filename = os.path.join(self.temp_dir, "output.txt")

    def tearDown(self) -> None:
        for filename in [
            self.temp_input_filename,
            self.temp_input2_filename,
            self.temp_output_filename,
        ]:
            try:
                os.remove(filename)
            except:
                pass
        super().tearDown()

    def run_cases_test(self, input: str, input2: str, expected: str):
        with open(self.temp_input_filename, "w+") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input)
            temp_input.write(parser.END_MARKER)
            temp_input.flush()

        with open(self.temp_input2_filename, "w+") as temp_input:
            temp_input.write(parser.BEGIN_MARKER)
            temp_input.write(input2)
            temp_input.write(parser.END_MARKER)
            temp_input.flush()

        with handle_stderr():
            optimizer_generator.generate_tier2_abstract_from_files(
                [self.temp_input_filename, self.temp_input2_filename],
                self.temp_output_filename
            )

        with open(self.temp_output_filename) as temp_output:
            lines = temp_output.readlines()
            while lines and lines[0].startswith(("// ", "#", "    #", "\n")):
                lines.pop(0)
            while lines and lines[-1].startswith(("#", "\n")):
                lines.pop(-1)
        actual = "".join(lines)
        self.assertEqual(actual.strip(), expected.strip())

    def test_overridden_abstract(self):
        input = """
        pure op(OP, (--)) {
            SPAM();
        }
        """
        input2 = """
        pure op(OP, (--)) {
            eggs();
        }
        """
        output = """
        case OP: {
            eggs();
            break;
        }
        """
        self.run_cases_test(input, input2, output)

    def test_overridden_abstract_args(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            out = SPAM(arg1);
        }
        op(OP2, (arg1 -- out)) {
            out = EGGS(arg1);
        }
        """
        input2 = """
        op(OP, (arg1 -- out)) {
            out = EGGS(arg1);
        }
        """
        output = """
        case OP: {
            JitOptSymbol *arg1;
            JitOptSymbol *out;
            arg1 = stack_pointer[-1];
            out = EGGS(arg1);
            stack_pointer[-1] = out;
            break;
        }

        case OP2: {
            JitOptSymbol *out;
            out = sym_new_not_null(ctx);
            stack_pointer[-1] = out;
            break;
        }
       """
        self.run_cases_test(input, input2, output)

    def test_no_overridden_case(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            out = SPAM(arg1);
        }

        pure op(OP2, (arg1 -- out)) {
        }

        """
        input2 = """
        pure op(OP2, (arg1 -- out)) {
            out = NULL;
        }
        """
        output = """
        case OP: {
            JitOptSymbol *out;
            out = sym_new_not_null(ctx);
            stack_pointer[-1] = out;
            break;
        }

        case OP2: {
            JitOptSymbol *out;
            out = NULL;
            stack_pointer[-1] = out;
            break;
        }
        """
        self.run_cases_test(input, input2, output)

    def test_missing_override_failure(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            SPAM();
        }
        """
        input2 = """
        pure op(OTHER, (arg1 -- out)) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(AssertionError, "All abstract uops"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_input_length_mismatch(self):
        input = """
        op(OP, (arg1 -- out)) {
            SPAM();
        }
        """
        input2 = """
        op(OP, (arg1, arg2 -- out)) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Must have the same number of inputs"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_output_length_mismatch(self):
        input = """
        op(OP, (arg1 -- out)) {
            SPAM();
        }
        """
        input2 = """
        op(OP, (arg1 -- out1, out2)) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Must have the same number of outputs"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_input_name_mismatch(self):
        input = """
        op(OP, (foo -- out)) {
            SPAM();
        }
        """
        input2 = """
        op(OP, (bar -- out)) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Inputs must have equal names"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_output_name_mismatch(self):
        input = """
        op(OP, (arg1 -- foo)) {
            SPAM();
        }
        """
        input2 = """
        op(OP, (arg1 -- bar)) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Outputs must have equal names"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_unused_input(self):
        input = """
        op(OP, (unused -- )) {
        }
        """
        input2 = """
        op(OP, (foo -- )) {
        }
        """
        output = """
        case OP: {
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            break;
        }
        """
        self.run_cases_test(input, input2, output)

        input = """
        op(OP, (foo -- )) {
        }
        """
        input2 = """
        op(OP, (unused -- )) {
        }
        """
        output = """
        case OP: {
            stack_pointer += -1;
            assert(WITHIN_STACK_BOUNDS());
            break;
        }
        """
        self.run_cases_test(input, input2, output)

    def test_validate_uop_unused_output(self):
        input = """
        op(OP, ( -- unused)) {
        }
        """
        input2 = """
        op(OP, ( -- foo)) {
            foo = NULL;
        }
        """
        output = """
        case OP: {
            JitOptSymbol *foo;
            foo = NULL;
            stack_pointer[0] = foo;
            stack_pointer += 1;
            assert(WITHIN_STACK_BOUNDS());
            break;
        }
        """
        self.run_cases_test(input, input2, output)

        input = """
        op(OP, ( -- foo)) {
            foo = NULL;
        }
        """
        input2 = """
        op(OP, ( -- unused)) {
        }
        """
        output = """
        case OP: {
            stack_pointer += 1;
            assert(WITHIN_STACK_BOUNDS());
            break;
        }
        """
        self.run_cases_test(input, input2, output)

    def test_validate_uop_input_size_mismatch(self):
        input = """
        op(OP, (arg1[2] -- )) {
        }
        """
        input2 = """
        op(OP, (arg1[4] -- )) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Inputs must have equal sizes"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_output_size_mismatch(self):
        input = """
        op(OP, ( -- out[2])) {
        }
        """
        input2 = """
        op(OP, ( -- out[4])) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Outputs must have equal sizes"):
            self.run_cases_test(input, input2, output)

    def test_validate_uop_unused_size_mismatch(self):
        input = """
        op(OP, (foo[2] -- )) {
        }
        """
        input2 = """
        op(OP, (unused[4] -- )) {
        }
        """
        output = """
        """
        with self.assertRaisesRegex(SyntaxError,
                                    "Inputs must have equal sizes"):
            self.run_cases_test(input, input2, output)

if __name__ == "__main__":
    unittest.main()
