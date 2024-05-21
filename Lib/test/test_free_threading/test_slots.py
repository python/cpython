import threading
from test.support import threading_helper
from unittest import TestCase


class Spam:
    __slots__ = [
        "eggs",
    ]

    def __init__(self, initial_value):
        self.eggs = initial_value


def run_in_threads(targets):
    """A decorator to run some functions in separate threads"""
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

    def test_str(self):
        spam = Spam("0")

        def writer():
            for _ in range(1_000):
                spam.eggs = str(int(spam.eggs) + 1)

        def reader():
            for _ in range(1_000):
                spam.eggs

        run_in_threads([writer, reader, reader, reader])

    # this segfaults
    # def test_int(self):
    #     spam = Spam(1)
    #
    #     def writer():
    #         for _ in range(1_000_000):
    #             spam.eggs += 1
    #
    #     def reader():
    #         for _ in range(1_000_000):
    #             spam.eggs
    #
    #     run_in_threads([writer, writer, reader, reader])
