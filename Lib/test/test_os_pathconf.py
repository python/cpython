import os
import unittest

class TestPathconf(unittest.TestCase):
    def test_fpathconf_invalid_fd(self):
        """os.fpathconf(-1, name) must raise ValueError, not segfault."""
        # Use a valid name; 1 (PC_LINK_MAX) is guaranteed to exist on all platforms.
        with self.assertRaises(ValueError) as ctx:
            os.fpathconf(-1, 1)
        self.assertIn("file descriptor cannot be negative", str(ctx.exception))

    def test_pathconf_invalid_fd(self):
        """os.pathconf(-1, name) must raise ValueError, not segfault."""
        with self.assertRaises(ValueError) as ctx:
            os.pathconf(-1, 1)
        self.assertIn("file descriptor cannot be negative", str(ctx.exception))

if __name__ == "__main__":
    unittest.main()
