import _testcapi
import threading
from test.support import threading_helper
from unittest import TestCase


def run_in_threads(targets):
    """Run `targets` in separate threads"""
    threads = [
        threading.Thread(target=target)
        for target in targets
    ]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()


@threading_helper.requires_working_threading()
class TestSlots(TestCase):

    def test_object(self):
        class Spam:
            __slots__ = [
                "eggs",
            ]

            def __init__(self, initial_value):
                self.eggs = initial_value

        spam = Spam(0)
        iters = 20_000

        def writer():
            for _ in range(iters):
                spam.eggs += 1

        def reader():
            for _ in range(iters):
                eggs = spam.eggs
                assert type(eggs) is int
                assert 0 <= eggs <= iters

        run_in_threads([writer, reader, reader, reader])

    def test_T_BOOL(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                # different code paths for True and False
                spam_old.T_BOOL = True
                spam_new.T_BOOL = True
                spam_old.T_BOOL = False
                spam_new.T_BOOL = False

        def reader():
            for _ in range(1_000):
                spam_old.T_BOOL
                spam_new.T_BOOL

        run_in_threads([writer, reader])

    def test_T_BYTE(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_BYTE = 0
                spam_new.T_BYTE = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_BYTE
                spam_new.T_BYTE

        run_in_threads([writer, reader])

    def test_T_UBYTE(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_UBYTE = 0
                spam_new.T_UBYTE = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_UBYTE
                spam_new.T_UBYTE

        run_in_threads([writer, reader])

    def test_T_SHORT(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_SHORT = 0
                spam_new.T_SHORT = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_SHORT
                spam_new.T_SHORT

        run_in_threads([writer, reader])

    def test_T_USHORT(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_USHORT = 0
                spam_new.T_USHORT = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_USHORT
                spam_new.T_USHORT

        run_in_threads([writer, reader])

    def test_T_INT(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_INT = 0
                spam_new.T_INT = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_INT
                spam_new.T_INT

        run_in_threads([writer, reader])

    def test_T_UINT(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_UINT = 0
                spam_new.T_UINT = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_UINT
                spam_new.T_UINT

        run_in_threads([writer, reader])

    def test_T_LONG(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_LONG = 0
                spam_new.T_LONG = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_LONG
                spam_new.T_LONG

        run_in_threads([writer, reader])

    def test_T_ULONG(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_ULONG = 0
                spam_new.T_ULONG = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_ULONG
                spam_new.T_ULONG

        run_in_threads([writer, reader])

    def test_T_PYSSIZET(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_PYSSIZET = 0
                spam_new.T_PYSSIZET = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_PYSSIZET
                spam_new.T_PYSSIZET

        run_in_threads([writer, reader])

    def test_T_FLOAT(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_FLOAT = 0.0
                spam_new.T_FLOAT = 0.0

        def reader():
            for _ in range(1_000):
                spam_old.T_FLOAT
                spam_new.T_FLOAT

        run_in_threads([writer, reader])

    def test_T_DOUBLE(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_DOUBLE = 0.0
                spam_new.T_DOUBLE = 0.0

        def reader():
            for _ in range(1_000):
                spam_old.T_DOUBLE
                spam_new.T_DOUBLE

        run_in_threads([writer, reader])

    def test_T_LONGLONG(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_LONGLONG = 0
                spam_new.T_LONGLONG = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_LONGLONG
                spam_new.T_LONGLONG

        run_in_threads([writer, reader])

    def test_T_ULONGLONG(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_ULONGLONG = 0
                spam_new.T_ULONGLONG = 0

        def reader():
            for _ in range(1_000):
                spam_old.T_ULONGLONG
                spam_new.T_ULONGLONG

        run_in_threads([writer, reader])

    def test_T_CHAR(self):
        spam_old = _testcapi._test_structmembersType_OldAPI()
        spam_new = _testcapi._test_structmembersType_NewAPI()

        def writer():
            for _ in range(1_000):
                spam_old.T_CHAR = "c"
                spam_new.T_CHAR = "c"

        def reader():
            for _ in range(1_000):
                spam_old.T_CHAR
                spam_new.T_CHAR

        run_in_threads([writer, reader])
