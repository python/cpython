import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

# The C accelerator keeps text delivered by expat in several chunks as a
# list and joins it on first access.
ET = import_helper.import_fresh_module('xml.etree.ElementTree',
                                       fresh=['_elementtree'])

NTHREADS = 8
NITEMS = 500

# Each item text arrives as three chunks ('name ', '&', ' N') and each
# tail as two ('\n', '  ').
DOC = ("<root>\n" +
       "".join(f"  <item>name &amp; {i}</item>\n" for i in range(NITEMS)) +
       "  </root>\n")


@threading_helper.requires_working_threading()
@unittest.skipIf(ET is None, 'requires _elementtree')
class TestElementConcurrentReads(unittest.TestCase):
    def check(self, worker, nrounds=20):
        for _ in range(nrounds):
            root = ET.fromstring(DOC)
            run_concurrently(worker_func=worker, nthreads=NTHREADS,
                             args=(root,))

    def test_concurrent_text_tail(self):
        def worker(root):
            for i, item in enumerate(root):
                self.assertEqual(item.text, f"name & {i}")
                self.assertEqual(item.tail, "\n  ")

        self.check(worker)

    def test_concurrent_tostring(self):
        expected = ET.tostring(ET.fromstring(DOC))

        def worker(root):
            self.assertEqual(ET.tostring(root), expected)

        self.check(worker)


if __name__ == "__main__":
    unittest.main()
