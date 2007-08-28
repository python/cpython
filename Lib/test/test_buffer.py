"""Unit tests for buffer objects.

For now, tests just new or changed functionality.

"""

import unittest
from test import test_support

class BufferTests(unittest.TestCase):

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing.
        s = "".join(chr(c) for c in list(range(255, -1, -1)))
        b = buffer(s)
        indices = (0, None, 1, 3, 19, 300, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip step 0 (invalid)
                for step in indices[1:]:
                    self.assertEqual(b[start:stop:step],
                                     s[start:stop:step])


def test_main():
    test_support.run_unittest(BufferTests)

if __name__ == "__main__":
    test_main()
