"""Compare files."""

import os, stat, statcache

_cache = {}
BUFSIZE=8*1024

def cmp(f1, f2, shallow=1,use_statcache=0): 
	"""Compare two files.

	Arguments:

	f1 -- First file name

	f2 -- Second file name

	shallow -- Just check stat signature (do not read the files).
	           defaults to 1.

	use_statcache -- Do not stat() each file directly: go through
	                 the statcache module for more efficiency.

	Return value:

	integer -- 1 if the files are the same, 0 otherwise.

	This function uses a cache for past comparisons and the results,
	with a cache invalidation mechanism relying on stale signatures.
	Of course, if 'use_statcache' is true, this mechanism is defeated,
	and the cache will never grow stale.

	"""
	stat_function = (os.stat, statcache.stat)[use_statcache]
	s1, s2 = _sig(stat_function(f1)), _sig(stat_function(f2))
	if s1[0]!=stat.S_IFREG or s2[0]!=stat.S_IFREG: return 0
	if shallow and s1 == s2: return 1
	if s1[1]!=s2[1]:         return 0

	result = _cache.get((f1, f2))
	if result and (s1, s2)==result[:2]:
		return result[2]
	outcome = _do_cmp(f1, f2)
	_cache[f1, f2] = s1, s2, outcome
	return outcome

def _sig(st):
	return (stat.S_IFMT(st[stat.ST_MODE]), 
	        st[stat.ST_SIZE], 
	        st[stat.ST_MTIME])

def _do_cmp(f1, f2):
	bufsize = BUFSIZE
	fp1 , fp2 = open(f1, 'rb'), open(f2, 'rb')
	while 1:
		b1, b2 = fp1.read(bufsize), fp2.read(bufsize)
		if b1!=b2: return 0
		if not b1: return 1
