# f.close() is not thread-safe: calling it at the same time as another
# operation (or another close) on the same file, but done from another
# thread, causes crashes.  The issue is more complicated than it seems,
# witness the discussions in:
#
# http://bugs.python.org/issue595601
# http://bugs.python.org/issue815646

import thread

while 1:
    f = open("multithreaded_close.tmp", "w")
    thread.start_new_thread(f.close, ())
    f.close()
