import _testinternalcapi
loops = 2**23
d = 1.0
for _ in range(10):
    _testinternalcapi.bench_api_A(loops, d)
    _testinternalcapi.bench_api_B(loops, d)
    _testinternalcapi.bench_api_C(loops, d)
