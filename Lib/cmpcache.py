# Module 'cmpcache'
#
# Efficiently compare files, boolean outcome only (equal / not equal).
#
# Tricks (used in this order):
#	- Use the statcache module to avoid statting files more than once
#	- Files with identical type, size & mtime are assumed to be clones
#	- Files with different type or size cannot be identical
#	- We keep a cache of outcomes of earlier comparisons
#	- We don't fork a process to run 'cmp' but read the files ourselves

import posix
from stat import *
import statcache


# The cache.
#
cache = {}


# Compare two files, use the cache if possible.
# May raise posix.error if a stat or open of either fails.
#
def cmp(f1, f2):
	# Return 1 for identical files, 0 for different.
	# Raise exceptions if either file could not be statted, read, etc.
	s1, s2 = sig(statcache.stat(f1)), sig(statcache.stat(f2))
	if not S_ISREG(s1[0]) or not S_ISREG(s2[0]):
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
	if cache.has_key(key):
		cs1, cs2, outcome = cache[key]
		# cache hit
		if s1 = cs1 and s2 = cs2:
			# cached signatures match
			return outcome
		# stale cached signature(s)
	# really compare
	outcome = do_cmp(f1, f2)
	cache[key] = s1, s2, outcome
	return outcome

# Return signature (i.e., type, size, mtime) from raw stat data.
#
def sig(st):
	return S_IFMT(st[ST_MODE]), st[ST_SIZE], st[ST_MTIME]

# Compare two files, really.
#
def do_cmp(f1, f2):
	#print '    cmp', f1, f2 # XXX remove when debugged
	bufsize = 8096 # Could be tuned
	fp1 = open(f1, 'r')
	fp2 = open(f2, 'r')
	while 1:
		b1 = fp1.read(bufsize)
		b2 = fp2.read(bufsize)
		if b1 <> b2: return 0
		if not b1: return 1
