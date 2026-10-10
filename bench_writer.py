import pyperf, _testcapi, functools
runner = pyperf.Runner()
bench = lambda n: functools.partial(_testcapi.bench_writer, n)
#for n in (3, 16, 64, 255, 300):
#    runner.bench_time_func(f'bench({n})', bench(n))
runner.bench_time_func(f'bench(3)', bench(3))
