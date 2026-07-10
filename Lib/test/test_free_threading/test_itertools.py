import unittest
from itertools import (
    tee,
)
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)


class TestTeeConcurrent(unittest.TestCase):
    # itertools.tee branches share a linked list of internal data cells.
    # Concurrent iteration must not corrupt that shared state or crash the
    # free-threaded build.  A crash shows up as the interpreter dying (not as a
    # caught exception); tee is documented as not thread-safe, so a
    # ``RuntimeError`` from the re-entrancy guard is an allowed outcome and is
    # tolerated here.

    def test_same_branch(self):
        # Many threads consume the same tee branch.
        errors = []

        def consume(it):
            try:
                for _ in it:
                    pass
            except RuntimeError:
                pass
            except Exception as e:
                errors.append(e)

        for _ in range(100):
            a, _ = tee(iter(range(2000)), 2)
            threading_helper.run_concurrently(consume, nthreads=8, args=(a,))

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")

    def test_sibling_branches(self):
        # Each thread consumes a different sibling branch of the same tee.
        errors = []

        def make_worker(it):
            def consume():
                try:
                    for _ in it:
                        pass
                except RuntimeError:
                    pass
                except Exception as e:
                    errors.append(e)

            return consume

        for _ in range(100):
            branches = tee(iter(range(4000)), 8)
            threading_helper.run_concurrently(
                [make_worker(it) for it in branches]
            )

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")


if __name__ == "__main__":
    unittest.main()
