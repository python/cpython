"""
Create and delete FILES_PER_THREAD temp files (via tempfile.TemporaryFile)
in each of NUM_THREADS threads, recording the number of successes and
failures.  A failure is a bug in tempfile, and may be due to:

+ Trying to create more than one tempfile with the same name.
+ Trying to delete a tempfile that doesn't still exist.
+ Something we've never seen before.

By default, NUM_THREADS == 20 and FILES_PER_THREAD == 50.  This is enough to
create about 150 failures per run under Win98SE in 2.0, and runs pretty
quickly. Guido reports needing to boost FILES_PER_THREAD to 500 before
provoking a 2.0 failure under Linux.
"""

NUM_THREADS = 20
FILES_PER_THREAD = 50

import tempfile

from test.support import threading_setup, threading_cleanup, run_unittest, import_module
threading = import_module('threading')
import unittest
import io
from traceback import print_exc

startEvent = threading.Event()

class TempFileGreedy(threading.Thread):
    error_count = 0
    ok_count = 0

    def run(self):
        self.errors = io.StringIO()
        startEvent.wait()
        for i in range(FILES_PER_THREAD):
            try:
                f = tempfile.TemporaryFile("w+b")
                f.close()
            except:
                self.error_count += 1
                print_exc(file=self.errors)
            else:
                self.ok_count += 1


class ThreadedTempFileTest(unittest.TestCase):
    def test_main(self):
        threads = []
        thread_info = threading_setup()

        for i in range(NUM_THREADS):
            t = TempFileGreedy()
            threads.append(t)
            t.start()

        startEvent.set()

        ok = 0
        errors = []
        for t in threads:
            t.join()
            ok += t.ok_count
            if t.error_count:
                errors.append(str(t.name) + str(t.errors.getvalue()))

        threading_cleanup(*thread_info)

        msg = "Errors: errors %d ok %d\n%s" % (len(errors), ok,
            '\n'.join(errors))
        self.assertEqual(errors, [], msg)
        self.assertEqual(ok, NUM_THREADS * FILES_PER_THREAD)

def test_main():
    run_unittest(ThreadedTempFileTest)

if __name__ == "__main__":
    test_main()
