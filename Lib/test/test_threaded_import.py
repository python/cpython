# This is a variant of the very old (early 90's) file
# Demo/threads/bug.py.  It simply provokes a number of threads into
# trying to import the same module "at the same time".
# There are no pleasant failure modes -- most likely is that Python
# complains several times about module random having no attribute
# randrange, and then Python hangs.

import thread

critical_section = thread.allocate_lock()
done = thread.allocate_lock()

def task():
    global N, critical_section, done
    import random
    x = random.randrange(1, 3)
    critical_section.acquire()
    N -= 1
    if N == 0:
        done.release()
    critical_section.release()

# Tricky, tricky, tricky.
# When regrtest imports this module, the thread running regrtest grabs the
# import lock and won't let go of it until this module returns.  All other
# threads attempting an import hang for the duration.  So we have to spawn
# a thread to run the test and return to regrtest.py right away, else the
# test can't make progress.
#
# One miserable consequence:  This test can't wait to make sure all the
# threads complete!
#
# Another:  If this test fails, the output may show up while running
# some other test.
#
# Another:  If you run this test directly, the OS will probably kill
# all the threads right away, because the program exits immediately
# after spawning a thread to run the real test.
#
# Another:  If this test ever does fail and you attempt to run it by
# itself via regrtest, the same applies:  regrtest will get out so fast
# the OS will kill all the threads here.

def run_the_test():
    global N, done
    done.acquire()
    for N in [1, 2, 3, 4, 20, 4, 3, 2]:
        for i in range(N):
            thread.start_new_thread(task, ())
        done.acquire()

thread.start_new_thread(run_the_test, ())
