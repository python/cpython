"""Benchmark some basic import use-cases.

The assumption is made that this benchmark is run in a fresh interpreter and
thus has no external changes made to import-related attributes in sys.

"""
from test.test_importlib import util
import decimal
from importlib.util import cache_from_source
import importlib
import importlib.machinery
import json
import os
import py_compile
import sys
import tabnanny
import timeit
import types


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
    module = types.ModuleType(name)
    module.__file__ = '<test>'
    module.__package__ = ''
    with util.uncache(name):
        sys.modules[name] = module
        yield from bench(name, repeat=repeat, seconds=seconds)


def builtin_mod(seconds, repeat):
    """Built-in module"""
    name = 'errno'
    if name in sys.modules:
        del sys.modules[name]
    # Relying on built-in importer being implicit.
    yield from bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                     seconds=seconds)


def source_wo_bytecode(seconds, repeat):
    """Source w/o bytecode: small"""
    sys.dont_write_bytecode = True
    try:
        name = '__importlib_test_benchmark__'
        # Clears out sys.modules and puts an entry at the front of sys.path.
        with util.create_modules(name) as mapping:
            assert not os.path.exists(cache_from_source(mapping[name]))
            sys.meta_path.append(importlib.machinery.PathFinder)
            loader = (importlib.machinery.SourceFileLoader,
                      importlib.machinery.SOURCE_SUFFIXES)
            sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
            yield from bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                             seconds=seconds)
    finally:
        sys.dont_write_bytecode = False


def _wo_bytecode(module):
    name = module.__name__
    def benchmark_wo_bytecode(seconds, repeat):
        """Source w/o bytecode: {}"""
        bytecode_path = cache_from_source(module.__file__)
        if os.path.exists(bytecode_path):
            os.unlink(bytecode_path)
        sys.dont_write_bytecode = True
        try:
            yield from bench(name, lambda: sys.modules.pop(name),
                             repeat=repeat, seconds=seconds)
        finally:
            sys.dont_write_bytecode = False

    benchmark_wo_bytecode.__doc__ = benchmark_wo_bytecode.__doc__.format(name)
    return benchmark_wo_bytecode

tabnanny_wo_bytecode = _wo_bytecode(tabnanny)
decimal_wo_bytecode = _wo_bytecode(decimal)


def source_writing_bytecode(seconds, repeat):
    """Source writing bytecode: small"""
    assert not sys.dont_write_bytecode
    name = '__importlib_test_benchmark__'
    with util.create_modules(name) as mapping:
        sys.meta_path.append(importlib.machinery.PathFinder)
        loader = (importlib.machinery.SourceFileLoader,
                  importlib.machinery.SOURCE_SUFFIXES)
        sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
        def cleanup():
            sys.modules.pop(name)
            os.unlink(cache_from_source(mapping[name]))
        for result in bench(name, cleanup, repeat=repeat, seconds=seconds):
            assert not os.path.exists(cache_from_source(mapping[name]))
            yield result


def _writing_bytecode(module):
    name = module.__name__
    def writing_bytecode_benchmark(seconds, repeat):
        """Source writing bytecode: {}"""
        assert not sys.dont_write_bytecode
        def cleanup():
            sys.modules.pop(name)
            os.unlink(cache_from_source(module.__file__))
        yield from bench(name, cleanup, repeat=repeat, seconds=seconds)

    writing_bytecode_benchmark.__doc__ = (
                                writing_bytecode_benchmark.__doc__.format(name))
    return writing_bytecode_benchmark

tabnanny_writing_bytecode = _writing_bytecode(tabnanny)
decimal_writing_bytecode = _writing_bytecode(decimal)


def source_using_bytecode(seconds, repeat):
    """Source w/ bytecode: small"""
    name = '__importlib_test_benchmark__'
    with util.create_modules(name) as mapping:
        sys.meta_path.append(importlib.machinery.PathFinder)
        loader = (importlib.machinery.SourceFileLoader,
                  importlib.machinery.SOURCE_SUFFIXES)
        sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
        py_compile.compile(mapping[name])
        assert os.path.exists(cache_from_source(mapping[name]))
        yield from bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                         seconds=seconds)


def _using_bytecode(module):
    name = module.__name__
    def using_bytecode_benchmark(seconds, repeat):
        """Source w/ bytecode: {}"""
        py_compile.compile(module.__file__)
        yield from bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                         seconds=seconds)

    using_bytecode_benchmark.__doc__ = (
                                using_bytecode_benchmark.__doc__.format(name))
    return using_bytecode_benchmark

tabnanny_using_bytecode = _using_bytecode(tabnanny)
decimal_using_bytecode = _using_bytecode(decimal)


def main(import_, options):
    if options.source_file:
        with open(options.source_file, 'r', encoding='utf-8') as source_file:
            prev_results = json.load(source_file)
    else:
        prev_results = {}
    __builtins__.__import__ = import_
    benchmarks = (from_cache, builtin_mod,
                  source_writing_bytecode,
                  source_wo_bytecode, source_using_bytecode,
                  tabnanny_writing_bytecode,
                  tabnanny_wo_bytecode, tabnanny_using_bytecode,
                  decimal_writing_bytecode,
                  decimal_wo_bytecode, decimal_using_bytecode,
                )
    if options.benchmark:
        for b in benchmarks:
            if b.__doc__ == options.benchmark:
                benchmarks = [b]
                break
        else:
            print('Unknown benchmark: {!r}'.format(options.benchmark),
                  file=sys.stderr)
            sys.exit(1)
    seconds = 1
    seconds_plural = 's' if seconds > 1 else ''
    repeat = 3
    header = ('Measuring imports/second over {} second{}, best out of {}\n'
              'Entire benchmark run should take about {} seconds\n'
              'Using {!r} as __import__\n')
    print(header.format(seconds, seconds_plural, repeat,
                        len(benchmarks) * seconds * repeat, __import__))
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
    if prev_results:
        print('\n\nComparing new vs. old\n')
        for benchmark in benchmarks:
            benchmark_name = benchmark.__doc__
            old_result = max(prev_results[benchmark_name])
            new_result = max(new_results[benchmark_name])
            result = '{:,d} vs. {:,d} ({:%})'.format(new_result,
                                                     old_result,
                                              new_result/old_result)
            print(benchmark_name, ':', result)
    if options.dest_file:
        with open(options.dest_file, 'w', encoding='utf-8') as dest_file:
            json.dump(new_results, dest_file, indent=2)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--builtin', dest='builtin', action='store_true',
                        default=False, help="use the built-in __import__")
    parser.add_argument('-r', '--read', dest='source_file',
                        help='file to read benchmark data from to compare '
                             'against')
    parser.add_argument('-w', '--write', dest='dest_file',
                        help='file to write benchmark data to')
    parser.add_argument('--benchmark', dest='benchmark',
                        help='specific benchmark to run')
    options = parser.parse_args()
    import_ = __import__
    if not options.builtin:
        import_ = importlib.__import__

    main(import_, options)
