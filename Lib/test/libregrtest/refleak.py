import errno
import os
import re
import sys
import warnings
from inspect import isabstract
from test import support


try:
    MAXFD = os.sysconf("SC_OPEN_MAX")
except Exception:
    MAXFD = 256


def fd_count():
    """Count the number of open file descriptors"""
    if sys.platform.startswith(('linux', 'freebsd')):
        try:
            names = os.listdir("/proc/self/fd")
            return len(names)
        except FileNotFoundError:
            pass

    count = 0
    for fd in range(MAXFD):
        try:
            # Prefer dup() over fstat(). fstat() can require input/output
            # whereas dup() doesn't.
            fd2 = os.dup(fd)
        except OSError as e:
            if e.errno != errno.EBADF:
                raise
        else:
            os.close(fd2)
            count += 1
    return count


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
    fd_deltas = [0] * repcount

    print("beginning", repcount, "repetitions", file=sys.stderr)
    print(("1234567890"*(repcount//10 + 1))[:repcount], file=sys.stderr,
          flush=True)
    # initialize variables to make pyflakes quiet
    rc_before = alloc_before = fd_before = 0
    for i in range(repcount):
        indirect_test()
        alloc_after, rc_after, fd_after = dash_R_cleanup(fs, ps, pic, zdc,
                                                         abcs)
        print('.', end='', file=sys.stderr, flush=True)
        if i >= nwarmup:
            rc_deltas[i] = rc_after - rc_before
            alloc_deltas[i] = alloc_after - alloc_before
            fd_deltas[i] = fd_after - fd_before
        alloc_before = alloc_after
        rc_before = rc_after
        fd_before = fd_after
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
        (alloc_deltas, 'memory blocks', check_alloc_deltas),
        (fd_deltas, 'file descriptors', check_rc_deltas)]:
        if checker(deltas):
            msg = '%s leaked %s %s, sum=%s' % (
                test, deltas[nwarmup:], item_name, sum(deltas))
            print(msg, file=sys.stderr, flush=True)
            with open(fname, "a") as refrep:
                print(msg, file=refrep)
                refrep.flush()
            failed = True
    return failed


def dash_R_cleanup(fs, ps, pic, zdc, abcs):
    import gc, copyreg
    import collections.abc
    from weakref import WeakSet

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
    abs_classes = [getattr(collections.abc, a) for a in collections.abc.__all__]
    abs_classes = filter(isabstract, abs_classes)
    if 'typing' in sys.modules:
        t = sys.modules['typing']
        # These classes require special treatment because they do not appear
        # in direct subclasses of collections.abc classes
        abs_classes = list(abs_classes) + [t.ChainMap, t.Counter, t.DefaultDict]
    for abc in abs_classes:
        for obj in abc.__subclasses__() + [abc]:
            obj._abc_registry = abcs.get(obj, WeakSet()).copy()
            obj._abc_cache.clear()
            obj._abc_negative_cache.clear()

    clear_caches()

    # Collect cyclic trash and read memory statistics immediately after.
    func1 = sys.getallocatedblocks
    func2 = sys.gettotalrefcount
    gc.collect()
    return func1(), func2(), fd_count()


def clear_caches():
    import gc

    # Clear the warnings registry, so they can be displayed again
    for mod in sys.modules.values():
        if hasattr(mod, '__warningregistry__'):
            del mod.__warningregistry__

    # Flush standard output, so that buffered data is sent to the OS and
    # associated Python objects are reclaimed.
    for stream in (sys.stdout, sys.stderr, sys.__stdout__, sys.__stderr__):
        if stream is not None:
            stream.flush()

    # Clear assorted module caches.
    # Don't worry about resetting the cache if the module is not loaded
    try:
        distutils_dir_util = sys.modules['distutils.dir_util']
    except KeyError:
        pass
    else:
        distutils_dir_util._path_created.clear()
    re.purge()

    try:
        _strptime = sys.modules['_strptime']
    except KeyError:
        pass
    else:
        _strptime._regex_cache.clear()

    try:
        urllib_parse = sys.modules['urllib.parse']
    except KeyError:
        pass
    else:
        urllib_parse.clear_cache()

    try:
        urllib_request = sys.modules['urllib.request']
    except KeyError:
        pass
    else:
        urllib_request.urlcleanup()

    try:
        linecache = sys.modules['linecache']
    except KeyError:
        pass
    else:
        linecache.clearcache()

    try:
        mimetypes = sys.modules['mimetypes']
    except KeyError:
        pass
    else:
        mimetypes._default_mime_types()

    try:
        filecmp = sys.modules['filecmp']
    except KeyError:
        pass
    else:
        filecmp._cache.clear()

    try:
        struct = sys.modules['struct']
    except KeyError:
        pass
    else:
        struct._clearcache()

    try:
        doctest = sys.modules['doctest']
    except KeyError:
        pass
    else:
        doctest.master = None

    try:
        ctypes = sys.modules['ctypes']
    except KeyError:
        pass
    else:
        ctypes._reset_cache()

    try:
        typing = sys.modules['typing']
    except KeyError:
        pass
    else:
        for f in typing._cleanups:
            f()

    gc.collect()


def warm_caches():
    # char cache
    s = bytes(range(256))
    for i in range(256):
        s[i:i+1]
    # unicode cache
    [chr(i) for i in range(256)]
    # int cache
    list(range(-5, 257))
