import time

def f(a, b, loops=200_000_000):
    z = a + b
    for _ in range(loops):
        z + z + z + z + z + z + z + z + z + z + z + z + z + z + z + z 

# Warmup
f(1.0 , 2.0, 64)
f(1.0 , 2.0, 64)

# Running the actual benchmark

print("Starting benchmark...")
start = time.perf_counter()
f(1.0 , 2.0)
print("Time taken is", time.perf_counter() - start, "s")