import time

def f():
    a = 1.0
    b = 2.0
    return a + b + a * a + b - a - b

# Warmup
f()
f()

# Running the actual benchmark

print("Starting benchmark...")
start = time.perf_counter()
for _ in range(100_000_000):
    f()
print("Time taken is", time.perf_counter() - start, "s")