Issue #25274: sys.setrecursionlimit() now raises a RecursionError if the new
recursion limit is too low depending at the current recursion depth. Modify
also the "lower-water mark" formula to make it monotonic. This mark is used
to decide when the overflowed flag of the thread state is reset.