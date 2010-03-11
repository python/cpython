#!/usr/bin/env python3

# The above line should get replaced with the path to the Python
# interpreter; the block below should get 2to3-converted.

try:
    from test2to3.hello import hello
except ImportError, e:
    print "Import failed", e
hello()
