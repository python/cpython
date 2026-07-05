import itertools
from test.support import threading_helper
errs=[]
for _ in range(40):
    a, b = itertools.tee(iter(range(300_000)), 2)
    def w():
        try:
            for _ in a: pass
        except Exception as e: errs.append(repr(e))
    threading_helper.run_concurrently(w, nthreads=8)
print("SAME_DONE errors=", len(errs), errs[:3], flush=True)
