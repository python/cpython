#! /usr/bin/env python
"""Test script for popen2.py
   Christian Tismer
"""

# popen2 contains its own testing routine
# which is especially useful to see if open files
# like stdin can be read successfully by a forked
# subprocess.

def main():
    try:
        from os import popen
    except ImportError:
        # if we don't have os.popen, check that
        # we have os.fork.  if not, skip the test
        # (by raising an ImportError)
        from os import fork
    import popen2
    popen2._test()

main()

