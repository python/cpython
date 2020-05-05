#!/usr/bin/env python
"""

Intel(R) Core(TM) i7-8650U CPU @ 1.90GHz

100 digits
1000000 loops, best of 5: 371 nsec per loop
1000 digits
50000 loops, best of 5: 7.94 usec per loop
5000 digits
2000 loops, best of 5: 142 usec per loop
10000 digits
500 loops, best of 5: 543 usec per loop
25000 digits
100 loops, best of 5: 3.31 msec per loop
50000 digits
20 loops, best of 5: 12.8 msec per loop
100000 digits
5 loops, best of 5: 52.4 msec per loop
250000 digits
1 loop, best of 5: 318 msec per loop
500000 digits
1 loop, best of 5: 1.27 sec per loop
1000000 digits
1 loop, best of 5: 5.2 sec per loop
"""
import timeit


for i in [
    100,
    1_000,
    5_000,
    10_000,
    25_000,
    50_000,
    100_000,
    250_000,
    500_000,
    1_000_000,
]:
    print(f"{i} digits")
    timeit.main(
        ["-s", f"s = '1' + ('0' * {i})", "int(s)",]
    )
