# A parallelized "find(1)" using the thread module.

# This demonstrates the use of a work queue and worker threads.
# It really does do more stats/sec when using multiple threads,
# although the improvement is only about 20-30 percent.

# I'm too lazy to write a command line parser for the full find(1)
# command line syntax, so the predicate it searches for is wired-in,
# see function selector() below.  (It currently searches for files with
# group or world write permission.)

# Usage: parfind.py [-w nworkers] [directory] ...
# Default nworkers is 4, maximum appears to be 8 (on Irix 4.0.2)


import sys
import getopt
import string
import time
import os
from stat import *
import thread


# Work queue class.  Usage:
#   wq = WorkQ()
#   wq.addwork(func, (arg1, arg2, ...)) # one or more calls
#   wq.run(nworkers)
# The work is done when wq.run() completes.
# The function calls executed by the workers may add more work.
# Don't use keyboard interrupts!

class WorkQ:

	# Invariants:

	# - busy and work are only modified when mutex is locked
	# - len(work) is the number of jobs ready to be taken
	# - busy is the number of jobs being done
	# - todo is locked iff there is no work and somebody is busy

	def __init__(self):
		self.mutex = thread.allocate()
		self.todo = thread.allocate()
		self.todo.acquire()
		self.work = []
		self.busy = 0

	def addwork(self, func, args):
		job = (func, args)
		self.mutex.acquire()
		self.work.append(job)
		self.mutex.release()
		if len(self.work) == 1:
			self.todo.release()

	def _getwork(self):
		self.todo.acquire()
		self.mutex.acquire()
		if self.busy == 0 and len(self.work) == 0:
			self.mutex.release()
			self.todo.release()
			return None
		job = self.work[0]
		del self.work[0]
		self.busy = self.busy + 1
		self.mutex.release()
		if len(self.work) > 0:
			self.todo.release()
		return job

	def _donework(self):
		self.mutex.acquire()
		self.busy = self.busy - 1
		if self.busy == 0 and len(self.work) == 0:
			self.todo.release()
		self.mutex.release()

	def _worker(self):
		time.sleep(0.00001)	# Let other threads run
		while 1:
			job = self._getwork()
			if not job:
				break
			func, args = job
			apply(func, args)
			self._donework()

	def run(self, nworkers):
		if not self.work:
			return # Nothing to do
		for i in range(nworkers-1):
			thread.start_new(self._worker, ())
		self._worker()
		self.todo.acquire()


# Main program

def main():
	sys.argv.append("/tmp")
	nworkers = 4
	opts, args = getopt.getopt(sys.argv[1:], '-w:')
	for opt, arg in opts:
		if opt == '-w':
			nworkers = string.atoi(arg)
	if not args:
		args = [os.curdir]

	wq = WorkQ()
	for dir in args:
		wq.addwork(find, (dir, selector, wq))

	t1 = time.time()
	wq.run(nworkers)
	t2 = time.time()

	sys.stderr.write('Total time ' + `t2-t1` + ' sec.\n')


# The predicate -- defines what files we look for.
# Feel free to change this to suit your purpose

def selector(dir, name, fullname, stat):
	# Look for group or world writable files
	return (stat[ST_MODE] & 0022) != 0


# The find procedure -- calls wq.addwork() for subdirectories

def find(dir, pred, wq):
	try:
		names = os.listdir(dir)
	except os.error, msg:
		print `dir`, ':', msg
		return
	for name in names:
		if name not in (os.curdir, os.pardir):
			fullname = os.path.join(dir, name)
			try:
				stat = os.lstat(fullname)
			except os.error, msg:
				print `fullname`, ':', msg
				continue
			if pred(dir, name, fullname, stat):
				print fullname
			if S_ISDIR(stat[ST_MODE]):
				if not os.path.ismount(fullname):
					wq.addwork(find, (fullname, pred, wq))


# Call the main program

main()
