# The following self-contained little program usually freezes with most
# threads reporting
# 
# Unhandled exception in thread:
# Traceback (innermost last):
#   File "importbug.py", line 6
#     x = whrandom.randint(1,3)
# AttributeError: randint
# 
# Here's the program; it doesn't use anything from the attached module:

import thread

def task():
    global N
    import whrandom
    x = whrandom.randint(1,3)
    a.acquire()
    N = N - 1
    if N == 0: done.release()
    a.release()

a = thread.allocate_lock()
done = thread.allocate_lock()
N = 10

done.acquire()
for i in range(N):
    thread.start_new_thread(task, ())
done.acquire()
print 'done'


# Sticking an acquire/release pair around the 'import' statement makes the
# problem go away.
# 
# I believe that what happens is:
# 
# 1) The first thread to hit the import atomically reaches, and executes
#    most of, get_module.  In particular, it finds Lib/whrandom.pyc,
#    installs its name in sys.modules, and executes
# 
#         v = eval_code(co, d, d, d, (object *)NULL);
# 
#    to initialize the module.
# 
# 2) eval_code "ticker"-slices the 1st thread out, and gives another thread
#    a chance.  When this 2nd thread hits the same 'import', import_module
#    finds 'whrandom' in sys.modules, so just proceeds.
# 
# 3) But the 1st thread is still "in the middle" of executing whrandom.pyc.
#    So the 2nd thread has a good chance of trying to look up 'randint'
#    before the 1st thread has placed it in whrandom's dict.
# 
# 4) The more threads there are, the more likely that at least one of them
#    will do this before the 1st thread finishes the import work.
# 
# If that's right, a perhaps not-too-bad workaround would be to introduce a
# static "you can't interrupt this thread" flag in ceval.c, check it before
# giving up interpreter_lock, and have IMPORT_NAME set it & restore (plain
# clearing would not work) it around its call to import_module.  To its
# credit, there's something wonderfully perverse about fixing a race via an
# unprotected static <grin>.
# 
# as-with-most-other-things-(pseudo-)parallel-programming's-more-fun-
#    in-python-too!-ly y'rs  - tim
# 
# Tim Peters   tim@ksr.com
# not speaking for Kendall Square Research Corp
