"""Usage: runtests.py [-q] [-r] [-v] [-u resources] [mask]

Run all tests found in this directory, and print a summary of the results.
Command line flags:
  -q     quiet mode: don't prnt anything while the tests are running
  -r     run tests repeatedly, look for refcount leaks
  -u<resources>
         Add resources to the lits of allowed resources. '*' allows all
         resources.
  -v     verbose mode: print the test currently executed
  mask   mask to select filenames containing testcases, wildcards allowed
"""
import sys
import ctypes.test

if __name__ == "__main__":
    sys.exit(ctypes.test.main(ctypes.test))
