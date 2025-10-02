import unittest

try:
    import posix
except ImportError:
    import nt as posix


def create_file(filename, content=b'content'):
    with open(filename, "xb", 0) as fp:
        fp.write(content)


def _supports_sched():
    if not hasattr(posix, 'sched_getscheduler'):
        return False
    try:
        posix.sched_getscheduler(0)
    except OSError as e:
        if e.errno == errno.ENOSYS:
            return False
    return True

requires_sched = unittest.skipUnless(_supports_sched(), 'requires POSIX scheduler API')
