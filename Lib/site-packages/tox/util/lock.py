"""holds locking functionality that works across processes"""
from __future__ import absolute_import, unicode_literals

from contextlib import contextmanager

import py
from filelock import FileLock, Timeout

from tox.reporter import verbosity1


@contextmanager
def hold_lock(lock_file, reporter=verbosity1):
    py.path.local(lock_file.dirname).ensure(dir=1)
    lock = FileLock(str(lock_file))
    try:
        try:
            lock.acquire(0.0001)
        except Timeout:
            reporter("lock file {} present, will block until released".format(lock_file))
            lock.acquire()
        yield
    finally:
        lock.release(force=True)


def get_unique_file(path, prefix, suffix):
    """get a unique file in a folder having a given prefix and suffix,
    with unique number in between"""
    lock_file = path.join(".lock")
    prefix = "{}-".format(prefix)
    with hold_lock(lock_file):
        max_value = -1
        for candidate in path.listdir("{}*{}".format(prefix, suffix)):
            try:
                max_value = max(max_value, int(candidate.basename[len(prefix) : -len(suffix)]))
            except ValueError:
                continue
        winner = path.join("{}{}{}".format(prefix, max_value + 1, suffix))
        winner.ensure(dir=0)
        return winner
