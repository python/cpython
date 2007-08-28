# An example for http://bugs.python.org/issue815646

import thread

while 1:
    f = open("/tmp/dupa", "w")
    thread.start_new_thread(f.close, ())
    f.close()
