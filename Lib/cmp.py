# Module 'cmp'

# Efficiently compare files, boolean outcome only (equal / not equal).

# Tricks (used in this order):
#	- Files with identical type, size & mtime are assumed to be clones
#	- Files with different type or size cannot be identical
#	- We keep a cache of outcomes of earlier comparisons
#	- We don't fork a process to run 'cmp' but read the files ourselves

import posix

cache = {}

def cmp(f1, f2): # Compare two files, use the cache if possible.
	# Return 1 for identical files, 0 for different.
	# Raise exceptions if either file could not be statted, read, etc.
	s1, s2 = sig(posix.stat(f1)), sig(posix.stat(f2))
	if s1[0] <> 8 or s2[0] <> 8:
		# Either is a not a plain file -- always report as different
		return 0
	if s1 = s2:
		# type, size & mtime match -- report same
		return 1
	if s1[:2] <> s2[:2]: # Types or sizes differ, don't bother
		# types or sizes differ -- report different
		return 0
	# same type and size -- look in the cache
	key = f1 + ' ' + f2
	try:
		cs1, cs2, outcome = cache[key]
		# cache hit
		if s1 = cs1 and s2 = cs2:
			# cached signatures match
			return outcome
		# stale cached signature(s)
	except RuntimeError:
		# cache miss
		pass
	# really compare
	outcome = do_cmp(f1, f2)
	cache[key] = s1, s2, outcome
	return outcome

def sig(st): # Return signature (i.e., type, size, mtime) from raw stat data
	# 0-5: st_mode, st_ino, st_dev, st_nlink, st_uid, st_gid
	# 6-9: st_size, st_atime, st_mtime, st_ctime
	type = st[0] / 4096
	size = st[6]
	mtime = st[8]
	return type, size, mtime

def do_cmp(f1, f2): # Compare two files, really
	bufsize = 8096 # Could be tuned
	fp1 = open(f1, 'r')
	fp2 = open(f2, 'r')
	while 1:
		b1 = fp1.read(bufsize)
		b2 = fp2.read(bufsize)
		if b1 <> b2: return 0
		if not b1: return 1
