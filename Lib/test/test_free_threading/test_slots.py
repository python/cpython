import threading
from test.support import threading_helper
from unittest import TestCase


class Spam:
    __slots__ = [
        "cheese",
        "eric",
    ]

    def __init__(self):
        self.cheese = 0
        self.eric = ""


spam = Spam()


def writer():
    for _ in range(1_000):
        spam.eric = "idle"
        spam.cheese += 1


def reader():
    spam.eric
    spam.cheese


@threading_helper.requires_working_threading()
class TestList(TestCase):

    def test_slots(self):
        threads = [threading.Thread(target=writer), *[
            threading.Thread(target=reader)
            for _ in range(3)
        ]]

        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
