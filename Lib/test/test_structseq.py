from test_support import vereq

import time

t = time.gmtime()
astuple = tuple(t)
vereq(len(t), len(astuple))
vereq(t, astuple)

# Check that slicing works the same way; at one point, slicing t[i:j] with
# 0 < i < j could produce NULLs in the result.
for i in range(-len(t), len(t)):
    for j in range(-len(t), len(t)):
        vereq(t[i:j], astuple[i:j])

XXX more needed
