import functools
from test.support import threading_helper

@functools.singledispatch
def g(x):
    return 0

errors = []
def churn():
    try:
        for i in range(400):
            T = type(f"X{i}", (), {})   # fresh type; dies next iter -> WeakKeyDict removal callback
            g(T())                       # dispatch_cache[T]=impl races with concurrent _commit_removals
    except Exception as e:
        errors.append(repr(e))

for _ in range(40):
    threading_helper.run_concurrently(churn, nthreads=16)
    if errors:
        break
print("CHURN_DONE errors=", len(errors), errors[:5])
