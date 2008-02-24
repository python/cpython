import mutex

import unittest
import test.test_support

class MutexTest(unittest.TestCase):

    def setUp(self):
        self.mutex = mutex.mutex()

    def called_by_mutex(self, some_data):
        self.assert_(self.mutex.test(), "mutex not held")
        # Nested locking
        self.mutex.lock(self.called_by_mutex2, "eggs")

    def called_by_mutex2(self, some_data):
        self.assert_(self.ready_for_2,
                     "called_by_mutex2 called too soon")

    def test_lock_and_unlock(self):
        self.read_for_2 = False
        self.mutex.lock(self.called_by_mutex, "spam")
        self.ready_for_2 = True
        # unlock both locks
        self.mutex.unlock()
        self.mutex.unlock()
        self.failIf(self.mutex.test(), "mutex still held")

def test_main():
    test.test_support.run_unittest(MutexTest)

if __name__ == "__main__":
    test_main()
