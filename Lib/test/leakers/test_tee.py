
# Test case taken from test_generators
# See http://mail.python.org/pipermail/python-dev/2005-November/058339.html

from itertools import tee
import gc

def leak():
  def inner():
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
  inner()
  gc.collect() ; gc.collect()
  # this is expected to return 0
  return gc.collect()
