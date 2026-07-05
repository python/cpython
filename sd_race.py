import functools
from test.support import threading_helper

# singledispatch caches the resolved impl per argument type in an unsynchronized
# WeakKeyDictionary (dispatch_cache) with a plain nonlocal cache_token.  On the
# free-threaded build, concurrent first dispatches race the cache fill/read.
# Re-arm each round by clearing the cache, then race the refill across threads.

@functools.singledispatch
def g(x):
    return 0

TYPES = [type(f"T{i}", (), {}) for i in range(64)]
INSTANCES = [T() for T in TYPES]

errors = []

def hammer():
    try:
        for obj in INSTANCES:
            g(obj)          # dispatch on type(obj) -> cache miss/fill/read
    except Exception as e:
        errors.append(repr(e))

for _ in range(60):
    g._clear_cache()        # empty cache -> next round all threads race the fill
    threading_helper.run_concurrently(hammer, nthreads=16)
    if errors:
        break

print("ROUNDS_DONE errors=", len(errors), errors[:3])
