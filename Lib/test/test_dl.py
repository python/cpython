#! /usr/bin/env python
"""Test dlmodule.c
   Roger E. Masse
"""
filename = '/usr/lib/libresolv.so'
try:
    import dl
except ImportError:
    # No test if no library
    raise SystemExit

try:
    import os
    n = os.popen('/bin/uname','r')
    if n.readlines()[0][:-1] != 'SunOS':
	raise SystemExit
    l = dl.open('/usr/lib/libresolv.so')
except:
    # No test if not SunOS (or Solaris)
    raise SystemExit

# Try to open a shared library that should be available
# on SunOS and Solaris in a default place
try:
    open(filename,'r')
except IOError:
    # No test if I can't even open the test file with builtin open
    raise SystemExit

l = dl.open(filename)
a = l.call('gethostent')
l.close()
