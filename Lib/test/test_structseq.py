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

# Devious code could crash structseqs' contructors
class C:
    def __getitem__(self, i):
        raise IndexError
    def __len__(self):
        return 9

try:
    repr(time.struct_time(C()))
except:
    pass

# XXX more needed
