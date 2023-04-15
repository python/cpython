import time

def f(a, b):
    for _ in range(10_000_000):
        return a + b + a * a + b - a - b

# Warmup
f(1.0, 2.0)
f(1.0, 2.0)

# Running the actual benchmark

print("Starting benchmark...")
start = time.perf_counter()
for _ in range(100_000_000):
    f(1.0, 2.0)
print("Time taken is", time.perf_counter() - start, "s")
