import itertools
from test.support import threading_helper
errs=[]
for r in range(40):
    a,b,c = itertools.tee(iter(range(80_000)), 3)
    br=[a,b,c]; cnt=[0]
    def w():
        i=cnt[0]; cnt[0]+=1
        try:
            for _ in br[i%3]: pass
        except Exception as e: errs.append(repr(e))
    threading_helper.run_concurrently(w, nthreads=16)
print("HEAVY_DONE errors=",len(errs),errs[:3],flush=True)
