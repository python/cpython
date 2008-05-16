from __future__ import with_statement
from pybench import Test

class WithFinally(Test):

    version = 2.0
    operations = 20
    rounds = 80000

    class ContextManager(object):
        def __enter__(self):
            pass
        def __exit__(self, exc, val, tb):
            pass

    def test(self):

        cm = self.ContextManager()

        for i in range(self.rounds):
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass
            with cm: pass

    def calibrate(self):

        cm = self.ContextManager()

        for i in range(self.rounds):
            pass


class TryFinally(Test):

    version = 2.0
    operations = 20
    rounds = 80000

    class ContextManager(object):
        def __enter__(self):
            pass
        def __exit__(self):
            # "Context manager" objects used just for their cleanup
            # actions in finally blocks usually don't have parameters.
            pass

    def test(self):

        cm = self.ContextManager()

        for i in range(self.rounds):
            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

            cm.__enter__()
            try: pass
            finally: cm.__exit__()

    def calibrate(self):

        cm = self.ContextManager()

        for i in range(self.rounds):
            pass


class WithRaiseExcept(Test):

    version = 2.0
    operations = 2 + 3 + 3
    rounds = 100000

    class BlockExceptions(object):
        def __enter__(self):
            pass
        def __exit__(self, exc, val, tb):
            return True

    def test(self):

        error = ValueError
        be = self.BlockExceptions()

        for i in range(self.rounds):
            with be: raise error
            with be: raise error
            with be: raise error("something")
            with be: raise error("something")
            with be: raise error("something")
            with be: raise error("something")
            with be: raise error("something")
            with be: raise error("something")

    def calibrate(self):

        error = ValueError
        be = self.BlockExceptions()

        for i in range(self.rounds):
            pass
