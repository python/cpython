# Very rudimentary test of threading module

# Create a bunch of threads, let each do some work, wait until all are done

from test_support import verbose
import random
import threading
import time

numtasks = 10

# no more than 3 of the 10 can run at once
sema = threading.BoundedSemaphore(value=3)
mutex = threading.RLock()
running = 0

class TestThread(threading.Thread):
    def run(self):
        global running
        delay = random.random() * numtasks
        if verbose:
            print 'task', self.getName(), 'will run for', round(delay, 1), 'sec'
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

print 'waiting for all tasks to complete'
for t in threads:
    t.join()
print 'all tasks done'

