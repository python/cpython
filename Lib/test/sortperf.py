"""Sort performance test.

See main() for command line syntax.
See tabulate() for output format.

"""

import sys
import time
import whrandom
import marshal
import tempfile
import operator
import os

td = tempfile.gettempdir()

def randrange(n):
    """Return a random shuffle of range(n)."""
    fn = os.path.join(td, "rr%06d" % n)
    try:
        fp = open(fn, "rb")
    except IOError:
        result = []
        for i in range(n):
            result.append(whrandom.random())
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
        except IOError, msg:
            print "can't write", fn, ":", msg
    else:
        result = marshal.load(fp)
        fp.close()
        ##assert len(result) == n
        # Shuffle it a bit...
        for i in range(10):
            i = whrandom.randint(0, n-1)
            temp = result[:i]
            del result[:i]
            temp.reverse()
            result[len(result):] = temp
            del temp
    return result

def fl():
    sys.stdout.flush()

def doit(L):
    t0 = time.clock()
    L.sort()
    t1 = time.clock()
    print "%6.2f" % (t1-t0),
    fl()

def tabulate(r):
    """Tabulate sort speed for lists of various sizes.

    The sizes are 2**i for i in r (the argument, a list).

    The output displays i, 2**i, and the time to sort arrays of 2**i
    floating point numbers with the following properties:

    *sort: random data
    \sort: descending data
    /sort: ascending data
    ~sort: many duplicates
    -sort: all equal
    !sort: worst case scenario

    """
    cases = ("*sort", "\\sort", "/sort", "~sort", "-sort", "!sort")
    fmt = ("%2s %6s" + " %6s"*len(cases))
    print fmt % (("i", "2**i") + cases)
    for i in r:
        n = 1<<i
        L = randrange(n)
        ##assert len(L) == n
        print "%2d %6d" % (i, n),
        fl()
        doit(L) # *sort
        L.reverse()
        doit(L) # \sort
        doit(L) # /sort
        if n > 4:
            del L[4:]
            L = L*(n/4)
            L = map(lambda x: --x, L)
        doit(L) # ~sort
        del L
        L = map(abs, [-0.5]*n)
        doit(L) # -sort
        L = range(n/2-1, -1, -1)
        L[len(L):] = range(n/2)
        doit(L) # !sort
        print

def main():
    """Main program when invoked as a script.

    One argument: tabulate a single row.
    Two arguments: tabulate a range (inclusive).
    Extra arguments are used to seed the random generator.

    """
    import string
    # default range (inclusive)
    k1 = 15
    k2 = 19
    if sys.argv[1:]:
        # one argument: single point
        k1 = k2 = string.atoi(sys.argv[1])
        if sys.argv[2:]:
            # two arguments: specify range
            k2 = string.atoi(sys.argv[2])
            if sys.argv[3:]:
                # derive random seed from remaining arguments
                x, y, z = 0, 0, 0
                for a in sys.argv[3:]:
                    h = hash(a)
                    h, d = divmod(h, 256)
                    h = h & 0xffffff
                    x = (x^h^d) & 255
                    h = h>>8
                    y = (y^h^d) & 255
                    h = h>>8
                    z = (z^h^d) & 255
                whrandom.seed(x, y, z)
    r = range(k1, k2+1)                 # include the end point
    tabulate(r)

if __name__ == '__main__':
    main()
