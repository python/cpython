# Very rudimentary test of threading module

# Create a bunch of threads, let each do some work, wait until all are done

from test.test_support import verbose
import dummy_threading as _threading
import time


class TestThread(_threading.Thread):

    def run(self):
        global running
        # Uncomment if testing another module, such as the real 'threading'
        # module.
        #delay = random.random() * 2
        delay = 0
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

def starttasks():
    for i in range(numtasks):
        t = TestThread(name="<thread %d>"%i)
        threads.append(t)
        t.start()


def test_main():
    # This takes about n/3 seconds to run (about n/3 clumps of tasks, times
    # about 1 second per clump).
    global numtasks
    numtasks = 10

    # no more than 3 of the 10 can run at once
    global sema
    sema = _threading.BoundedSemaphore(value=3)
    global mutex
    mutex = _threading.RLock()
    global running
    running = 0

    global threads
    threads = []

    starttasks()

    if verbose:
        print 'waiting for all tasks to complete'
    for t in threads:
        t.join()
    if verbose:
        print 'all tasks done'



if __name__ == '__main__':
    test_main()
