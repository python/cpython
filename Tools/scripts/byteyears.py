#! /usr/local/python

# byteyears file ...
#
# Print a number representing the product of age and size of each file,
# in suitable units.

import sys, posix, time
from stat import *

secs_per_year = 365.0 * 24.0 * 3600.0
now = time.time()
status = 0

for file in sys.argv[1:]:
	try:
		st = posix.stat(file)
	except posix.error, msg:
		sys.stderr.write('can\'t stat ' + `file` + ': ' + `msg` + '\n')
		status = 1
		st = ()
	if st:
		mtime = st[ST_MTIME]
		size = st[ST_SIZE]
		age = now - mtime
		byteyears = float(size) * float(age) / secs_per_year
		print file + '\t\t' + `int(byteyears)`

sys.exit(status)
