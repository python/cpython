from dis import dis, Bytecode
import inspect

source = """
def func():
    x, y = 0, 1
    z = (x or 1) if y else 1
    print(z)
"""

# source = """
# def func():
#     z = 0.1
#     if z:
#         x, y = 0, 1
#     else:
#         x, y = 1, 0
#     print(x, y)
# """

func = compile(source, "inline_bug_report.py", "exec", optimize=2)

print(dis(func))

# for name, value in inspect.getmembers(func.__code__):
#     print(name, value)

# print("-- lines are --")
# lines = [line for line in func.__code__.co_lines()]
# print(lines)

# for code in Bytecode(func):
#     print(f"{code.line_number} {code.opcode:06x} {code.opname}")
