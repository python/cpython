# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

import sys
import unittest
from test import support
import _testcapi

def testfunction(self):
    """some doc"""
    return self

class InstanceMethod:
    id = _testcapi.instancemethod(id)
    testfunction = _testcapi.instancemethod(testfunction)

class CAPITest(unittest.TestCase):

    def test_instancemethod(self):
        inst = InstanceMethod()
        self.assertEqual(id(inst), inst.id())
        self.assert_(inst.testfunction() is inst)
        self.assertEqual(inst.testfunction.__doc__, testfunction.__doc__)
        self.assertEqual(InstanceMethod.testfunction.__doc__, testfunction.__doc__)

        InstanceMethod.testfunction.attribute = "test"
        self.assertEqual(testfunction.attribute, "test")
        self.assertRaises(AttributeError, setattr, inst.testfunction, "attribute", "test")


def test_main():
    support.run_unittest(CAPITest)

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if support.verbose:
                print("internal", name)
            test()

    # some extra thread-state tests driven via _testcapi
    def TestThreadState():
        if support.verbose:
            print("auto-thread-state")

        idents = []

        def callback():
            idents.append(_thread.get_ident())

        _testcapi._test_thread_state(callback)
        a = b = callback
        time.sleep(1)
        # Check our main thread is in the list exactly 3 times.
        if idents.count(_thread.get_ident()) != 3:
            raise support.TestFailed(
                        "Couldn't find main thread correctly in the list")

    try:
        _testcapi._test_thread_state
        have_thread_state = True
    except AttributeError:
        have_thread_state = False

    if have_thread_state:
        import _thread
        import time
        TestThreadState()
        import threading
        t = threading.Thread(target=TestThreadState)
        t.start()
        t.join()


if __name__ == "__main__":
    test_main()
