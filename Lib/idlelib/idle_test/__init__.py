"""idlelib.idle_test implements test.test_idle, which tests the IDLE
application as part of the stdlib test suite.
Run IDLE tests alone with "python -m test.test_idle (-v)".

This package and its contained modules are subject to change and
any direct use is at your own risk.
"""
from os.path import dirname

# test_idle imports load_tests for test discovery (default all).
# To run subsets of idlelib module tests, insert '[<chars>]' after '_'.
# Example: insert '[ac]' for modules beginning with 'a' or 'c'.
# Additional .discover/.addTest pairs with separate inserts work.
# Example: pairs with 'c' and 'g' test c* files and grep.

def load_tests(loader, standard_tests, pattern):
    this_dir = dirname(__file__)
    top_dir = dirname(dirname(this_dir))
    module_tests = loader.discover(start_dir=this_dir,
                                    pattern='test_*.py',  # Insert here.
                                    top_level_dir=top_dir)
    standard_tests.addTests(module_tests)
##    module_tests = loader.discover(start_dir=this_dir,
##                                    pattern='test_*.py',  # Insert here.
##                                    top_level_dir=top_dir)
##    standard_tests.addTests(module_tests)
    return standard_tests
