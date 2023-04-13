"""
The Computer Language Benchmarks Game
http://benchmarksgame.alioth.debian.org/

Contributed by Sokolov Yura, modified by Tupteq.
"""

# ./python -m Tools.justin.bm_fannkuch

import time

from . import trace


DEFAULT_ARG = 9


def fannkuch(n):
    count = list(range(1, n + 1))
    max_flips = 0
    m = n - 1
    r = n
    perm1 = list(range(n))
    perm = list(range(n))
    perm1_ins = perm1.insert
    perm1_pop = perm1.pop

    while 1:
        while r != 1:
            count[r - 1] = r
            r -= 1

        if perm1[0] != 0 and perm1[m] != m:
            perm = perm1[:]
            flips_count = 0
            k = perm[0]
            while k:
                perm[:k + 1] = perm[k::-1]
                flips_count += 1
                k = perm[0]

            if flips_count > max_flips:
                max_flips = flips_count

        while r != n:
            perm1_ins(r, perm1_pop(0))
            count[r] -= 1
            if count[r] > 0:
                break
            r += 1
        else:
            return max_flips
        
def bench_fannkuch(loops: int) -> float:
    range_it = range(loops)
    t0 = time.perf_counter()
    for _ in range_it:
        fannkuch(DEFAULT_ARG)
    return time.perf_counter() - t0

loops = 1 << 2
fannkuch_time = bench_fannkuch(loops)
fannkuch = trace(fannkuch)
fannkuch_jit_time = bench_fannkuch(loops)

print(f"fannkuch_jit is {fannkuch_time / fannkuch_jit_time - 1:.0%} faster than fannkuch!")
print(round(fannkuch_time, 3), round(fannkuch_jit_time, 3))
