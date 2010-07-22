"""Benchmark some basic import use-cases.

The assumption is made that this benchmark is run in a fresh interpreter and
thus has no external changes made to import-related attributes in sys.

"""
from . import util
from .source import util as source_util
import decimal
import imp
import importlib
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


def main(import_):
    __builtins__.__import__ = import_
    benchmarks = (from_cache, builtin_mod,
                  source_using_bytecode, source_wo_bytecode,
                  source_writing_bytecode,
                  decimal_using_bytecode, decimal_writing_bytecode,
                  decimal_wo_bytecode,)
    seconds = 1
    seconds_plural = 's' if seconds > 1 else ''
    repeat = 3
    header = "Measuring imports/second over {} second{}, best out of {}\n"
    print(header.format(seconds, seconds_plural, repeat))
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


if __name__ == '__main__':
    import optparse

    parser = optparse.OptionParser()
    parser.add_option('-b', '--builtin', dest='builtin', action='store_true',
                        default=False, help="use the built-in __import__")
    options, args = parser.parse_args()
    if args:
        raise RuntimeError("unrecognized args: {}".format(args))
    import_ = __import__
    if not options.builtin:
        import_ = importlib.__import__

    main(import_)
