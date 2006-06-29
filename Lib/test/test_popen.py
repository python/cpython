#! /usr/bin/env python
"""Basic tests for os.popen()

  Particularly useful for platforms that fake popen.
"""

import os
import sys
from test.test_support import TestSkipped, reap_children
from os import popen

# Test that command-lines get down as we expect.
# To do this we execute:
#    python -c "import sys;print sys.argv" {rest_of_commandline}
# This results in Python being spawned and printing the sys.argv list.
# We can then eval() the result of this, and see what each argv was.
python = sys.executable
if ' ' in python:
    python = '"' + python + '"'     # quote embedded space for cmdline
def _do_test_commandline(cmdline, expected):
    cmd = '%s -c "import sys;print sys.argv" %s' % (python, cmdline)
    data = popen(cmd).read()
    got = eval(data)[1:] # strip off argv[0]
    if got != expected:
        print "Error in popen commandline handling."
        print " executed '%s', expected '%r', but got '%r'" \
                                                    % (cmdline, expected, got)

def _test_commandline():
    _do_test_commandline("foo bar", ["foo", "bar"])
    _do_test_commandline('foo "spam and eggs" "silly walk"', ["foo", "spam and eggs", "silly walk"])
    _do_test_commandline('foo "a \\"quoted\\" arg" bar', ["foo", 'a "quoted" arg', "bar"])
    print "popen seemed to process the command-line correctly"

def main():
    print "Test popen:"
    _test_commandline()
    reap_children()

main()
