"""This test checks for correct fork() behavior.

We want fork1() semantics -- only the forking thread survives in the
child after a fork().

On some systems (e.g. Solaris without posix threads) we find that all
active threads survive in the child after a fork(); this is an error.

While BeOS doesn't officially support fork and native threading in
the same application, the present example should work just fine.  DC
"""

import os, sys, time, thread
from test.test_support import verify, verbose, TestSkipped

try:
    os.fork
except AttributeError:
    raise TestSkipped, "os.fork not defined -- skipping test_fork1"

LONGSLEEP = 2

SHORTSLEEP = 0.5

NUM_THREADS = 4

alive = {}

stop = 0

def f(id):
    while not stop:
        alive[id] = os.getpid()
        try:
            time.sleep(SHORTSLEEP)
        except IOError:
            pass

def main():
    for i in range(NUM_THREADS):
        thread.start_new(f, (i,))

    time.sleep(LONGSLEEP)

    a = alive.keys()
    a.sort()
    verify(a == range(NUM_THREADS))

    prefork_lives = alive.copy()

    if sys.platform in ['unixware7']:
        cpid = os.fork1()
    else:
        cpid = os.fork()

    if cpid == 0:
        # Child
        time.sleep(LONGSLEEP)
        n = 0
        for key in alive.keys():
            if alive[key] != prefork_lives[key]:
                n = n+1
        os._exit(n)
    else:
        # Parent
        spid, status = os.waitpid(cpid, 0)
        verify(spid == cpid)
        verify(status == 0,
                "cause = %d, exit = %d" % (status&0xff, status>>8) )
        global stop
        # Tell threads to die
        stop = 1
        time.sleep(2*SHORTSLEEP) # Wait for threads to die

main()
