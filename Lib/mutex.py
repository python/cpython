# Mutual exclusion -- for use with module sched

# A mutex has two pieces of state -- a 'locked' bit and a queue.
# When the mutex is not locked, the queue is empty.
# Otherwise, the queue contains 0 or more (function, argument) pairs
# representing functions (or methods) waiting to acquire the lock.
# When the mutex is unlocked while the queue is not empty,
# the first queue entry is removed and its function(argument) pair called,
# implying it now has the lock.
#
# Of course, no multi-threading is implied -- hence the funny interface
# for lock, where a function is called once the lock is aquired.
#
class mutex:
	#
	# Create a new mutex -- initially unlocked
	#
	def init(self):
		self.locked = 0
		self.queue = []
		return self
	#
	# Test the locked bit of the mutex
	#
	def test(self):
		return self.locked
	#
	# Atomic test-and-set -- grab the lock if it is not set,
	# return true if it succeeded
	#
	def testandset(self):
		if not self.locked:
			self.locked = 1
			return 1
		else:
			return 0
	#
	# Lock a mutex, call the function with supplied argument
	# when it is acquired.
	# If the mutex is already locked, place function and argument
	# in the queue.
	#
	def lock(self, (function, argument)):
		if self.testandset():
			function(argument)
		else:
			self.queue.append(function, argument)
	#
	# Unlock a mutex.  If the queue is not empty, call the next
	# function with its argument.
	#
	def unlock(self):
		if self.queue:
			function, argument = self.queue[0]
			del self.queue[0]
			function(argument)
		else:
			self.locked = 0
	#
