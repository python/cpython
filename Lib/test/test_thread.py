# Very rudimentary test of thread module

# Create a bunch of threads, let each do some work, wait until all are done

import whrandom
import thread
import time

mutex = thread.allocate_lock()
running = 0
done = thread.allocate_lock()
done.acquire()

def task(ident):
	global running
	delay = whrandom.random() * 10
	print 'task', ident, 'will run for', delay, 'sec'
	time.sleep(delay)
	print 'task', ident, 'done'
	mutex.acquire()
	running = running - 1
	if running == 0:
		done.release()
	mutex.release()

next_ident = 0
def newtask():
	global next_ident, running
	mutex.acquire()
	next_ident = next_ident + 1
	print 'creating task', next_ident
	thread.start_new_thread(task, (next_ident,))
	running = running + 1
	mutex.release()

for i in range(10):
	newtask()

print 'waiting for all tasks to complete'
done.acquire()
print 'all tasks done'
