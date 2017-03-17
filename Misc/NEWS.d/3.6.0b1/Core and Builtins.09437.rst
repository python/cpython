Issue #27558: Fix a SystemError in the implementation of "raise" statement.
In a brand new thread, raise a RuntimeError since there is no active
exception to reraise. Patch written by Xiang Zhang.