import thread
# Start empty thread to initialize thread mechanics (and global lock!)
# This thread will finish immediately thus won't make much influence on
# test results by itself, only by that fact that it initializes global lock
thread.start_new_thread(lambda : 1, ())

import test.pystone
test.pystone.main()

