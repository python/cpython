#! /usr/local/python

# Variant of "which".
# On stderr, near and total misses are reported.

import sys, posix, string, path
from stat import *

def msg(str):
	sys.stderr.write(str + '\n')

pathlist = string.splitfields(posix.environ['PATH'], ':')

sts = 0

for prog in sys.argv[1:]:
	ident = ()
	for dir in pathlist:
		file = path.join(dir, prog)
		try:
			st = posix.stat(file)
			if S_ISREG(st[ST_MODE]):
				mode = S_IMODE(st[ST_MODE])
				if mode % 2 or mode/8 % 2 or mode/64 % 2:
					if ident:
						if st[:3] = ident:
							s = ': same as '
						else:
							s = ': also '
						msg(prog + s + file)
					else:
						print file
						ident = st[:3]
				else:
					msg(file + ': not executable')
			else:
				msg(file + ': not a disk file')
		except posix.error:
			pass
	if not ident:
		msg(prog + ': not found')
		sts = 1

sys.exit(sts)
