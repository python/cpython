# Bisection algorithms


# Insert item x in list a, and keep it sorted assuming a is sorted

def insort(a, x, lo=0, hi=None):
	if hi is None:
		hi = len(a)
        while lo < hi:
		mid = (lo+hi)/2
		if x < a[mid]: hi = mid
		else: lo = mid+1
	a.insert(lo, x)


# Find the index where to insert item x in list a, assuming a is sorted

def bisect(a, x, lo=0, hi=None):
	if hi is None:
		hi = len(a)
        while lo < hi:
		mid = (lo+hi)/2
		if x < a[mid]: hi = mid
		else: lo = mid+1
	return lo
