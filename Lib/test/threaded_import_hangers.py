# This is a helper module for test_threaded_import.  The test imports this
# module, and this module tries to run various Python library functions in
# their own thread, as a side effect of being imported.  If the spawned
# thread doesn't complete in TIMEOUT seconds, an "appeared to hang" message
# is appended to the module-global `errors` list.  That list remains empty
# if (and only if) all functions tested complete.

TIMEOUT = 10

import threading

import tempfile
import os.path

errors = []

# This class merely runs a function in its own thread T.  The thread importing
# this module holds the import lock, so if the function called by T tries
# to do its own imports it will block waiting for this module's import
# to complete.
class Worker(threading.Thread):
    def __init__(self, function, args):
        threading.Thread.__init__(self)
        self.function = function
        self.args = args

    def run(self):
        self.function(*self.args)

for name, func, args in [
        # Bug 147376:  TemporaryFile hung on Windows, starting in Python 2.4.
        ("tempfile.TemporaryFile", tempfile.TemporaryFile, ()),

        # The real cause for bug 147376:  ntpath.abspath() caused the hang.
        ("os.path.abspath", os.path.abspath, ('.',)),
        ]:

    t = Worker(func, args)
    t.start()
    t.join(TIMEOUT)
    if t.isAlive():
        errors.append("%s appeared to hang" % name)
