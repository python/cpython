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
# Create sys.argv
#
argvemulator.ArgvCollector().mainloop()
#
# Find the realy main program to run
#
_dir = os.path.split(sys.argv[0])[0]
__file__ = os.path.join(_dir, '__main__.py')
if os.path.exists(__file__):
	#
	# Setup something resembling a normal environment and go.
	#
	sys.argv[0] = __file__
	del argvemulator, os, sys, _dir
	execfile(__file__)
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
		exec __code__
	else:
		sys.stderr.write("%s: neither __main__.py nor __main__.pyc found\n"%sys.argv[0])
		sys.exit(1)
