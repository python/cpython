import gc

thingy = object()
class A(object):
    def f(self):
        return 1
    x = thingy

r = gc.get_referrers(thingy)
if "__module__" in r[0]:
    dct = r[0]
else:
    dct = r[1]

a = A()
for i in range(10):
    a.f()
dct["f"] = lambda self: 2

print(a.f()) # should print 1
