import array
import gc
import importlib
import sys


def run_tests(testmod, verbose=True):
    for name in dir(testmod):
        if not name.startswith('test'):
            continue
        func = getattr(testmod, name)
        print(f"{name}()")
        func()

    print("add()")
    if testmod.add(11, 23) != 34:
        raise AssertionError("add() failed badly")

    print(flush=True)


def main():
    if len(sys.argv) < 2:
        print("usage: python runtests.py TEST_MODULE_NAME")
        sys.exit(1)
    module_name = sys.argv[1]

    testmod = importlib.import_module(module_name)

    newline = False
    for name in ('__STDC_VERSION__', '__cplusplus', '_MSVC_LANG'):
        try:
            value = getattr(testmod, name)
        except AttributeError:
            pass
        else:
            print(f'{name}: {value}')
            newline = True
    if newline:
        print()

    if hasattr(sys, 'gettotalrefcount'):
        # First run to warm up Python. For example, test_datetime() imports
        # the datetime module.
        run_tests(testmod, verbose=False)

        refcount = array.array('q', [0, 0])
        gc.collect()

        # Check for reference leak
        refcount[0] = sys.gettotalrefcount()
        run_tests(testmod)
        refcount[1] = sys.gettotalrefcount()

        diff = refcount[1] - refcount[0]
        if diff >= 1:
            raise AssertionError(f'Tests leaked {diff} references')
    else:
        run_tests(testmod)


if __name__ == "__main__":
    main()
