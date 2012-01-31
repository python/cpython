"""Benchmark some basic import use-cases.

The assumption is made that this benchmark is run in a fresh interpreter and
thus has no external changes made to import-related attributes in sys.

"""
from . import util
from .source import util as source_util
import decimal
import imp
import importlib
import json
import os
import py_compile
import sys
import timeit


def bench(name, cleanup=lambda: None, *, seconds=1, repeat=3):
    """Bench the given statement as many times as necessary until total
    executions take one second."""
    stmt = "__import__({!r})".format(name)
    timer = timeit.Timer(stmt)
    for x in range(repeat):
        total_time = 0
        count = 0
        while total_time < seconds:
            try:
                total_time += timer.timeit(1)
            finally:
                cleanup()
            count += 1
        else:
            # One execution too far
            if total_time > seconds:
                count -= 1
        yield count // seconds

def from_cache(seconds, repeat):
    """sys.modules"""
    name = '<benchmark import>'
    module = imp.new_module(name)
    module.__file__ = '<test>'
    module.__package__ = ''
    with util.uncache(name):
        sys.modules[name] = module
        for result in bench(name, repeat=repeat, seconds=seconds):
            yield result


def builtin_mod(seconds, repeat):
    """Built-in module"""
    name = 'errno'
    if name in sys.modules:
        del sys.modules[name]
    # Relying on built-in importer being implicit.
    for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                        seconds=seconds):
        yield result


def source_wo_bytecode(seconds, repeat):
    """Source w/o bytecode: simple"""
    sys.dont_write_bytecode = True
    try:
        name = '__importlib_test_benchmark__'
        # Clears out sys.modules and puts an entry at the front of sys.path.
        with source_util.create_modules(name) as mapping:
            assert not os.path.exists(imp.cache_from_source(mapping[name]))
            for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                                seconds=seconds):
                yield result
    finally:
        sys.dont_write_bytecode = False


def decimal_wo_bytecode(seconds, repeat):
    """Source w/o bytecode: decimal"""
    name = 'decimal'
    decimal_bytecode = imp.cache_from_source(decimal.__file__)
    if os.path.exists(decimal_bytecode):
        os.unlink(decimal_bytecode)
    sys.dont_write_bytecode = True
    try:
        for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                            seconds=seconds):
            yield result
    finally:
        sys.dont_write_bytecode = False


def source_writing_bytecode(seconds, repeat):
    """Source writing bytecode: simple"""
    assert not sys.dont_write_bytecode
    name = '__importlib_test_benchmark__'
    with source_util.create_modules(name) as mapping:
        def cleanup():
            sys.modules.pop(name)
            os.unlink(imp.cache_from_source(mapping[name]))
        for result in bench(name, cleanup, repeat=repeat, seconds=seconds):
            assert not os.path.exists(imp.cache_from_source(mapping[name]))
            yield result


def decimal_writing_bytecode(seconds, repeat):
    """Source writing bytecode: decimal"""
    assert not sys.dont_write_bytecode
    name = 'decimal'
    def cleanup():
        sys.modules.pop(name)
        os.unlink(imp.cache_from_source(decimal.__file__))
    for result in bench(name, cleanup, repeat=repeat, seconds=seconds):
        yield result


def source_using_bytecode(seconds, repeat):
    """Bytecode w/ source: simple"""
    name = '__importlib_test_benchmark__'
    with source_util.create_modules(name) as mapping:
        py_compile.compile(mapping[name])
        assert os.path.exists(imp.cache_from_source(mapping[name]))
        for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                            seconds=seconds):
            yield result


def decimal_using_bytecode(seconds, repeat):
    """Bytecode w/ source: decimal"""
    name = 'decimal'
    py_compile.compile(decimal.__file__)
    for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                        seconds=seconds):
        yield result


def main(import_, filename=None, benchmark=None):
    if filename and os.path.exists(filename):
        with open(filename, 'r') as file:
            prev_results = json.load(file)
    else:
        prev_results = {}
    __builtins__.__import__ = import_
    benchmarks = (from_cache, builtin_mod,
                  source_using_bytecode, source_wo_bytecode,
                  source_writing_bytecode,
                  decimal_using_bytecode, decimal_writing_bytecode,
                  decimal_wo_bytecode,)
    if benchmark:
        for b in benchmarks:
            if b.__doc__ == benchmark:
                benchmarks = [b]
                break
        else:
            print('Unknown benchmark: {!r}'.format(benchmark, file=sys.stderr))
            sys.exit(1)
    seconds = 1
    seconds_plural = 's' if seconds > 1 else ''
    repeat = 3
    header = ('Measuring imports/second over {} second{}, best out of {}\n'
              'Entire benchmark run should take about {} seconds\n')
    print(header.format(seconds, seconds_plural, repeat,
                        len(benchmarks) * seconds * repeat))
    new_results = {}
    for benchmark in benchmarks:
        print(benchmark.__doc__, "[", end=' ')
        sys.stdout.flush()
        results = []
        for result in benchmark(seconds=seconds, repeat=repeat):
            results.append(result)
            print(result, end=' ')
            sys.stdout.flush()
        assert not sys.dont_write_bytecode
        print("]", "best is", format(max(results), ',d'))
        new_results[benchmark.__doc__] = results
    prev_results[import_.__module__] = new_results
    if 'importlib._bootstrap' in prev_results and 'builtins' in prev_results:
        print('\n\nComparing importlib vs. __import__\n')
        importlib_results = prev_results['importlib._bootstrap']
        builtins_results = prev_results['builtins']
        for benchmark in benchmarks:
            benchmark_name = benchmark.__doc__
            importlib_result = max(importlib_results[benchmark_name])
            builtins_result = max(builtins_results[benchmark_name])
            result = '{:,d} vs. {:,d} ({:%})'.format(importlib_result,
                                                     builtins_result,
                                              importlib_result/builtins_result)
            print(benchmark_name, ':', result)
    if filename:
        with open(filename, 'w') as file:
            json.dump(prev_results, file, indent=2)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--builtin', dest='builtin', action='store_true',
                        default=False, help="use the built-in __import__")
    parser.add_argument('-f', '--file', dest='filename', default=None,
                        help='file to read/write results from/to'
                             '(incompatible w/ --benchmark)')
    parser.add_argument('--benchmark', dest='benchmark',
                        help='specific benchmark to run '
                             '(incompatible w/ --file')
    options = parser.parse_args()
    if options.filename and options.benchmark:
        print('Cannot specify a benchmark *and* read/write results')
        sys.exit(1)
    import_ = __import__
    if not options.builtin:
        import_ = importlib.__import__

    main(import_, options.filename, options.benchmark)
