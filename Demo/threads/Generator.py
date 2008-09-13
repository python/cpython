# Generator implementation using threads

import sys
import thread

class Killed(Exception):
    pass

class Generator:
    # Constructor
    def __init__(self, func, args):
        self.getlock = thread.allocate_lock()
        self.putlock = thread.allocate_lock()
        self.getlock.acquire()
        self.putlock.acquire()
        self.func = func
        self.args = args
        self.done = 0
        self.killed = 0
        thread.start_new_thread(self._start, ())

    # Internal routine
    def _start(self):
        try:
            self.putlock.acquire()
            if not self.killed:
                try:
                    apply(self.func, (self,) + self.args)
                except Killed:
                    pass
        finally:
            if not self.killed:
                self.done = 1
                self.getlock.release()

    # Called by producer for each value; raise Killed if no more needed
    def put(self, value):
        if self.killed:
            raise TypeError, 'put() called on killed generator'
        self.value = value
        self.getlock.release()  # Resume consumer thread
        self.putlock.acquire()  # Wait for next get() call
        if self.killed:
            raise Killed

    # Called by producer to get next value; raise EOFError if no more
    def get(self):
        if self.killed:
            raise TypeError, 'get() called on killed generator'
        self.putlock.release()  # Resume producer thread
        self.getlock.acquire()  # Wait for value to appear
        if self.done:
            raise EOFError  # Say there are no more values
        return self.value

    # Called by consumer if no more values wanted
    def kill(self):
        if self.killed:
            raise TypeError, 'kill() called on killed generator'
        self.killed = 1
        self.putlock.release()

    # Clone constructor
    def clone(self):
        return Generator(self.func, self.args)

def pi(g):
    k, a, b, a1, b1 = 2L, 4L, 1L, 12L, 4L
    while 1:
        # Next approximation
        p, q, k = k*k, 2L*k+1L, k+1L
        a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
        # Print common digits
        d, d1 = a//b, a1//b1
        while d == d1:
            g.put(int(d))
            a, a1 = 10L*(a%b), 10L*(a1%b1)
            d, d1 = a//b, a1//b1

def test():
    g = Generator(pi, ())
    g.kill()
    g = Generator(pi, ())
    for i in range(10): print g.get(),
    print
    h = g.clone()
    g.kill()
    while 1:
        print h.get(),
        sys.stdout.flush()

test()
