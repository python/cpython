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
