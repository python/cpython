import errno
import os
import unittest


def _supports_sched():
    if not hasattr(os, 'sched_getscheduler'):
        return False
    try:
        os.sched_getscheduler(0)
    except OSError as e:
        if e.errno == errno.ENOSYS:
            return False
    return True

requires_sched = unittest.skipUnless(_supports_sched(), 'requires POSIX scheduler API')


def create_file(filename, content=b'content'):
    with open(filename, "xb", 0) as fp:
        if content:
            fp.write(content)
