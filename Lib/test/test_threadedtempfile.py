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
provoking a 2.0 failure under Linux.  Run the test alone to boost either
via cmdline switches:

-f  FILES_PER_THREAD (int)
-t  NUM_THREADS (int)
"""

NUM_THREADS = 20        # change w/ -t option
FILES_PER_THREAD = 50   # change w/ -f option

import thread # If this fails, we can't test this module
import threading
from test.test_support import TestFailed
import StringIO
from traceback import print_exc
import tempfile

startEvent = threading.Event()

class TempFileGreedy(threading.Thread):
    error_count = 0
    ok_count = 0

    def run(self):
        self.errors = StringIO.StringIO()
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

def test_main():
    threads = []

    print "Creating"
    for i in range(NUM_THREADS):
        t = TempFileGreedy()
        threads.append(t)
        t.start()

    print "Starting"
    startEvent.set()

    print "Reaping"
    ok = errors = 0
    for t in threads:
        t.join()
        ok += t.ok_count
        errors += t.error_count
        if t.error_count:
            print '%s errors:\n%s' % (t.getName(), t.errors.getvalue())

    msg = "Done: errors %d ok %d" % (errors, ok)
    print msg
    if errors:
        raise TestFailed(msg)


if __name__ == "__main__":
    import sys, getopt
    opts, args = getopt.getopt(sys.argv[1:], "t:f:")
    for o, v in opts:
        if o == "-f":
            FILES_PER_THREAD = int(v)
        elif o == "-t":
            NUM_THREADS = int(v)
    test_main()
