import unittest

from test.support import threading_helper

import decimal

N_THREADS = 8
ITERATIONS = 100_000

@threading_helper.requires_working_threading()
class TestDecimal(unittest.TestCase):
    def test_add_status(self):
        # prec=4 makes "1.23456" Inexact|Rounded
        shared_ctx = decimal.Context(prec=4)
        def worker():
            for _ in range(ITERATIONS):
                shared_ctx.create_decimal("1.23456")
        threading_helper.run_concurrently(worker, N_THREADS)


    def test_clear_flags(self):
        shared_ctx = decimal.Context(prec=4)

        def producer():
            for _ in range(ITERATIONS):
                shared_ctx.create_decimal("1.23456")

        def clearer():
            for _ in range(ITERATIONS):
                shared_ctx.clear_flags()

        threading_helper.run_concurrently([producer]*N_THREADS + [clearer]*N_THREADS)
