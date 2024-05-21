import threading
from test.support import threading_helper
from unittest import TestCase


# import _testcapi


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
                "eggs",  # the Py_T_OBJECT_EX type is missing in
                         # Modules/_testcapi/structmember.c
            ]

            def __init__(self, initial_value):
                self.eggs = initial_value

        spam = Spam(0)
        iters = 1_000_000

        def writer():
            for _ in range(iters):
                spam.eggs += 1

        def reader():
            for _ in range(iters):
                eggs = spam.eggs
                assert type(eggs) is int
                assert 0 <= eggs <= iters

        run_in_threads([writer, reader, reader, reader])

    # def test_bool(self):
    #     spam_old = _testcapi._test_structmembersType_OldAPI()
    #     spam_new = _testcapi._test_structmembersType_NewAPI()
    #
    #     def writer():
    #         for _ in range(1_000):
    #             spam_old.T_BOOL = True
    #             spam_old.T_BOOL = False  # separate code path for True/False
    #             spam_new.T_BOOL = True
    #             spam_new.T_BOOL = False
    #
    #     def reader():
    #         for _ in range(1_000):
    #             spam_old.T_BOOL
    #             spam_new.T_BOOL
    #
    #     run_in_threads([writer, reader, reader, reader])
