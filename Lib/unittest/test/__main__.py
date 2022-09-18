import os
import unittest


def load_tests(loader, standard_tests, pattern):
    # top level directory cached on loader instance
    this_dir = os.path.dirname(__file__)
    pattern = pattern or "test_*.py"
    # We are inside unittest.test, so the top-level is two notches up
    top_level_dir = os.path.dirname(os.path.dirname(this_dir))
    package_tests = loader.discover(start_dir=this_dir, pattern=pattern,
                                    top_level_dir=top_level_dir)
    standard_tests.addTests(package_tests)
    return standard_tests


if __name__ == '__main__':
    unittest.main()
