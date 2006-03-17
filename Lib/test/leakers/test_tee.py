
# Test case taken from test_itertools
# See http://mail.python.org/pipermail/python-dev/2005-November/058339.html
# When this is fixed remember to remove from LEAKY_TESTS in Misc/build.sh.

from itertools import tee

def leak():
    def fib():
        def yield_identity_forever(g):
            while 1:
                yield g
        def _fib():
            for i in yield_identity_forever(head):
                yield i
        head, tail, result = tee(_fib(), 3)
        return result

    x = fib()
    x.next()
