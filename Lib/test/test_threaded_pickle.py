import sys
import importlib
import os
import shutil
import threading
import unittest
from test.support import TESTFN, forget
import pickle

slow_import_module = """
import time

time.sleep(%(delay)s)

class ToBePickled(object):
    pass
"""


class ThreadedPickleTests(unittest.TestCase):
    def test_pickle_thread_imports(self):
        # https://bugs.python.org/issue34572
        delay = 0.01
        os.mkdir(TESTFN)
        self.addCleanup(shutil.rmtree, TESTFN)
        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)
        contents = slow_import_module % {'delay': delay}
        with open(os.path.join(TESTFN, "slowimport.py"), "wb") as f:
            f.write(contents.encode('utf-8'))
        self.addCleanup(forget, "slowimport")

        import slowimport
        obj = slowimport.ToBePickled()
        pickle_bytes = pickle.dumps(obj)
        del obj

        # Then try to unpickle two of these simultaneously
        def do_unpickle():
            results.append(pickle.loads(pickle_bytes))

        for repeat_test in range(10):
            results = []
            forget("slowimport")
            self.assertNotIn("slowimport", sys.modules)
            importlib.invalidate_caches()
            t1 = threading.Thread(target=do_unpickle)
            t2 = threading.Thread(target=do_unpickle)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
            self.assertEqual(len(results), 2)
