from . import util
from .source import util as source_util
import gc
import decimal
import imp
import importlib
import sys
import timeit


def bench_cache(import_, repeat, number):
    """Measure the time it takes to pull from sys.modules."""
    name = '<benchmark import>'
    with util.uncache(name):
        module = imp.new_module(name)
        sys.modules[name] = module
        runs = []
        for x in range(repeat):
            start_time = timeit.default_timer()
            for y in range(number):
                import_(name)
            end_time = timeit.default_timer()
            runs.append(end_time - start_time)
        return min(runs)


def bench_importing_source(import_, repeat, number, loc=100000):
    """Measure importing source from disk.

    For worst-case scenario, the line endings are \\r\\n and thus require
    universal newline translation.

    """
    name = '__benchmark'
    with source_util.create_modules(name) as mapping:
        with open(mapping[name], 'w') as file:
            for x in range(loc):
                file.write("{0}\r\n".format(x))
        with util.import_state(path=[mapping['.root']]):
            runs = []
            for x in range(repeat):
                start_time = timeit.default_timer()
                for y in range(number):
                    try:
                        import_(name)
                    finally:
                        del sys.modules[name]
                end_time = timeit.default_timer()
                runs.append(end_time - start_time)
            return min(runs)


def main(import_):
    args = [('sys.modules', bench_cache, 5, 500000),
            ('source', bench_importing_source, 5, 10000)]
    test_msg = "{test}, {number} times (best of {repeat}):"
    result_msg = "{result:.2f} secs"
    gc.disable()
    try:
        for name, meth, repeat, number in args:
            result = meth(import_, repeat, number)
            print(test_msg.format(test=name, repeat=repeat,
                    number=number).ljust(40),
                    result_msg.format(result=result).rjust(10))
    finally:
        gc.enable()


if __name__ == '__main__':
    import optparse

    parser = optparse.OptionParser()
    parser.add_option('-b', '--builtin', dest='builtin', action='store_true',
                        default=False, help="use the built-in __import__")
    options, args = parser.parse_args()
    if args:
        raise RuntimeError("unrecognized args: {0}".format(args))
    import_ = __import__
    if not options.builtin:
        import_ = importlib.__import__

    main(import_)
