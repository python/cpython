# ./python.exe -m Tools.justin

import timeit

from . import Engine

# First, create our JIT engine:
engine = Engine()
# This performs all of the steps that normally happen at build time:
engine.build()
# This performs all of the steps that normally happen during startup:
engine.load()

def fib(n: float, a: float = 0.0, b: float = 1.0) -> float:
    while n > 0.0:
        a, b = b, a + b
        n -= 1.0
    return a

def fib_jit(n: float, a: float = 0.0, b: float = 1.0) -> float:
    if n > 0.0:
        trace.run_here()
    return a

fib_jit.__code__ = fib_jit.__code__.replace(co_consts=fib.__code__.co_consts)  # XXX

fib_result = fib(100.0)

#          0 RESUME                    0
#          2 LOAD_FAST__LOAD_CONST     0 (n)
#          4 LOAD_CONST                1 (0)
#          6 COMPARE_AND_BRANCH_FLOAT 75 (>)
#         10 POP_JUMP_IF_FALSE        18 (to 48)
# --> >>  12 LOAD_FAST__LOAD_FAST      2 (b)
#         14 LOAD_FAST__LOAD_FAST      1 (a)
# -->     16 LOAD_FAST                 2 (b)
# -->     18 BINARY_OP_ADD_FLOAT       0 (+)
# -->     22 STORE_FAST__STORE_FAST    2 (b)
#         24 STORE_FAST__LOAD_FAST     1 (a)
# -->     26 LOAD_FAST__LOAD_CONST     0 (n)
#         28 LOAD_CONST                2 (1)
# -->     30 BINARY_OP_SUBTRACT_FLOAT 23 (-=)
# -->     34 STORE_FAST__LOAD_FAST     0 (n)
#         36 LOAD_FAST__LOAD_CONST     0 (n)
# -->     38 LOAD_CONST                1 (0)
# -->     40 COMPARE_AND_BRANCH_FLOAT 75 (>)
#         44 POP_JUMP_IF_FALSE         1 (to 48)
# -->     46 JUMP_BACKWARD            18 (to 12)
#     >>  48 LOAD_FAST                 1 (a)
#         50 RETURN_VALUE

trace = engine.compile_trace(
    fib.__code__, [12, 16, 18, 22, 26, 30, 34, 38, 40, 46]
)

fib_jit_result = fib_jit(100.0)
assert fib_result == fib_jit_result, f"{fib_result} != {fib_jit_result}"

size = 100_000_000.0
fib_time = timeit.timeit(f"fib({size:_})", globals=globals(), number=1)
fib_jit_time = timeit.timeit(f"fib_jit({size:_})", globals=globals(), number=1)

print(f"fib_jit is {fib_time / fib_jit_time - 1:.0%} faster than fib!")
