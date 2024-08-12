import contextlib
import os
import sys
import tempfile
import unittest

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
    from analyzer import StackItem
    import parser
    from stack import Stack
    import tier1_generator
    import optimizer_generator


def handle_stderr():
    if support.verbose > 1:
        return contextlib.nullcontext()
    else:
        return support.captured_stderr()


class TestEffects(unittest.TestCase):
    def test_effect_sizes(self):
        stack = Stack()
        inputs = [
            x := StackItem("x", None, "", "1"),
            y := StackItem("y", None, "", "oparg"),
            z := StackItem("z", None, "", "oparg*2"),
        ]
        outputs = [
            StackItem("x", None, "", "1"),
            StackItem("b", None, "", "oparg*4"),
            StackItem("c", None, "", "1"),
        ]
        stack.pop(z)
        stack.pop(y)
        stack.pop(x)
        for out in outputs:
            stack.push(out)
        self.assertEqual(stack.base_offset.to_c(), "-1 - oparg*2 - oparg")
        self.assertEqual(stack.top_offset.to_c(), "1 - oparg*2 - oparg + oparg*4")


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
            lines = temp_output.readlines()
            while lines and lines[0].startswith(("// ", "#", "    #", "\n")):
                lines.pop(0)
            while lines and lines[-1].startswith(("#", "\n")):
                lines.pop(-1)
        actual = "".join(lines)
        # if actual.strip() != expected.strip():
        #     print("Actual:")
        #     print(actual)
        #     print("Expected:")
        #     print(expected)
        #     print("End")

        self.assertEqual(actual.strip(), expected.strip())

    def test_inst_no_args(self):
        input = """
        inst(OP, (--)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            spam();
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_pop(self):
        input = """
        inst(OP, (value --)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *value;
            value = stack_pointer[-1];
            spam();
            stack_pointer += -1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push(self):
        input = """
        inst(OP, (-- res)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *res;
            spam();
            stack_pointer[0] = res;
            stack_pointer += 1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_inst_one_push_one_pop(self):
        input = """
        inst(OP, (value -- res)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *value;
            PyObject *res;
            value = stack_pointer[-1];
            spam();
            stack_pointer[-1] = res;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_binary_op(self):
        input = """
        inst(OP, (left, right -- res)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *right;
            PyObject *left;
            PyObject *res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            spam();
            stack_pointer[-2] = res;
            stack_pointer += -1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_overlap(self):
        input = """
        inst(OP, (left, right -- left, result)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *right;
            PyObject *left;
            PyObject *result;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            spam();
            stack_pointer[-1] = result;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_predictions_and_eval_breaker(self):
        input = """
        inst(OP1, (arg -- rest)) {
        }
        inst(OP3, (arg -- res)) {
            DEOPT_IF(xxx);
            CHECK_EVAL_BREAKER();
        }
        family(OP1, INLINE_CACHE_ENTRIES_OP1) = { OP3 };
    """
        output = """
        TARGET(OP1) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            PREDICTED(OP1);
            PyObject *arg;
            PyObject *rest;
            arg = stack_pointer[-1];
            stack_pointer[-1] = rest;
            DISPATCH();
        }

        TARGET(OP3) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP1 == 0, "incorrect cache size");
            PyObject *arg;
            PyObject *res;
            arg = stack_pointer[-1];
            DEOPT_IF(xxx, OP1);
            stack_pointer[-1] = res;
            CHECK_EVAL_BREAKER();
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_plain(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond, label);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) goto label;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_plain_with_comment(self):
        input = """
        inst(OP, (--)) {
            ERROR_IF(cond, label);  // Comment is ok
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            if (cond) goto label;
            // Comment is ok
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_error_if_pop(self):
        input = """
        inst(OP, (left, right -- res)) {
            ERROR_IF(cond, label);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *right;
            PyObject *left;
            PyObject *res;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            if (cond) goto pop_2_label;
            stack_pointer[-2] = res;
            stack_pointer += -1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_cache_effect(self):
        input = """
        inst(OP, (counter/1, extra/2, value --)) {
        }
    """
        output = """
        TARGET(OP) {
            _Py_CODEUNIT *this_instr = frame->instr_ptr = next_instr;
            (void)this_instr;
            next_instr += 4;
            INSTRUCTION_STATS(OP);
            PyObject *value;
            value = stack_pointer[-1];
            uint16_t counter = read_u16(&this_instr[1].cache);
            (void)counter;
            uint32_t extra = read_u32(&this_instr[2].cache);
            (void)extra;
            stack_pointer += -1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_suppress_dispatch(self):
        input = """
        inst(OP, (--)) {
            goto somewhere;
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            goto somewhere;
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
        }
        macro(OP) = OP1 + cache/2 + OP2;
        inst(OP3, (unused/5, arg2, left, right -- res)) {
            res = op3(arg2, left, right);
        }
        family(OP, INLINE_CACHE_ENTRIES_OP) = { OP3 };
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP);
            PREDICTED(OP);
            _Py_CODEUNIT *this_instr = next_instr - 6;
            (void)this_instr;
            PyObject *right;
            PyObject *left;
            PyObject *arg2;
            PyObject *res;
            // _OP1
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            {
                uint16_t counter = read_u16(&this_instr[1].cache);
                (void)counter;
                op1(left, right);
            }
            /* Skip 2 cache entries */
            // OP2
            arg2 = stack_pointer[-3];
            {
                uint32_t extra = read_u32(&this_instr[4].cache);
                (void)extra;
                res = op2(arg2, left, right);
            }
            stack_pointer[-3] = res;
            stack_pointer += -2;
            DISPATCH();
        }

        TARGET(OP1) {
            _Py_CODEUNIT *this_instr = frame->instr_ptr = next_instr;
            (void)this_instr;
            next_instr += 2;
            INSTRUCTION_STATS(OP1);
            PyObject *right;
            PyObject *left;
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            uint16_t counter = read_u16(&this_instr[1].cache);
            (void)counter;
            op1(left, right);
            DISPATCH();
        }

        TARGET(OP3) {
            frame->instr_ptr = next_instr;
            next_instr += 6;
            INSTRUCTION_STATS(OP3);
            static_assert(INLINE_CACHE_ENTRIES_OP == 5, "incorrect cache size");
            PyObject *right;
            PyObject *left;
            PyObject *arg2;
            PyObject *res;
            /* Skip 5 cache entries */
            right = stack_pointer[-1];
            left = stack_pointer[-2];
            arg2 = stack_pointer[-3];
            res = op3(arg2, left, right);
            stack_pointer[-3] = res;
            stack_pointer += -2;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_unused_caches(self):
        input = """
        inst(OP, (unused/1, unused/2 --)) {
            body();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 4;
            INSTRUCTION_STATS(OP);
            /* Skip 1 cache entry */
            /* Skip 2 cache entries */
            body();
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_no_flags(self):
        input = """
        pseudo(OP) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_pseudo_instruction_with_flags(self):
        input = """
        pseudo(OP, (HAS_ARG, HAS_JUMP)) = {
            OP1,
        };

        inst(OP1, (--)) {
        }
    """
        output = """
        TARGET(OP1) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP1);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_input(self):
        input = """
        inst(OP, (below, values[oparg*2], above --)) {
            spam();
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *above;
            PyObject **values;
            PyObject *below;
            above = stack_pointer[-1];
            values = &stack_pointer[-1 - oparg*2];
            below = stack_pointer[-2 - oparg*2];
            spam();
            stack_pointer += -2 - oparg*2;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_output(self):
        input = """
        inst(OP, (unused, unused -- below, values[oparg*3], above)) {
            spam(values, oparg);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *below;
            PyObject **values;
            PyObject *above;
            values = &stack_pointer[-1];
            spam(values, oparg);
            stack_pointer[-2] = below;
            stack_pointer[-1 + oparg*3] = above;
            stack_pointer += oparg*3;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_input_output(self):
        input = """
        inst(OP, (values[oparg] -- values[oparg], above)) {
            spam(values, oparg);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject **values;
            PyObject *above;
            values = &stack_pointer[-oparg];
            spam(values, oparg);
            stack_pointer[0] = above;
            stack_pointer += 1;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_array_error_if(self):
        input = """
        inst(OP, (extra, values[oparg] --)) {
            ERROR_IF(oparg == 0, somewhere);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject **values;
            PyObject *extra;
            values = &stack_pointer[-oparg];
            extra = stack_pointer[-1 - oparg];
            if (oparg == 0) { stack_pointer += -1 - oparg; goto somewhere; }
            stack_pointer += -1 - oparg;
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_cond_effect(self):
        input = """
        inst(OP, (aa, input if ((oparg & 1) == 1), cc -- xx, output if (oparg & 2), zz)) {
            output = spam(oparg, input);
        }
    """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            PyObject *cc;
            PyObject *input = NULL;
            PyObject *aa;
            PyObject *xx;
            PyObject *output = NULL;
            PyObject *zz;
            cc = stack_pointer[-1];
            if ((oparg & 1) == 1) { input = stack_pointer[-1 - (((oparg & 1) == 1) ? 1 : 0)]; }
            aa = stack_pointer[-2 - (((oparg & 1) == 1) ? 1 : 0)];
            output = spam(oparg, input);
            stack_pointer[-2 - (((oparg & 1) == 1) ? 1 : 0)] = xx;
            if (oparg & 2) stack_pointer[-1 - (((oparg & 1) == 1) ? 1 : 0)] = output;
            stack_pointer[-1 - (((oparg & 1) == 1) ? 1 : 0) + ((oparg & 2) ? 1 : 0)] = zz;
            stack_pointer += -(((oparg & 1) == 1) ? 1 : 0) + ((oparg & 2) ? 1 : 0);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_macro_cond_effect(self):
        input = """
        op(A, (left, middle, right --)) {
            # Body of A
        }
        op(B, (-- deep, extra if (oparg), res)) {
            # Body of B
        }
        macro(M) = A + B;
    """
        output = """
        TARGET(M) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            PyObject *right;
            PyObject *middle;
            PyObject *left;
            PyObject *deep;
            PyObject *extra = NULL;
            PyObject *res;
            // A
            right = stack_pointer[-1];
            middle = stack_pointer[-2];
            left = stack_pointer[-3];
            {
                # Body of A
            }
            // B
            {
                # Body of B
            }
            stack_pointer[-3] = deep;
            if (oparg) stack_pointer[-2] = extra;
            stack_pointer[-2 + ((oparg) ? 1 : 0)] = res;
            stack_pointer += -1 + ((oparg) ? 1 : 0);
            DISPATCH();
        }
    """
        self.run_cases_test(input, output)

    def test_macro_push_push(self):
        input = """
        op(A, (-- val1)) {
            val1 = spam();
        }
        op(B, (-- val2)) {
            val2 = spam();
        }
        macro(M) = A + B;
        """
        output = """
        TARGET(M) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            PyObject *val1;
            PyObject *val2;
            // A
            {
                val1 = spam();
            }
            // B
            {
                val2 = spam();
            }
            stack_pointer[0] = val1;
            stack_pointer[1] = val2;
            stack_pointer += 2;
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_inst(self):
        input = """
        inst(OP, (--)) {
            spam();
        }
        override inst(OP, (--)) {
            ham();
        }
        """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_override_op(self):
        input = """
        op(OP, (--)) {
            spam();
        }
        macro(M) = OP;
        override op(OP, (--)) {
            ham();
        }
        """
        output = """
        TARGET(M) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            ham();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_inst(self):
        input = """
        pure inst(OP, (--)) {
            ham();
        }
        """
        output = """
        TARGET(OP) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(OP);
            ham();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

    def test_annotated_op(self):
        input = """
        pure op(OP, (--)) {
            spam();
        }
        macro(M) = OP;
        """
        output = """
        TARGET(M) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(M);
            spam();
            DISPATCH();
        }
        """
        self.run_cases_test(input, output)

        input = """
        pure register specializing op(OP, (--)) {
            spam();
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
        with self.assertRaises(Exception):
            self.run_cases_test(input, output)

    def test_pop_on_error_peeks(self):

        input = """
        op(FIRST, (x, y -- a, b)) {
            a = x;
            b = y;
        }

        op(SECOND, (a, b -- a, b)) {
        }

        op(THIRD, (j, k --)) {
            ERROR_IF(cond, error);
        }

        macro(TEST) = FIRST + SECOND + THIRD;
        """
        output = """
        TARGET(TEST) {
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(TEST);
            PyObject *y;
            PyObject *x;
            PyObject *a;
            PyObject *b;
            PyObject *k;
            PyObject *j;
            // FIRST
            y = stack_pointer[-1];
            x = stack_pointer[-2];
            {
                a = x;
                b = y;
            }
            // SECOND
            {
            }
            // THIRD
            k = b;
            j = a;
            {
                if (cond) goto pop_2_error;
            }
            stack_pointer += -2;
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
            spam();
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
            spam();
        }
        op(OP2, (arg1 -- out)) {
            eggs();
        }
        """
        input2 = """
        op(OP, (arg1 -- out)) {
            eggs();
        }
        """
        output = """
        case OP: {
            _Py_UopsSymbol *arg1;
            _Py_UopsSymbol *out;
            arg1 = stack_pointer[-1];
            eggs();
            stack_pointer[-1] = out;
            break;
        }

        case OP2: {
            _Py_UopsSymbol *out;
            out = sym_new_not_null(ctx);
            if (out == NULL) goto out_of_space;
            stack_pointer[-1] = out;
            break;
        }
       """
        self.run_cases_test(input, input2, output)

    def test_no_overridden_case(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            spam();
        }

        pure op(OP2, (arg1 -- out)) {
        }

        """
        input2 = """
        pure op(OP2, (arg1 -- out)) {
        }
        """
        output = """
        case OP: {
            _Py_UopsSymbol *out;
            out = sym_new_not_null(ctx);
            if (out == NULL) goto out_of_space;
            stack_pointer[-1] = out;
            break;
        }

        case OP2: {
            _Py_UopsSymbol *arg1;
            _Py_UopsSymbol *out;
            arg1 = stack_pointer[-1];
            stack_pointer[-1] = out;
            break;
        }
        """
        self.run_cases_test(input, input2, output)

    def test_missing_override_failure(self):
        input = """
        pure op(OP, (arg1 -- out)) {
            spam();
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


if __name__ == "__main__":
    unittest.main()
