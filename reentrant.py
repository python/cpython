import itertools, faulthandler, sys
faulthandler.dump_traceback_later(8, exit=True)  # if it hangs 8s -> dump + kill

# A source iterator whose __next__ re-enters the same tee branch.
class Reenter:
    def __init__(self): self.n=0; self.t=None
    def __iter__(self): return self
    def __next__(self):
        self.n += 1
        if self.n == 1 and self.t is not None:
            try:
                next(self.t)          # re-enter the SAME tee mid-fetch
            except RuntimeError as e:
                print("GOT RuntimeError (expected, by-design):", e)
        if self.n > 3: raise StopIteration
        return self.n

src = Reenter()
a, b = itertools.tee(src, 2)
src.t = a
print("result:", list(a))
print("NO_DEADLOCK")
