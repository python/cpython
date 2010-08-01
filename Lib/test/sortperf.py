"""Sort performance test.

See main() for command line syntax.
See tabulate() for output format.

"""

import sys
import time
import random
import marshal
import tempfile
import os

td = tempfile.gettempdir()

def randfloats(n):
    """Return a list of n random floats in [0, 1)."""
    # Generating floats is expensive, so this writes them out to a file in
    # a temp directory.  If the file already exists, it just reads them
    # back in and shuffles them a bit.
    fn = os.path.join(td, "rr%06d" % n)
    try:
        fp = open(fn, "rb")
    except IOError:
        r = random.random
        result = [r() for i in range(n)]
        try:
            try:
                fp = open(fn, "wb")
                marshal.dump(result, fp)
                fp.close()
                fp = None
            finally:
                if fp:
                    try:
                        os.unlink(fn)
                    except os.error:
                        pass
        except IOError as msg:
            print("can't write", fn, ":", msg)
    else:
        result = marshal.load(fp)
        fp.close()
        # Shuffle it a bit...
        for i in range(10):
            i = random.randrange(n)
            temp = result[:i]
            del result[:i]
            temp.reverse()
            result.extend(temp)
            del temp
    assert len(result) == n
    return result

def flush():
    sys.stdout.flush()

def doit(L):
    t0 = time.clock()
    L.sort()
    t1 = time.clock()
    print("%6.2f" % (t1-t0), end=' ')
    flush()

def tabulate(r):
    """Tabulate sort speed for lists of various sizes.

    The sizes are 2**i for i in r (the argument, a list).

    The output displays i, 2**i, and the time to sort arrays of 2**i
    floating point numbers with the following properties:

    *sort: random data
    \sort: descending data
    /sort: ascending data
    3sort: ascending, then 3 random exchanges
    +sort: ascending, then 10 random at the end
    %sort: ascending, then randomly replace 1% of the elements w/ random values
    ~sort: many duplicates
    =sort: all equal
    !sort: worst case scenario

    """
    cases = tuple([ch + "sort" for ch in r"*\/3+%~=!"])
    fmt = ("%2s %7s" + " %6s"*len(cases))
    print(fmt % (("i", "2**i") + cases))
    for i in r:
        n = 1 << i
        L = randfloats(n)
        print("%2d %7d" % (i, n), end=' ')
        flush()
        doit(L) # *sort
        L.reverse()
        doit(L) # \sort
        doit(L) # /sort

        # Do 3 random exchanges.
        for dummy in range(3):
            i1 = random.randrange(n)
            i2 = random.randrange(n)
            L[i1], L[i2] = L[i2], L[i1]
        doit(L) # 3sort

        # Replace the last 10 with random floats.
        if n >= 10:
            L[-10:] = [random.random() for dummy in range(10)]
        doit(L) # +sort

        # Replace 1% of the elements at random.
        for dummy in range(n // 100):
            L[random.randrange(n)] = random.random()
        doit(L) # %sort

        # Arrange for lots of duplicates.
        if n > 4:
            del L[4:]
            L = L * (n // 4)
            # Force the elements to be distinct objects, else timings can be
            # artificially low.
            L = list(map(lambda x: --x, L))
        doit(L) # ~sort
        del L

        # All equal.  Again, force the elements to be distinct objects.
        L = list(map(abs, [-0.5] * n))
        doit(L) # =sort
        del L

        # This one looks like [3, 2, 1, 0, 0, 1, 2, 3].  It was a bad case
        # for an older implementation of quicksort, which used the median
        # of the first, last and middle elements as the pivot.
        half = n // 2
        L = list(range(half - 1, -1, -1))
        L.extend(range(half))
        # Force to float, so that the timings are comparable.  This is
        # significantly faster if we leave tham as ints.
        L = list(map(float, L))
        doit(L) # !sort
        print()

def main():
    """Main program when invoked as a script.

    One argument: tabulate a single row.
    Two arguments: tabulate a range (inclusive).
    Extra arguments are used to seed the random generator.

    """
    # default range (inclusive)
    k1 = 15
    k2 = 20
    if sys.argv[1:]:
        # one argument: single point
        k1 = k2 = int(sys.argv[1])
        if sys.argv[2:]:
            # two arguments: specify range
            k2 = int(sys.argv[2])
            if sys.argv[3:]:
                # derive random seed from remaining arguments
                x = 1
                for a in sys.argv[3:]:
                    x = 69069 * x + hash(a)
                random.seed(x)
    r = range(k1, k2+1)                 # include the end point
    tabulate(r)

if __name__ == '__main__':
    main()
