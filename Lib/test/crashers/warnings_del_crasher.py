"""
Expose a race in the _warnings module, which is the C backend for the
warnings module. The "_warnings" module tries to access attributes of the
"warnings" module (because of the API it has to support), but doing so
during interpreter shutdown is problematic. Specifically, the call to
PyImport_GetModuleDict() in Python/_warnings.c:get_warnings_attr will
abort() if the modules dict has already been cleaned up.

This crasher is timing-dependent, and more threads (NUM_THREADS) may be
necessary to expose it reliably on different systems.
"""

import threading
import warnings

NUM_THREADS = 10

class WarnOnDel(object):
    def __del__(self):
        warnings.warn("oh no something went wrong", UserWarning)

def do_work():
    while True:
        w = WarnOnDel()

for i in range(NUM_THREADS):
    t = threading.Thread(target=do_work)
    t.setDaemon(1)
    t.start()
