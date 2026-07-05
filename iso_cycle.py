import itertools
from test.support import threading_helper
errs=[]
def w_count():
    c=g
    for _ in range(20000):
        next(c)
def w_cycle():
    for _ in range(20000): next(cy)
def w_tee():
    for _ in a: pass
if "cycle"=="count":
    g=itertools.count()
    threading_helper.run_concurrently(w_count, nthreads=8)
elif "cycle"=="cycle":
    cy=itertools.cycle([1,2,3,4,5])
    threading_helper.run_concurrently(w_cycle, nthreads=8)
else:
    a,b=itertools.tee(iter(range(2_000_000)),2)
    threading_helper.run_concurrently(w_tee, nthreads=8)
print("cycle DONE", flush=True)
