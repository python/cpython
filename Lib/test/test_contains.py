from test_support import TestFailed

class base_set:

	def __init__(self, el):
		self.el = el

class set(base_set):

	def __contains__(self, el):
		return self.el == el

class seq(base_set):

	def __getitem__(self, n):
		return [self.el][n]

def check(ok, *args):
    if not ok:
        raise TestFailed, join(map(str, args), " ")

a = base_set(1)
b = set(1)
c = seq(1)

check(1 in b, "1 not in set(1)")
check(0 not in b, "0 in set(1)")
check(1 in c, "1 not in seq(1)")
check(0 not in c, "0 in seq(1)")

try:
	1 in a
	check(0, "in base_set did not raise error")
except AttributeError:
	pass

try:
	1 not in a
	check(0, "not in base_set did not raise error")
except AttributeError:
	pass
