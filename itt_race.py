import itertools, collections
from test.support import threading_helper

# 1) itertools.count: concurrent next() -> lost/dup increments if C fast-path unsynced
def test_count():
    c = itertools.count()
    got = []
    def w():
        local = []
        for _ in range(20000):
            local.append(next(c))
        got.extend(local)
    threading_helper.run_concurrently(w, nthreads=8)
    dup = [v for v, n in collections.Counter(got).items() if n > 1]
    return f"count: total={len(got)} unique={len(set(got))} dups={len(dup)}"

# 2) itertools.cycle: concurrent next()
def test_cycle():
    cy = itertools.cycle([1,2,3,4,5])
    errs = []
    def w():
        try:
            for _ in range(20000):
                next(cy)
        except Exception as e:
            errs.append(repr(e))
    threading_helper.run_concurrently(w, nthreads=8)
    return f"cycle: errors={len(errs)} {errs[:2]}"

# 3) itertools.tee: concurrent iteration of one branch
def test_tee():
    a, b = itertools.tee(iter(range(2_000_000)), 2)
    errs = []
    def w(it):
        try:
            for _ in it:
                pass
        except Exception as e:
            errs.append(repr(e))
    threading_helper.run_concurrently(w, nthreads=8, args=(a,))
    return f"tee: errors={len(errs)} {errs[:2]}"

print(test_count())
print(test_cycle())
print(test_tee())
