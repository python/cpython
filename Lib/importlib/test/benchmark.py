"""Benchmark some basic import use-cases."""
# XXX
#    - from source
#        + sys.dont_write_bytecode = True
#        + sys.dont_write_bytecode = False
#    - from bytecode
#    - extensions
from . import util
from .source import util as source_util
import imp
import importlib
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

def from_cache(repeat):
    """sys.modules"""
    name = '<benchmark import>'
    module = imp.new_module(name)
    module.__file__ = '<test>'
    module.__package__ = ''
    with util.uncache(name):
        sys.modules[name] = module
        for result in bench(name, repeat=repeat):
            yield result


def builtin_mod(repeat):
    """Built-in module"""
    name = 'errno'
    if name in sys.modules:
        del sys.modules[name]
    # Relying on built-in importer being implicit.
    for result in bench(name, lambda: sys.modules.pop(name), repeat=repeat):
        yield result


def main(import_, *, repeat=3):
    __builtins__.__import__ = import_
    benchmarks = from_cache, builtin_mod
    print("Measuring imports/second\n")
    for benchmark in benchmarks:
        print(benchmark.__doc__, "[", end=' ')
        sys.stdout.flush()
        results = []
        for result in benchmark(repeat):
            results.append(result)
            print(result, end=' ')
            sys.stdout.flush()
        print("]", "best is", max(results))


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
