# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

import sys
from test import test_support
import _testcapi

def test_main():

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if test_support.verbose:
                print("internal", name)
            test()

    # some extra thread-state tests driven via _testcapi
    def TestThreadState():
        if test_support.verbose:
            print("auto-thread-state")

        idents = []

        def callback():
            idents.append(thread.get_ident())

        _testcapi._test_thread_state(callback)
        a = b = callback
        time.sleep(1)
        # Check our main thread is in the list exactly 3 times.
        if idents.count(thread.get_ident()) != 3:
            raise test_support.TestFailed(
                        "Couldn't find main thread correctly in the list")

    try:
        _testcapi._test_thread_state
        have_thread_state = True
    except AttributeError:
        have_thread_state = False

    if have_thread_state:
        import thread
        import time
        TestThreadState()
        import threading
        t=threading.Thread(target=TestThreadState)
        t.start()
        t.join()

if __name__ == "__main__":
    test_main()
