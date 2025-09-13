"""
Benchmark for recursive coroutines.

Author: Kumar Aditya
"""


async def fibonacci(n: int) -> int:
    if n <= 1:
        return n
    return await fibonacci(n - 1) + await fibonacci(n - 2)


def bench_coroutines(loops: int) -> float:
    range_it = range(loops)
    for _ in range_it:
        coro = fibonacci(25)
        try:
            while True:
                coro.send(None)
        except StopIteration:
            pass


def run_pgo():
    bench_coroutines(10)
