import sys


class Null:

    """Just absorb what is given."""

    def __getattr__(self):
        return lambda *args, **kwargs: None


class SilenceStdout:

    """Silence sys.stdout."""

    def setUp(self):
        """Substitute sys.stdout with something that does not print to the
        screen thanks to what bytecode is frozen."""
        sys.stdout = Null()
        super().setUp()

    def tearDown(self):
        sys.stdout = sys.__stdout__
        super().tearDown()
