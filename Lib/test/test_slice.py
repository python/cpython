# tests for slice objects; in particular the indices method.

from test_support import vereq

vereq(slice(None           ).indices(10), (0, 10,  1))
vereq(slice(None,  None,  2).indices(10), (0, 10,  2))
vereq(slice(1,     None,  2).indices(10), (1, 10,  2))
vereq(slice(None,  None, -1).indices(10), (9, -1, -1))
vereq(slice(None,  None, -2).indices(10), (9, -1, -2))
vereq(slice(3,     None, -2).indices(10), (3, -1, -2))
vereq(slice(-100,  100     ).indices(10), slice(None).indices(10))
vereq(slice(100,  -100,  -1).indices(10), slice(None, None, -1).indices(10))
vereq(slice(-100L, 100L, 2L).indices(10), (0, 10,  2))

