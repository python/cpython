This directory contains test cases that are known to leak references.
The idea is that you can import these modules while in the interpreter
and call the leak function repeatedly.  This will only be helpful if
the interpreter was built in debug mode.  If the total ref count
doesn't increase, the bug has been fixed and the file should be removed
from the repository.

Note:  be careful to check for cyclic garbage.  Sometimes it may be helpful
to define the leak function like:

def leak():
    def inner_leak():
        # this is the function that leaks, but also creates cycles
    inner_leak()
    gc.collect() ; gc.collect() ; gc.collect()

Here's an example interpreter session for test_gestalt which still leaks:

>>> from test.leakers.test_gestalt import leak
[24275 refs]
>>> leak()
[28936 refs]
>>> leak()
[28938 refs]
>>> leak()
[28940 refs]
>>> 

Once the leak is fixed, the test case should be moved into an appropriate
test (even if it was originally from the test suite).  This ensures the
regression doesn't happen again.  And if it does, it should be easier
to track down.
