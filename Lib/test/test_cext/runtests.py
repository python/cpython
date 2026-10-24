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


def run_refleak_tests(testmod):
    # Run tests and check for reference leaks
    warmups = 3
    runs = 3
    repcount = warmups + runs
    refcount = array.array('q', [0] * repcount)
    rep_range = tuple(range(repcount))

    for i in rep_range:
        print(f"Run #{i + 1}", flush=True)
        run_tests(testmod)
        gc.collect()
        refcount[i] = sys.gettotalrefcount()

    # Compute differences
    previous = None
    for i in rep_range:
        value = refcount[i]
        if previous is not None:
           refcount[i] = value - previous
        previous = value

    deltas = list(refcount[warmups:])
    if all(delta >= 1 for delta in deltas):
        print(f'ERROR: Tests leaked {deltas} references')
        sys.exit(1)
    elif any(deltas):
        print(f'WARNING: Tests leaked {deltas} references (this is fine)')


def main():
    if len(sys.argv) < 2:
        print("usage: python runtests.py TEST_MODULE_NAME")
        sys.exit(1)
    module_name = sys.argv[1]

    testmod = importlib.import_module(module_name)

    newline = False
    for name in (
        '__STDC_VERSION__',
        '__STRICT_ANSI__',
        '__cplusplus',
        '_MSVC_LANG',
    ):
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
        run_refleak_tests(testmod)
    else:
        run_tests(testmod)


if __name__ == "__main__":
    main()
