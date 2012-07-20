"""Benchmark some basic import use-cases.

The assumption is made that this benchmark is run in a fresh interpreter and
thus has no external changes made to import-related attributes in sys.

"""
from test.test_importlib import util
from test.test_importlib.source import util as source_util
import decimal
import imp
import importlib
import importlib.machinery
import json
import os
import py_compile
import sys
import tabnanny
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
    """Source w/o bytecode: small"""
    sys.dont_write_bytecode = True
    try:
        name = '__importlib_test_benchmark__'
        # Clears out sys.modules and puts an entry at the front of sys.path.
        with source_util.create_modules(name) as mapping:
            assert not os.path.exists(imp.cache_from_source(mapping[name]))
            sys.meta_path.append(importlib.machinery.PathFinder)
            loader = (importlib.machinery.SourceFileLoader,
                      importlib.machinery.SOURCE_SUFFIXES, True)
            sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
            for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                                seconds=seconds):
                yield result
    finally:
        sys.dont_write_bytecode = False


def _wo_bytecode(module):
    name = module.__name__
    def benchmark_wo_bytecode(seconds, repeat):
        """Source w/o bytecode: {}"""
        bytecode_path = imp.cache_from_source(module.__file__)
        if os.path.exists(bytecode_path):
            os.unlink(bytecode_path)
        sys.dont_write_bytecode = True
        try:
            for result in bench(name, lambda: sys.modules.pop(name),
                                repeat=repeat, seconds=seconds):
                yield result
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
    with source_util.create_modules(name) as mapping:
        sys.meta_path.append(importlib.machinery.PathFinder)
        loader = (importlib.machinery.SourceFileLoader,
                  importlib.machinery.SOURCE_SUFFIXES, True)
        sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
        def cleanup():
            sys.modules.pop(name)
            os.unlink(imp.cache_from_source(mapping[name]))
        for result in bench(name, cleanup, repeat=repeat, seconds=seconds):
            assert not os.path.exists(imp.cache_from_source(mapping[name]))
            yield result


def _writing_bytecode(module):
    name = module.__name__
    def writing_bytecode_benchmark(seconds, repeat):
        """Source writing bytecode: {}"""
        assert not sys.dont_write_bytecode
        def cleanup():
            sys.modules.pop(name)
            os.unlink(imp.cache_from_source(module.__file__))
        for result in bench(name, cleanup, repeat=repeat, seconds=seconds):
            yield result

    writing_bytecode_benchmark.__doc__ = (
                                writing_bytecode_benchmark.__doc__.format(name))
    return writing_bytecode_benchmark

tabnanny_writing_bytecode = _writing_bytecode(tabnanny)
decimal_writing_bytecode = _writing_bytecode(decimal)


def source_using_bytecode(seconds, repeat):
    """Source w/ bytecode: small"""
    name = '__importlib_test_benchmark__'
    with source_util.create_modules(name) as mapping:
        sys.meta_path.append(importlib.machinery.PathFinder)
        loader = (importlib.machinery.SourceFileLoader,
                importlib.machinery.SOURCE_SUFFIXES, True)
        sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))
        py_compile.compile(mapping[name])
        assert os.path.exists(imp.cache_from_source(mapping[name]))
        for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                            seconds=seconds):
            yield result


def _using_bytecode(module):
    name = module.__name__
    def using_bytecode_benchmark(seconds, repeat):
        """Source w/ bytecode: {}"""
        py_compile.compile(module.__file__)
        for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat,
                            seconds=seconds):
            yield result

    using_bytecode_benchmark.__doc__ = (
                                using_bytecode_benchmark.__doc__.format(name))
    return using_bytecode_benchmark

tabnanny_using_bytecode = _using_bytecode(tabnanny)
decimal_using_bytecode = _using_bytecode(decimal)


def main(import_, options):
    if options.source_file:
        with options.source_file:
            prev_results = json.load(options.source_file)
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
            print('Unknown benchmark: {!r}'.format(options.benchmark,
                  file=sys.stderr))
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
        with options.dest_file:
            json.dump(new_results, options.dest_file, indent=2)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--builtin', dest='builtin', action='store_true',
                        default=False, help="use the built-in __import__")
    parser.add_argument('-r', '--read', dest='source_file',
                        type=argparse.FileType('r'),
                        help='file to read benchmark data from to compare '
                             'against')
    parser.add_argument('-w', '--write', dest='dest_file',
                        type=argparse.FileType('w'),
                        help='file to write benchmark data to')
    parser.add_argument('--benchmark', dest='benchmark',
                        help='specific benchmark to run')
    options = parser.parse_args()
    import_ = __import__
    if not options.builtin:
        import_ = importlib.__import__

    main(import_, options)
