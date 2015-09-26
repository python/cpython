import os
import re
import sys
import warnings
from inspect import isabstract
from test import support


def dash_R(the_module, test, indirect_test, huntrleaks):
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

    # Save current values for dash_R_cleanup() to restore.
    fs = warnings.filters[:]
    ps = copyreg.dispatch_table.copy()
    pic = sys.path_importer_cache.copy()
    try:
        import zipimport
    except ImportError:
        zdc = None # Run unmodified on platforms without zipimport support
    else:
        zdc = zipimport._zip_directory_cache.copy()
    abcs = {}
    for abc in [getattr(collections.abc, a) for a in collections.abc.__all__]:
        if not isabstract(abc):
            continue
        for obj in abc.__subclasses__() + [abc]:
            abcs[obj] = obj._abc_registry.copy()

    nwarmup, ntracked, fname = huntrleaks
    fname = os.path.join(support.SAVEDCWD, fname)
    repcount = nwarmup + ntracked
    rc_deltas = [0] * repcount
    alloc_deltas = [0] * repcount

    print("beginning", repcount, "repetitions", file=sys.stderr)
    print(("1234567890"*(repcount//10 + 1))[:repcount], file=sys.stderr)
    sys.stderr.flush()
    for i in range(repcount):
        indirect_test()
        alloc_after, rc_after = dash_R_cleanup(fs, ps, pic, zdc, abcs)
        sys.stderr.write('.')
        sys.stderr.flush()
        if i >= nwarmup:
            rc_deltas[i] = rc_after - rc_before
            alloc_deltas[i] = alloc_after - alloc_before
        alloc_before, rc_before = alloc_after, rc_after
    print(file=sys.stderr)
    # These checkers return False on success, True on failure
    def check_rc_deltas(deltas):
        return any(deltas)
    def check_alloc_deltas(deltas):
        # At least 1/3rd of 0s
        if 3 * deltas.count(0) < len(deltas):
            return True
        # Nothing else than 1s, 0s and -1s
        if not set(deltas) <= {1,0,-1}:
            return True
        return False
    failed = False
    for deltas, item_name, checker in [
        (rc_deltas, 'references', check_rc_deltas),
        (alloc_deltas, 'memory blocks', check_alloc_deltas)]:
        if checker(deltas):
            msg = '%s leaked %s %s, sum=%s' % (
                test, deltas[nwarmup:], item_name, sum(deltas))
            print(msg, file=sys.stderr)
            sys.stderr.flush()
            with open(fname, "a") as refrep:
                print(msg, file=refrep)
                refrep.flush()
            failed = True
    return failed


def dash_R_cleanup(fs, ps, pic, zdc, abcs):
    import gc, copyreg
    import _strptime, linecache
    import urllib.parse, urllib.request, mimetypes, doctest
    import struct, filecmp, collections.abc
    from distutils.dir_util import _path_created
    from weakref import WeakSet

    # Clear the warnings registry, so they can be displayed again
    for mod in sys.modules.values():
        if hasattr(mod, '__warningregistry__'):
            del mod.__warningregistry__

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

    # clear type cache
    sys._clear_type_cache()

    # Clear ABC registries, restoring previously saved ABC registries.
    for abc in [getattr(collections.abc, a) for a in collections.abc.__all__]:
        if not isabstract(abc):
            continue
        for obj in abc.__subclasses__() + [abc]:
            obj._abc_registry = abcs.get(obj, WeakSet()).copy()
            obj._abc_cache.clear()
            obj._abc_negative_cache.clear()

    # Flush standard output, so that buffered data is sent to the OS and
    # associated Python objects are reclaimed.
    for stream in (sys.stdout, sys.stderr, sys.__stdout__, sys.__stderr__):
        if stream is not None:
            stream.flush()

    # Clear assorted module caches.
    _path_created.clear()
    re.purge()
    _strptime._regex_cache.clear()
    urllib.parse.clear_cache()
    urllib.request.urlcleanup()
    linecache.clearcache()
    mimetypes._default_mime_types()
    filecmp._cache.clear()
    struct._clearcache()
    doctest.master = None
    try:
        import ctypes
    except ImportError:
        # Don't worry about resetting the cache if ctypes is not supported
        pass
    else:
        ctypes._reset_cache()

    # Collect cyclic trash and read memory statistics immediately after.
    func1 = sys.getallocatedblocks
    func2 = sys.gettotalrefcount
    gc.collect()
    return func1(), func2()


def warm_caches():
    # char cache
    s = bytes(range(256))
    for i in range(256):
        s[i:i+1]
    # unicode cache
    x = [chr(i) for i in range(256)]
    # int cache
    x = list(range(-5, 257))
