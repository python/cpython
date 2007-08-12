# Emulate sys.argv and run __main__.py or __main__.pyc in an environment that
# is as close to "normal" as possible.
#
# This script is put into __rawmain__.pyc for applets that need argv
# emulation, by BuildApplet and friends.
#
import argvemulator
import os
import sys
import marshal

#
# Make sure we have an argv[0], and make _dir point to the Resources
# directory.
#
if not sys.argv or sys.argv[0][:1] == '-':
    # Insert our (guessed) name.
    _dir = os.path.split(sys.executable)[0] # removes "python"
    _dir = os.path.split(_dir)[0] # Removes "MacOS"
    _dir = os.path.join(_dir, 'Resources')
    sys.argv.insert(0, '__rawmain__')
else:
    _dir = os.path.split(sys.argv[0])[0]
#
# Add the Resources directory to the path. This is where files installed
# by BuildApplet.py with the --extra option show up, and if those files are
# modules this sys.path modification is necessary to be able to import them.
#
sys.path.insert(0, _dir)
#
# Create sys.argv
#
argvemulator.ArgvCollector().mainloop()
#
# Find the real main program to run
#
__file__ = os.path.join(_dir, '__main__.py')
if os.path.exists(__file__):
    #
    # Setup something resembling a normal environment and go.
    #
    sys.argv[0] = __file__
    del argvemulator, os, sys, _dir
    exec(open(__file__).read())
else:
    __file__ = os.path.join(_dir, '__main__.pyc')
    if os.path.exists(__file__):
        #
        # If we have only a .pyc file we read the code object from that
        #
        sys.argv[0] = __file__
        _fp = open(__file__, 'rb')
        _fp.read(8)
        __code__ = marshal.load(_fp)
        #
        # Again, we create an almost-normal environment (only __code__ is
        # funny) and go.
        #
        del argvemulator, os, sys, marshal, _dir, _fp
        exec(__code__)
    else:
        sys.stderr.write("%s: neither __main__.py nor __main__.pyc found\n"%sys.argv[0])
        sys.exit(1)
