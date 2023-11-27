import pyperf
import _testinternalcapi
runner = pyperf.Runner()
d = 1.0
runner.bench_time_func('bench_api_A', _testinternalcapi.bench_api_A, d)
runner.bench_time_func('bench_api_B', _testinternalcapi.bench_api_B, d)
runner.bench_time_func('bench_api_C', _testinternalcapi.bench_api_C, d)
