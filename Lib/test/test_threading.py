# Very rudimentary test of threading module

# Create a bunch of threads, let each do some work, wait until all are done

from test_support import verbose
import random
import threading
import time

# This takes about n/3 seconds to run (about n/3 clumps of tasks, times
# about 1 second per clump).
numtasks = 10

# no more than 3 of the 10 can run at once
sema = threading.BoundedSemaphore(value=3)
mutex = threading.RLock()
running = 0

class TestThread(threading.Thread):
    def run(self):
        global running
        delay = random.random() * 2
        if verbose:
            print 'task', self.getName(), 'will run for', delay, 'sec'
        sema.acquire()
        mutex.acquire()
        running = running + 1
        if verbose:
            print running, 'tasks are running'
        mutex.release()
        time.sleep(delay)
        if verbose:
            print 'task', self.getName(), 'done'
        mutex.acquire()
        running = running - 1
        if verbose:
            print self.getName(), 'is finished.', running, 'tasks are running'
        mutex.release()
        sema.release()

threads = []
def starttasks():
    for i in range(numtasks):
        t = TestThread(name="<thread %d>"%i)
        threads.append(t)
        t.start()

starttasks()

if verbose:
    print 'waiting for all tasks to complete'
for t in threads:
    t.join()
if verbose:
    print 'all tasks done'
