import os
import sys
import warnings
from inspect import isabstract
from typing import Any

from test import support
from test.support import os_helper

from .runtests import HuntRefleak
from .utils import clear_caches

try:
    from _abc import _get_dump
except ImportError:
    import weakref

    def _get_dump(cls):
        # Reimplement _get_dump() for pure-Python implementation of
        # the abc module (Lib/_py_abc.py)
        registry_weakrefs = set(weakref.ref(obj) for obj in cls._abc_registry)
        return (registry_weakrefs, cls._abc_cache,
                cls._abc_negative_cache, cls._abc_negative_cache_version)


def save_support_xml(filename):
    if support.junit_xml_list is None:
        return

    import pickle
    with open(filename, 'xb') as fp:
        pickle.dump(support.junit_xml_list, fp)
    support.junit_xml_list = None


def restore_support_xml(filename):
    try:
        fp = open(filename, 'rb')
    except FileNotFoundError:
        return

    import pickle
    with fp:
        xml_list = pickle.load(fp)
    os.unlink(filename)

    support.junit_xml_list = xml_list


def runtest_refleak(test_name, test_func,
                    hunt_refleak: HuntRefleak,
                    quiet: bool):
    """Run a test multiple times, looking for reference leaks.

    Returns:
        False if the test didn't leak references; True if we detected refleaks.
    """
    # This code is hackish and inelegant, but it seems to do the job.
    import copyreg
    import collections.abc

    if not hasattr(sys, 'gettotalrefcount'):
        raise Exception("Tracking reference leaks requires a debug build "
                        "of Python")

    # Avoid false positives due to various caches
    # filling slowly with random data:
    warm_caches()

    # Save current values for dash_R_cleanup() to restore.
    fs = warnings.filters[:]
    ps = copyreg.dispatch_table.copy()
    pic = sys.path_importer_cache.copy()
    zdc: dict[str, Any] | None
    try:
        import zipimport
    except ImportError:
        zdc = None # Run unmodified on platforms without zipimport support
    else:
        # private attribute that mypy doesn't know about:
        zdc = zipimport._zip_directory_cache.copy()  # type: ignore[attr-defined]
    abcs = {}
    for abc in [getattr(collections.abc, a) for a in collections.abc.__all__]:
        if not isabstract(abc):
            continue
        for obj in abc.__subclasses__() + [abc]:
            abcs[obj] = _get_dump(obj)[0]

    # bpo-31217: Integer pool to get a single integer object for the same
    # value. The pool is used to prevent false alarm when checking for memory
    # block leaks. Fill the pool with values in -1000..1000 which are the most
    # common (reference, memory block, file descriptor) differences.
    int_pool = {value: value for value in range(-1000, 1000)}
    def get_pooled_int(value):
        return int_pool.setdefault(value, value)

    warmups = hunt_refleak.warmups
    runs = hunt_refleak.runs
    filename = hunt_refleak.filename
    repcount = warmups + runs

    # Pre-allocate to ensure that the loop doesn't allocate anything new
    rep_range = list(range(repcount))
    rc_deltas = [0] * repcount
    alloc_deltas = [0] * repcount
    fd_deltas = [0] * repcount
    getallocatedblocks = sys.getallocatedblocks
    gettotalrefcount = sys.gettotalrefcount
    getunicodeinternedsize = sys.getunicodeinternedsize
    fd_count = os_helper.fd_count
    # initialize variables to make pyflakes quiet
    rc_before = alloc_before = fd_before = interned_before = 0

    if not quiet:
        print("beginning", repcount, "repetitions. Showing number of leaks "
                "(. for 0 or less, X for 10 or more)",
              file=sys.stderr)
        numbers = ("1234567890"*(repcount//10 + 1))[:repcount]
        numbers = numbers[:warmups] + ':' + numbers[warmups:]
        print(numbers, file=sys.stderr, flush=True)

    xml_filename = 'refleak-xml.tmp'
    result = None
    dash_R_cleanup(fs, ps, pic, zdc, abcs)
    support.gc_collect()

    for i in rep_range:
        result = test_func()

        save_support_xml(xml_filename)
        dash_R_cleanup(fs, ps, pic, zdc, abcs)
        support.gc_collect()

        # Read memory statistics immediately after the garbage collection.
        # Also, readjust the reference counts and alloc blocks by ignoring
        # any strings that might have been interned during test_func. These
        # strings will be deallocated at runtime shutdown
        interned_after = getunicodeinternedsize()
        alloc_after = getallocatedblocks() - interned_after
        rc_after = gettotalrefcount() - interned_after * 2
        fd_after = fd_count()

        rc_deltas[i] = get_pooled_int(rc_after - rc_before)
        alloc_deltas[i] = get_pooled_int(alloc_after - alloc_before)
        fd_deltas[i] = get_pooled_int(fd_after - fd_before)

        if not quiet:
            # use max, not sum, so total_leaks is one of the pooled ints
            total_leaks = max(rc_deltas[i], alloc_deltas[i], fd_deltas[i])
            if total_leaks <= 0:
                symbol = '.'
            elif total_leaks < 10:
                symbol = (
                    '.', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                    )[total_leaks]
            else:
                symbol = 'X'
            if i == warmups:
                print(' ', end='', file=sys.stderr, flush=True)
            print(symbol, end='', file=sys.stderr, flush=True)
            del total_leaks
            del symbol

        alloc_before = alloc_after
        rc_before = rc_after
        fd_before = fd_after
        interned_before = interned_after

        restore_support_xml(xml_filename)

    if not quiet:
        print(file=sys.stderr)

    # These checkers return False on success, True on failure
    def check_rc_deltas(deltas):
        # Checker for reference counters and memory blocks.
        #
        # bpo-30776: Try to ignore false positives:
        #
        #   [3, 0, 0]
        #   [0, 1, 0]
        #   [8, -8, 1]
        #
        # Expected leaks:
        #
        #   [5, 5, 6]
        #   [10, 1, 1]
        return all(delta >= 1 for delta in deltas)

    def check_fd_deltas(deltas):
        return any(deltas)

    failed = False
    for deltas, item_name, checker in [
        (rc_deltas, 'references', check_rc_deltas),
        (alloc_deltas, 'memory blocks', check_rc_deltas),
        (fd_deltas, 'file descriptors', check_fd_deltas)
    ]:
        # ignore warmup runs
        deltas = deltas[warmups:]
        failing = checker(deltas)
        suspicious = any(deltas)
        if failing or suspicious:
            msg = '%s leaked %s %s, sum=%s' % (
                test_name, deltas, item_name, sum(deltas))
            print(msg, end='', file=sys.stderr)
            if failing:
                print(file=sys.stderr, flush=True)
                with open(filename, "a", encoding="utf-8") as refrep:
                    print(msg, file=refrep)
                    refrep.flush()
                failed = True
            else:
                print(' (this is fine)', file=sys.stderr, flush=True)
    return (failed, result)


def dash_R_cleanup(fs, ps, pic, zdc, abcs):
    import copyreg
    import collections.abc

    # Restore some original values.
    warnings.filters[:] = fs
    copyreg.dispatch_table.clear()
    copyreg.dispatch_table.update(ps)
    sys.path_importer_cache.clear()
    sys.path_importer_cache.update(pic)
    try:
        import zipimport
    except ImportError:
        pass # Run unmodified on platforms without zipimport support
    else:
        zipimport._zip_directory_cache.clear()
        zipimport._zip_directory_cache.update(zdc)

    # Clear ABC registries, restoring previously saved ABC registries.
    # ignore deprecation warning for collections.abc.ByteString
    abs_classes = [getattr(collections.abc, a) for a in collections.abc.__all__]
    abs_classes = filter(isabstract, abs_classes)
    for abc in abs_classes:
        for obj in abc.__subclasses__() + [abc]:
            for ref in abcs.get(obj, set()):
                if ref() is not None:
                    obj.register(ref())
            obj._abc_caches_clear()

    # Clear caches
    clear_caches()

    # Clear type cache at the end: previous function calls can modify types
    sys._clear_type_cache()


def warm_caches():
    # char cache
    s = bytes(range(256))
    for i in range(256):
        s[i:i+1]
    # unicode cache
    [chr(i) for i in range(256)]
    # int cache
    list(range(-5, 257))
