#! /usr/bin/env python

"""Really nasty little script to create an empty, labeled index on stdout.

Do it this way since some shells seem to work badly (and differently) with
the leading '\b' for the first output line.  Specifically, /bin/sh on
Solaris doesn't seem to get it right.  Once the quoting works there, it
doesn't work on Linux any more.  ;-(
"""
__version__ = '$Revision$'
#  $Source$

import sys

if sys.argv[1:]:
    label = sys.argv[1]
else:
    label = "genindex"

print "\\begin{theindex}"
print "\\label{%s}" % label
print "\\end{theindex}"
