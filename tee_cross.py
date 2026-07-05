import itertools
from test.support import threading_helper
# cross-branch: a,b share the same teedataobject; consume each in its own thread
errs = []
def consume(it):
    try:
        for _ in it: pass
    except Exception as e:
        errs.append(repr(e))
for _ in range(30):
    a, b = itertools.tee(iter(range(500_000)), 2)
    branches = [a, b]
    def w(i=[0]):
        idx = i[0]; i[0]+=1
        consume(branches[idx % 2])
    threading_helper.run_concurrently(w, nthreads=8)
print("CROSS_DONE errors=", len(errs), errs[:3])
