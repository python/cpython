# ./python.exe -m Tools.justin

import timeit

from . import Engine

def fib(n: float) -> float:
    a, b = 0.0, 1.0
    while n > 0.0:
        a, b = b, a + b
        n -= 1.0
    return a

# First, create our JIT engine:
engine = Engine(verbose=True)
# This performs all of the steps that normally happen at build time:
engine.build()
# This performs all of the steps that normally happen during startup:
engine.load()

fib_jit = engine.trace(fib)

size = 10_000_000.0
fib_jit_time = timeit.timeit(f"fib_jit({size:_})", globals=globals(), number=10)
fib_time = timeit.timeit(f"fib({size:_})", globals=globals(), number=10)

print(f"fib_jit is {fib_time / fib_jit_time - 1:.0%} faster than fib!")

assert fib(100.0) == fib_jit(100.0)
assert fib(100) == fib_jit(100)
