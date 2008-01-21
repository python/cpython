import unittest
from test import test_support
import pstats



class AddCallersTestCase(unittest.TestCase):
    """Tests for pstats.add_callers helper."""

    def test_combine_results(self):
        """pstats.add_callers should combine the call results of both target
        and source by adding the call time. See issue1269."""
        target = {"a": (1, 2, 3, 4)}
        source = {"a": (1, 2, 3, 4), "b": (5, 6, 7, 8)}
        new_callers = pstats.add_callers(target, source)
        self.assertEqual(new_callers, {'a': (2, 4, 6, 8), 'b': (5, 6, 7, 8)})


def test_main():
    test_support.run_unittest(
        AddCallersTestCase
    )


if __name__ == "__main__":
    test_main()
