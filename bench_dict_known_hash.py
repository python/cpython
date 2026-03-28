import time

N = 50_000_000


def bench_get_str(n):
    d = {'a': 1, 'b': 2, 'c': 3}
    x = 0
    for _ in range(n):
        x += d['a']
    return x


def bench_get_int(n):
    d = {1: 10, 2: 20, 3: 30}
    x = 0
    for _ in range(n):
        x += d[1]
    return x


def bench_store_str(n):
    d = {}
    for _ in range(n):
        d['a'] = 1
    return d


def bench_store_int(n):
    d = {}
    for _ in range(n):
        d[1] = 1
    return d


def bench_get_multi(n):
    d = {'a': 1, 1: 2, b'x': 3, (1, 2): 4}
    x = 0
    for _ in range(n):
        x += d['a'] + d[1] + d[b'x'] + d[(1, 2)]
    return x


def run(name, func, n):
    # warmup
    func(1000)
    t0 = time.perf_counter()
    func(n)
    dt = time.perf_counter() - t0
    print(f"{name:25s}  {dt:.3f}s  ({n/dt/1e6:.1f}M iter/s)")


if __name__ == "__main__":
    import sys
    import os
    jit = "JIT" if os.environ.get("PYTHON_JIT", "1") == "1" else "no JIT"
    print(f"Python {sys.version.split()[0]} ({jit})\n")

    run("dict_get[str]",    bench_get_str,   N)
    run("dict_get[int]",    bench_get_int,   N)
    run("dict_store[str]",  bench_store_str, N)
    run("dict_store[int]",  bench_store_int, N)
    run("dict_get[multi]",  bench_get_multi, N // 4)
