import contextlib
import dis
import io

Instruction = dis.Instruction

# Fodder for instruction introspection tests
#   Editing any of these may require recalculating the expected output
def outer(a=1, b=2):
    def f(c=3, d=4):
        def inner(e=5, f=6):
            print(a, b, c, d, e, f)
        print(a, b, c, d)
        return inner
    print(a, b, '', 1, [], {}, "Hello world!")
    return f

def jumpy():
    # This won't actually run (but that's OK, we only disassemble it)
    for i in range(10):
        print(i)
        if i < 4:
            continue
        if i > 6:
            break
    else:
        print("I can haz else clause?")
    while i:
        print(i)
        i -= 1
        if i > 6:
            continue
        if i < 4:
            break
    else:
        print("Who let lolcatz into this test suite?")
    try:
        1 / 0
    except ZeroDivisionError:
        print("Here we go, here we go, here we go...")
    else:
        with i as dodgy:
            print("Never reach this")
    finally:
        print("OK, now we're done")

# End fodder for opinfo generation tests
expected_outer_line = 1
_line_offset = outer.__code__.co_firstlineno - 1
code_object_f = outer.__code__.co_consts[1]
expected_f_line = code_object_f.co_firstlineno - _line_offset
code_object_inner = code_object_f.co_consts[1]
expected_inner_line = code_object_inner.co_firstlineno - _line_offset
expected_jumpy_line = 1

# The following lines are useful to regenerate the expected results after
# either the fodder is modified or the bytecode generation changes
# After regeneration, update the references to code_object_f and
# code_object_inner before rerunning the tests

def _stringify_instruction(instr):
    # Since line numbers and other offsets change a lot for these
    # test cases, ignore them.
    return f"  {instr._replace(positions=None)!r},"

def _prepare_test_cases():
    ignore = io.StringIO()
    with contextlib.redirect_stdout(ignore):
        f = outer()
        inner = f()
    _instructions_outer = dis.get_instructions(outer, first_line=expected_outer_line)
    _instructions_f = dis.get_instructions(f, first_line=expected_f_line)
    _instructions_inner = dis.get_instructions(inner, first_line=expected_inner_line)
    _instructions_jumpy = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
    result = "\n".join(
        [
            "expected_opinfo_outer = [",
            *map(_stringify_instruction, _instructions_outer),
            "]",
            "",
            "expected_opinfo_f = [",
            *map(_stringify_instruction, _instructions_f),
            "]",
            "",
            "expected_opinfo_inner = [",
            *map(_stringify_instruction, _instructions_inner),
            "]",
            "",
            "expected_opinfo_jumpy = [",
            *map(_stringify_instruction, _instructions_jumpy),
            "]",
        ]
    )
    result = result.replace(repr(repr(code_object_f)), "repr(code_object_f)")
    result = result.replace(repr(code_object_f), "code_object_f")
    result = result.replace(repr(repr(code_object_inner)), "repr(code_object_inner)")
    result = result.replace(repr(code_object_inner), "code_object_inner")
    print(result)


Instruction = dis.Instruction

expected_opinfo_outer = [
  Instruction(opname='MAKE_CELL', opcode=91, arg=0, argval='a', argrepr='a', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='MAKE_CELL', opcode=91, arg=1, argval='b', argrepr='b', offset=2, start_offset=2, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=4, start_offset=4, starts_line=True, line_number=1, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=5, argval=(3, 4), argrepr='(3, 4)', offset=6, start_offset=6, starts_line=True, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='a', argrepr='a', offset=8, start_offset=8, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=1, argval='b', argrepr='b', offset=10, start_offset=10, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='BUILD_TUPLE', opcode=48, arg=2, argval=2, argrepr='', offset=12, start_offset=12, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=1, argval=code_object_f, argrepr=repr(code_object_f), offset=14, start_offset=14, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='MAKE_FUNCTION', opcode=23, arg=None, argval=None, argrepr='', offset=16, start_offset=16, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=103, arg=8, argval=8, argrepr='closure', offset=18, start_offset=18, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=103, arg=1, argval=1, argrepr='defaults', offset=20, start_offset=20, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='STORE_FAST', opcode=107, arg=2, argval='f', argrepr='f', offset=22, start_offset=22, starts_line=False, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=1, argval='print', argrepr='print + NULL', offset=24, start_offset=24, starts_line=True, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=0, argval='a', argrepr='a', offset=34, start_offset=34, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=1, argval='b', argrepr='b', offset=36, start_offset=36, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=2, argval='', argrepr="''", offset=38, start_offset=38, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=3, argval=1, argrepr='1', offset=40, start_offset=40, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='BUILD_LIST', opcode=43, arg=0, argval=0, argrepr='', offset=42, start_offset=42, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='BUILD_MAP', opcode=44, arg=0, argval=0, argrepr='', offset=44, start_offset=44, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=4, argval='Hello world!', argrepr="'Hello world!'", offset=46, start_offset=46, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=7, argval=7, argrepr='', offset=48, start_offset=48, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=56, start_offset=56, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=2, argval='f', argrepr='f', offset=58, start_offset=58, starts_line=True, line_number=8, label=None, positions=None, cache_info=None),
  Instruction(opname='RETURN_VALUE', opcode=33, arg=None, argval=None, argrepr='', offset=60, start_offset=60, starts_line=False, line_number=8, label=None, positions=None, cache_info=None),
]

expected_opinfo_f = [
  Instruction(opname='COPY_FREE_VARS', opcode=58, arg=2, argval=2, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='MAKE_CELL', opcode=91, arg=0, argval='c', argrepr='c', offset=2, start_offset=2, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='MAKE_CELL', opcode=91, arg=1, argval='d', argrepr='d', offset=4, start_offset=4, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=6, start_offset=6, starts_line=True, line_number=2, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=2, argval=(5, 6), argrepr='(5, 6)', offset=8, start_offset=8, starts_line=True, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=3, argval='a', argrepr='a', offset=10, start_offset=10, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=4, argval='b', argrepr='b', offset=12, start_offset=12, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='c', argrepr='c', offset=14, start_offset=14, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=1, argval='d', argrepr='d', offset=16, start_offset=16, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='BUILD_TUPLE', opcode=48, arg=4, argval=4, argrepr='', offset=18, start_offset=18, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=1, argval=code_object_inner, argrepr=repr(code_object_inner), offset=20, start_offset=20, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='MAKE_FUNCTION', opcode=23, arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=103, arg=8, argval=8, argrepr='closure', offset=24, start_offset=24, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=103, arg=1, argval=1, argrepr='defaults', offset=26, start_offset=26, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='STORE_FAST', opcode=107, arg=2, argval='inner', argrepr='inner', offset=28, start_offset=28, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=1, argval='print', argrepr='print + NULL', offset=30, start_offset=30, starts_line=True, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=3, argval='a', argrepr='a', offset=40, start_offset=40, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=4, argval='b', argrepr='b', offset=42, start_offset=42, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=0, argval='c', argrepr='c', offset=44, start_offset=44, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=1, argval='d', argrepr='d', offset=46, start_offset=46, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=4, argval=4, argrepr='', offset=48, start_offset=48, starts_line=False, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=56, start_offset=56, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=2, argval='inner', argrepr='inner', offset=58, start_offset=58, starts_line=True, line_number=6, label=None, positions=None, cache_info=None),
  Instruction(opname='RETURN_VALUE', opcode=33, arg=None, argval=None, argrepr='', offset=60, start_offset=60, starts_line=False, line_number=6, label=None, positions=None, cache_info=None),
]

expected_opinfo_inner = [
  Instruction(opname='COPY_FREE_VARS', opcode=58, arg=4, argval=4, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=2, start_offset=2, starts_line=True, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=1, argval='print', argrepr='print + NULL', offset=4, start_offset=4, starts_line=True, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=2, argval='a', argrepr='a', offset=14, start_offset=14, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=3, argval='b', argrepr='b', offset=16, start_offset=16, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=4, argval='c', argrepr='c', offset=18, start_offset=18, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_DEREF', opcode=80, arg=5, argval='d', argrepr='d', offset=20, start_offset=20, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST_LOAD_FAST', opcode=84, arg=1, argval=('e', 'f'), argrepr='e, f', offset=22, start_offset=22, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=6, argval=6, argrepr='', offset=24, start_offset=24, starts_line=False, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=32, start_offset=32, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='RETURN_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=34, start_offset=34, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
]

expected_opinfo_jumpy = [
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=1, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=1, argval='range', argrepr='range + NULL', offset=2, start_offset=2, starts_line=True, line_number=3, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=1, argval=10, argrepr='10', offset=12, start_offset=12, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=14, start_offset=14, starts_line=False, line_number=3, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='GET_ITER', opcode=16, arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='FOR_ITER', opcode=67, arg=30, argval=88, argrepr='to L4', offset=24, start_offset=24, starts_line=False, line_number=3, label=1, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='STORE_FAST', opcode=107, arg=0, argval='i', argrepr='i', offset=28, start_offset=28, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=30, start_offset=30, starts_line=True, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=40, start_offset=40, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=42, start_offset=42, starts_line=False, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=50, start_offset=50, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=52, start_offset=52, starts_line=True, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=2, argval=4, argrepr='4', offset=54, start_offset=54, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=54, arg=18, argval='<', argrepr='bool(<)', offset=56, start_offset=56, starts_line=False, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=94, arg=2, argval=68, argrepr='to L2', offset=60, start_offset=60, starts_line=False, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=72, arg=22, argval=24, argrepr='to L1', offset=64, start_offset=64, starts_line=True, line_number=6, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=68, start_offset=68, starts_line=True, line_number=7, label=2, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=3, argval=6, argrepr='6', offset=70, start_offset=70, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=54, arg=148, argval='>', argrepr='bool(>)', offset=72, start_offset=72, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=97, arg=2, argval=84, argrepr='to L3', offset=76, start_offset=76, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=72, arg=30, argval=24, argrepr='to L1', offset=80, start_offset=80, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=84, start_offset=84, starts_line=True, line_number=8, label=3, positions=None, cache_info=None),
  Instruction(opname='JUMP_FORWARD', opcode=74, arg=13, argval=114, argrepr='to L5', offset=86, start_offset=86, starts_line=False, line_number=8, label=None, positions=None, cache_info=None),
  Instruction(opname='END_FOR', opcode=9, arg=None, argval=None, argrepr='', offset=88, start_offset=88, starts_line=True, line_number=3, label=4, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=90, start_offset=90, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=92, start_offset=92, starts_line=True, line_number=10, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=4, argval='I can haz else clause?', argrepr="'I can haz else clause?'", offset=102, start_offset=102, starts_line=False, line_number=10, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=104, start_offset=104, starts_line=False, line_number=10, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=112, start_offset=112, starts_line=False, line_number=10, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST_CHECK', opcode=83, arg=0, argval='i', argrepr='i', offset=114, start_offset=114, starts_line=True, line_number=11, label=5, positions=None, cache_info=None),
  Instruction(opname='TO_BOOL', opcode=37, arg=None, argval=None, argrepr='', offset=116, start_offset=116, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=94, arg=33, argval=194, argrepr='to L8', offset=124, start_offset=124, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=128, start_offset=128, starts_line=True, line_number=12, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=138, start_offset=138, starts_line=False, line_number=12, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=140, start_offset=140, starts_line=False, line_number=12, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=148, start_offset=148, starts_line=False, line_number=12, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=150, start_offset=150, starts_line=True, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=5, argval=1, argrepr='1', offset=152, start_offset=152, starts_line=False, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='BINARY_OP', opcode=42, arg=23, argval=23, argrepr='-=', offset=154, start_offset=154, starts_line=False, line_number=13, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='STORE_FAST', opcode=107, arg=0, argval='i', argrepr='i', offset=158, start_offset=158, starts_line=False, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=160, start_offset=160, starts_line=True, line_number=14, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=3, argval=6, argrepr='6', offset=162, start_offset=162, starts_line=False, line_number=14, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=54, arg=148, argval='>', argrepr='bool(>)', offset=164, start_offset=164, starts_line=False, line_number=14, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=94, arg=2, argval=176, argrepr='to L6', offset=168, start_offset=168, starts_line=False, line_number=14, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=72, arg=31, argval=114, argrepr='to L5', offset=172, start_offset=172, starts_line=True, line_number=15, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=176, start_offset=176, starts_line=True, line_number=16, label=6, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=2, argval=4, argrepr='4', offset=178, start_offset=178, starts_line=False, line_number=16, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=54, arg=18, argval='<', argrepr='bool(<)', offset=180, start_offset=180, starts_line=False, line_number=16, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=97, arg=2, argval=192, argrepr='to L7', offset=184, start_offset=184, starts_line=False, line_number=16, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=72, arg=39, argval=114, argrepr='to L5', offset=188, start_offset=188, starts_line=False, line_number=16, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_FORWARD', opcode=74, arg=11, argval=216, argrepr='to L9', offset=192, start_offset=192, starts_line=True, line_number=17, label=7, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=194, start_offset=194, starts_line=True, line_number=19, label=8, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=6, argval='Who let lolcatz into this test suite?', argrepr="'Who let lolcatz into this test suite?'", offset=204, start_offset=204, starts_line=False, line_number=19, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=206, start_offset=206, starts_line=False, line_number=19, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=214, start_offset=214, starts_line=False, line_number=19, label=None, positions=None, cache_info=None),
  Instruction(opname='NOP', opcode=27, arg=None, argval=None, argrepr='', offset=216, start_offset=216, starts_line=True, line_number=20, label=9, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=5, argval=1, argrepr='1', offset=218, start_offset=218, starts_line=True, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=7, argval=0, argrepr='0', offset=220, start_offset=220, starts_line=False, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='BINARY_OP', opcode=42, arg=11, argval=11, argrepr='/', offset=222, start_offset=222, starts_line=False, line_number=21, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=226, start_offset=226, starts_line=False, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=81, arg=0, argval='i', argrepr='i', offset=228, start_offset=228, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=57, arg=1, argval=1, argrepr='', offset=230, start_offset=230, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_SPECIAL', opcode=89, arg=1, argval=1, argrepr='__exit__', offset=232, start_offset=232, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='SWAP', opcode=112, arg=2, argval=2, argrepr='', offset=234, start_offset=234, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='SWAP', opcode=112, arg=3, argval=3, argrepr='', offset=236, start_offset=236, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_SPECIAL', opcode=89, arg=0, argval=0, argrepr='__enter__', offset=238, start_offset=238, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=0, argval=0, argrepr='', offset=240, start_offset=240, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='STORE_FAST', opcode=107, arg=1, argval='dodgy', argrepr='dodgy', offset=248, start_offset=248, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=250, start_offset=250, starts_line=True, line_number=26, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=8, argval='Never reach this', argrepr="'Never reach this'", offset=260, start_offset=260, starts_line=False, line_number=26, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=262, start_offset=262, starts_line=False, line_number=26, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=270, start_offset=270, starts_line=False, line_number=26, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=0, argval=None, argrepr='None', offset=272, start_offset=272, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=0, argval=None, argrepr='None', offset=274, start_offset=274, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=79, arg=0, argval=None, argrepr='None', offset=276, start_offset=276, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=3, argval=3, argrepr='', offset=278, start_offset=278, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=286, start_offset=286, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=288, start_offset=288, starts_line=True, line_number=28, label=10, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=298, start_offset=298, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=300, start_offset=300, starts_line=False, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=308, start_offset=308, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='RETURN_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=310, start_offset=310, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=30, arg=None, argval=None, argrepr='', offset=312, start_offset=312, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='WITH_EXCEPT_START', opcode=41, arg=None, argval=None, argrepr='', offset=314, start_offset=314, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='TO_BOOL', opcode=37, arg=None, argval=None, argrepr='', offset=316, start_offset=316, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=97, arg=1, argval=330, argrepr='to L11', offset=324, start_offset=324, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='RERAISE', opcode=99, arg=2, argval=2, argrepr='', offset=328, start_offset=328, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=330, start_offset=330, starts_line=False, line_number=25, label=11, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=28, arg=None, argval=None, argrepr='', offset=332, start_offset=332, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=334, start_offset=334, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=336, start_offset=336, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=338, start_offset=338, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='JUMP_BACKWARD_NO_INTERRUPT', opcode=73, arg=27, argval=288, argrepr='to L10', offset=340, start_offset=340, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=57, arg=3, argval=3, argrepr='', offset=342, start_offset=342, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=28, arg=None, argval=None, argrepr='', offset=344, start_offset=344, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=99, arg=1, argval=1, argrepr='', offset=346, start_offset=346, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=30, arg=None, argval=None, argrepr='', offset=348, start_offset=348, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=4, argval='ZeroDivisionError', argrepr='ZeroDivisionError', offset=350, start_offset=350, starts_line=True, line_number=22, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='CHECK_EXC_MATCH', opcode=5, arg=None, argval=None, argrepr='', offset=360, start_offset=360, starts_line=False, line_number=22, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=94, arg=14, argval=394, argrepr='to L12', offset=362, start_offset=362, starts_line=False, line_number=22, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=366, start_offset=366, starts_line=False, line_number=22, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=368, start_offset=368, starts_line=True, line_number=23, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=9, argval='Here we go, here we go, here we go...', argrepr="'Here we go, here we go, here we go...'", offset=378, start_offset=378, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=380, start_offset=380, starts_line=False, line_number=23, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=388, start_offset=388, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=28, arg=None, argval=None, argrepr='', offset=390, start_offset=390, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='JUMP_BACKWARD_NO_INTERRUPT', opcode=73, arg=53, argval=288, argrepr='to L10', offset=392, start_offset=392, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=99, arg=0, argval=0, argrepr='', offset=394, start_offset=394, starts_line=True, line_number=22, label=12, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=57, arg=3, argval=3, argrepr='', offset=396, start_offset=396, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=28, arg=None, argval=None, argrepr='', offset=398, start_offset=398, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=99, arg=1, argval=1, argrepr='', offset=400, start_offset=400, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=30, arg=None, argval=None, argrepr='', offset=402, start_offset=402, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=87, arg=3, argval='print', argrepr='print + NULL', offset=404, start_offset=404, starts_line=True, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=79, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=414, start_offset=414, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=49, arg=1, argval=1, argrepr='', offset=416, start_offset=416, starts_line=False, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=29, arg=None, argval=None, argrepr='', offset=424, start_offset=424, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=99, arg=0, argval=0, argrepr='', offset=426, start_offset=426, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=57, arg=3, argval=3, argrepr='', offset=428, start_offset=428, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=28, arg=None, argval=None, argrepr='', offset=430, start_offset=430, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=99, arg=1, argval=1, argrepr='', offset=432, start_offset=432, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
]

# One last piece of inspect fodder to check the default line number handling
def simple(): pass
expected_opinfo_simple = [
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=simple.__code__.co_firstlineno, label=None, positions=None),
  Instruction(opname='RETURN_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=2, start_offset=2, starts_line=False, line_number=simple.__code__.co_firstlineno, label=None),
]

# =============
# End of tests:

if __name__ == "__main__":
    _prepare_test_cases()
