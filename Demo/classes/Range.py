# Example of a generator: re-implement the built-in range function
# without actually constructing the list of values.  (It turns out
# that the built-in function is about 20 times faster -- that's why
# it's built-in. :-)


# Wrapper function to emulate the complicated range() arguments

def range(*a):
	if len(a) == 1:
		start, stop, step = 0, a[0], 1
	elif len(a) == 2:
		start, stop = a
		step = 1
	elif len(a) == 3:
		start, stop, step = a
	else:
		raise TypeError, 'range() needs 1-3 arguments'
	return Range(start, stop, step)
	

# Class implementing a range object.
# To the user the instances feel like immutable sequences
# (and you can't concatenate or slice them)

class Range:

	# initialization -- should be called only by range() above
	def __init__(self, start, stop, step):
		if step == 0:
			raise ValueError, 'range() called with zero step'
		self.start = start
		self.stop = stop
		self.step = step
		self.len = max(0, int((self.stop - self.start) / self.step))

	# implement `x` and is also used by print x
	def __repr__(self):
		return 'range' + `self.start, self.stop, self.step`

	# implement len(x)
	def __len__(self):
		return self.len

	# implement x[i]
	def __getitem__(self, i):
		if 0 <= i < self.len:
			return self.start + self.step * i
		else:
			raise IndexError, 'range[i] index out of range'


# Small test program

def test():
	import time, __builtin__
	print range(10), range(-10, 10), range(0, 10, 2)
	for i in range(100, -100, -10): print i,
	print
	t1 = time.time()
	for i in range(1000):
		pass
	t2 = time.time()
	for i in __builtin__.range(1000):
		pass
	t3 = time.time()
	print t2-t1, 'sec (class)'
	print t3-t2, 'sec (built-in)'


test()
