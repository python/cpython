import unittest
import test.test_support

mutex = test.test_support.import_module("mutex", deprecated=True)

class MutexTest(unittest.TestCase):

    def test_lock_and_unlock(self):

        def called_by_mutex(some_data):
            self.assertEqual(some_data, "spam")
            self.assert_(m.test(), "mutex not held")
            # Nested locking
            m.lock(called_by_mutex2, "eggs")

        def called_by_mutex2(some_data):
            self.assertEquals(some_data, "eggs")
            self.assert_(m.test(), "mutex not held")
            self.assert_(ready_for_2,
                         "called_by_mutex2 called too soon")

        m = mutex.mutex()
        read_for_2 = False
        m.lock(called_by_mutex, "spam")
        ready_for_2 = True
        # unlock both locks
        m.unlock()
        m.unlock()
        self.failIf(m.test(), "mutex still held")

def test_main():
    test.test_support.run_unittest(MutexTest)

if __name__ == "__main__":
    test_main()
