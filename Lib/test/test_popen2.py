#! /usr/bin/env python
"""Test script for popen2.py
   Christian Tismer
"""

# popen2 contains its own testing routine
# which is especially useful to see if open files
# like stdin can be read successfully by a forked 
# subprocess.

def main():
    from os import fork # skips test through ImportError
    import popen2
    popen2._test()

main()

