# test the invariant that
#   iff a==b then hash(a)==hash(b)
#

import test_support


def same_hash(*objlist):
	# hash each object given an raise TestFailed if
	# the hash values are not all the same
	hashed = map(hash, objlist)
	for h in hashed[1:]:
		if h != hashed[0]:
			raise TestFailed, "hashed values differ: %s" % `objlist`



same_hash(1, 1L, 1.0, 1.0+0.0j)
same_hash(int(1), long(1), float(1), complex(1))

same_hash(long(1.23e300), float(1.23e300))

same_hash(float(0.5), complex(0.5, 0.0))



