
# This leaks since the introduction of yield-expr and the use of generators
# as coroutines, trunk revision 39239. The cycle-GC doesn't seem to pick up
# the cycle, or decides it can't clean it up.

def leak():
    def gen():
        while True:
            yield g
    g = gen()
