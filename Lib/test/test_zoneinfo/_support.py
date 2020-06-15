import contextlib
import functools
import sys
import threading
import unittest
from test.support import import_fresh_module

OS_ENV_LOCK = threading.Lock()
TZPATH_LOCK = threading.Lock()
TZPATH_TEST_LOCK = threading.Lock()


def call_once(f):
    """Decorator that ensures a function is only ever called once."""
    lock = threading.Lock()
    cached = functools.lru_cache(None)(f)

    @functools.wraps(f)
    def inner():
        with lock:
            return cached()

    return inner


@call_once
def get_modules():
    """Retrieve two copies of zoneinfo: pure Python and C accelerated.

    Because this function manipulates the import system in a way that might
    be fragile or do unexpected things if it is run many times, it uses a
    `call_once` decorator to ensure that this is only ever called exactly
    one time — in other words, when using this function you will only ever
    get one copy of each module rather than a fresh import each time.
    """
    import zoneinfo as c_module

    py_module = import_fresh_module("zoneinfo", blocked=["_zoneinfo"])

    return py_module, c_module


@contextlib.contextmanager
def set_zoneinfo_module(module):
    """Make sure sys.modules["zoneinfo"] refers to `module`.

    This is necessary because `pickle` will refuse to serialize
    an type calling itself `zoneinfo.ZoneInfo` unless `zoneinfo.ZoneInfo`
    refers to the same object.
    """

    NOT_PRESENT = object()
    old_zoneinfo = sys.modules.get("zoneinfo", NOT_PRESENT)
    sys.modules["zoneinfo"] = module
    yield
    if old_zoneinfo is not NOT_PRESENT:
        sys.modules["zoneinfo"] = old_zoneinfo
    else:  # pragma: nocover
        sys.modules.pop("zoneinfo")


class ZoneInfoTestBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.klass = cls.module.ZoneInfo
        super().setUpClass()

    @contextlib.contextmanager
    def tzpath_context(self, tzpath, block_tzdata=True, lock=TZPATH_LOCK):
        def pop_tzdata_modules():
            tzdata_modules = {}
            for modname in list(sys.modules):
                if modname.split(".", 1)[0] != "tzdata":  # pragma: nocover
                    continue

                tzdata_modules[modname] = sys.modules.pop(modname)

            return tzdata_modules

        with lock:
            if block_tzdata:
                # In order to fully exclude tzdata from the path, we need to
                # clear the sys.modules cache of all its contents — setting the
                # root package to None is not enough to block direct access of
                # already-imported submodules (though it will prevent new
                # imports of submodules).
                tzdata_modules = pop_tzdata_modules()
                sys.modules["tzdata"] = None

            old_path = self.module.TZPATH
            try:
                self.module.reset_tzpath(tzpath)
                yield
            finally:
                if block_tzdata:
                    sys.modules.pop("tzdata")
                    for modname, module in tzdata_modules.items():
                        sys.modules[modname] = module

                self.module.reset_tzpath(old_path)
