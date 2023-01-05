# Sorry for using pytest, these tests are mostly just for me.
# Use pytest -vv for best results.

import tempfile

import generate_cases


def run_cases_test(input: str, expected: str):
    temp_input = tempfile.NamedTemporaryFile("w+")
    temp_input.write(generate_cases.BEGIN_MARKER)
    temp_input.write(input)
    temp_input.write(generate_cases.END_MARKER)
    temp_input.flush()
    temp_output = tempfile.NamedTemporaryFile("w+")
    a = generate_cases.Analyzer(temp_input.name, temp_output.name)
    a.parse()
    a.analyze()
    if a.errors:
        raise RuntimeError(f"Found {a.errors} errors")
    a.write_instructions()
    temp_output.seek(0)
    lines = temp_output.readlines()
    while lines and lines[0].startswith("// "):
        lines.pop(0)
    actual = "".join(lines)
    assert actual.rstrip() == expected.rstrip()

def test_legacy():
    input = """
        inst(OP) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            spam();
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_inst_no_args():
    input = """
        inst(OP, (--)) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            spam();
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_inst_one_pop():
    input = """
        inst(OP, (value --)) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            PyObject *value = PEEK(1);
            spam();
            STACK_SHRINK(1);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_inst_one_push():
    input = """
        inst(OP, (-- res)) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            PyObject *res;
            spam();
            STACK_GROW(1);
            POKE(1, res);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_inst_one_push_one_pop():
    input = """
        inst(OP, (value -- res)) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            PyObject *value = PEEK(1);
            PyObject *res;
            spam();
            POKE(1, res);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_binary_op():
    input = """
        inst(OP, (left, right -- res)) {
            spam();
        }
    """
    output = """
        TARGET(OP) {
            PyObject *right = PEEK(1);
            PyObject *left = PEEK(2);
            PyObject *res;
            spam();
            STACK_SHRINK(1);
            POKE(1, res);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_predictions():
    input = """
        inst(OP1, (--)) {
        }
        inst(OP2, (--)) {
        }
        inst(OP3, (--)) {
            DEOPT_IF(xxx, OP1);
            PREDICT(OP2);
        }
    """
    output = """
        TARGET(OP1) {
            PREDICTED(OP1);
            DISPATCH();
        }

        TARGET(OP2) {
            PREDICTED(OP2);
            DISPATCH();
        }

        TARGET(OP3) {
            DEOPT_IF(xxx, OP1);
            PREDICT(OP2);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_error_if_plain():
    input = """
        inst(OP, (--)) {
            ERROR_IF(cond, label);
        }
    """
    output = """
        TARGET(OP) {
            if (cond) goto label;
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_error_if_pop():
    input = """
        inst(OP, (left, right -- res)) {
            ERROR_IF(cond, label);
        }
    """
    output = """
        TARGET(OP) {
            PyObject *right = PEEK(1);
            PyObject *left = PEEK(2);
            PyObject *res;
            if (cond) goto pop_2_label;
            STACK_SHRINK(1);
            POKE(1, res);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_cache_effect():
    input = """
        inst(OP, (counter/1, extra/2, value --)) {
        }
    """
    output = """
        TARGET(OP) {
            PyObject *value = PEEK(1);
            uint16_t counter = read_u16(&next_instr[0].cache);
            uint32_t extra = read_u32(&next_instr[1].cache);
            STACK_SHRINK(1);
            JUMPBY(3);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_suppress_dispatch():
    input = """
        inst(OP, (--)) {
            goto somewhere;
        }
    """
    output = """
        TARGET(OP) {
            goto somewhere;
        }
    """
    run_cases_test(input, output)

def test_super_instruction():
    # TODO: Test cache effect
    input = """
        inst(OP1, (counter/1, arg --)) {
            op1();
        }
        inst(OP2, (extra/2, arg --)) {
            op2();
        }
        super(OP) = OP1 + OP2;
    """
    output = """
        TARGET(OP1) {
            PyObject *arg = PEEK(1);
            uint16_t counter = read_u16(&next_instr[0].cache);
            op1();
            STACK_SHRINK(1);
            JUMPBY(1);
            DISPATCH();
        }

        TARGET(OP2) {
            PyObject *arg = PEEK(1);
            uint32_t extra = read_u32(&next_instr[0].cache);
            op2();
            STACK_SHRINK(1);
            JUMPBY(2);
            DISPATCH();
        }

        TARGET(OP) {
            PyObject *_tmp_1 = PEEK(1);
            PyObject *_tmp_2 = PEEK(2);
            {
                PyObject *arg = _tmp_1;
                uint16_t counter = read_u16(&next_instr[0].cache);
                op1();
            }
            JUMPBY(1);
            NEXTOPARG();
            JUMPBY(1);
            {
                PyObject *arg = _tmp_2;
                uint32_t extra = read_u32(&next_instr[0].cache);
                op2();
            }
            JUMPBY(2);
            STACK_SHRINK(2);
            DISPATCH();
        }
    """
    run_cases_test(input, output)

def test_macro_instruction():
    input = """
        inst(OP1, (counter/1, arg --)) {
            op1();
        }
        op(OP2, (extra/2, arg --)) {
            op2();
        }
        macro(OP) = OP1 + cache/2 + OP2;
    """
    output = """
        TARGET(OP1) {
            PyObject *arg = PEEK(1);
            uint16_t counter = read_u16(&next_instr[0].cache);
            op1();
            STACK_SHRINK(1);
            JUMPBY(1);
            DISPATCH();
        }

        TARGET(OP) {
            PyObject *_tmp_1 = PEEK(1);
            PyObject *_tmp_2 = PEEK(2);
            {
                PyObject *arg = _tmp_1;
                uint16_t counter = read_u16(&next_instr[0].cache);
                op1();
            }
            {
                PyObject *arg = _tmp_2;
                uint32_t extra = read_u32(&next_instr[3].cache);
                op2();
            }
            JUMPBY(5);
            STACK_SHRINK(2);
            DISPATCH();
        }
    """
    run_cases_test(input, output)
