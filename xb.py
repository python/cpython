import itertools
from test.support import threading_helper
errs=[]
for _ in range(15):
    a,b = itertools.tee(iter(range(100_000)),2)
    br=[a,b]; c=[0]
    def w():
        i=c[0]; c[0]+=1
        try:
            for _ in br[i%2]: pass
        except Exception as e: errs.append(repr(e))
    threading_helper.run_concurrently(w, nthreads=6)
print("XB_DONE errors=",len(errs),errs[:3],flush=True)
