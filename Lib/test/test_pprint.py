from test_support import verify
import pprint

# Verify that .isrecursive() and .isreadable() work.

a = range(100)
b = range(200)
a[-12] = b

for safe in 2, 2.0, 2j, "abc", [3], (2,2), {3: 3}, u"yaddayadda", a, b:
    verify(pprint.isrecursive(safe) == 0, "expected isrecursive == 0")
    verify(pprint.isreadable(safe) == 1,  "expected isreadable == 1")

# Tie a knot.
b[67] = a
# Messy dict.
d = {}
d[0] = d[1] = d[2] = d

for icky in a, b, d, (d, d):
    verify(pprint.isrecursive(icky) == 1, "expected isrecursive == 1")
    verify(pprint.isreadable(icky) == 0,  "expected isreadable == 0")

# Break the cycles.
d.clear()
del a[:]
del b[:]

for safe in a, b, d, (d, d):
    verify(pprint.isrecursive(safe) == 0, "expected isrecursive == 0")
    verify(pprint.isreadable(safe) == 1,  "expected isreadable == 1")

# Not recursive but not readable anyway.
for unreadable in type(3), pprint, pprint.isrecursive:
    verify(pprint.isrecursive(unreadable) == 0, "expected isrecursive == 0")
    verify(pprint.isreadable(unreadable) ==0,  "expected isreadable == 0")
