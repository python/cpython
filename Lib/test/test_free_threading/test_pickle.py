import pickle
import threading
import unittest

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestPickleFreeThreading(unittest.TestCase):

    def test_pickle_dumps_with_concurrent_dict_mutation(self):
        # gh-146452: Pickling a dict while another thread mutates it
        # used to segfault. batch_dict_exact() iterated dict items via
        # PyDict_Next() which returns borrowed references, and a
        # concurrent pop/replace could free the value before Py_INCREF
        # got to it.
        shared = {str(i): list(range(20)) for i in range(50)}

        def dumper():
            for _ in range(1000):
                try:
                    pickle.dumps(shared)
                except RuntimeError:
                    # "dictionary changed size during iteration" is expected
                    pass

        def mutator():
            for j in range(1000):
                key = str(j % 50)
                shared[key] = list(range(j % 20))
                if j % 10 == 0:
                    shared.pop(key, None)
                    shared[key] = [j]

        threads = []
        for _ in range(10):
            threads.append(threading.Thread(target=dumper))
        threads.append(threading.Thread(target=mutator))

        for t in threads:
            t.start()
        for t in threads:
            t.join()

        # If we get here without a segfault, the test passed.


if __name__ == "__main__":
    unittest.main()
