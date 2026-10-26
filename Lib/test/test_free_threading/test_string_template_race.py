import string
import unittest
from string import Template

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestTemplateCompileRace(unittest.TestCase):
    def test_concurrent_first_use(self):
        # Racing the lazy pattern compile must not raise a spurious ValueError
        # from recompiling an already-compiled pattern.  A throwaway subclass,
        # re-armed to the sentinel each round, keeps string.Template unmutated
        # (subclasses precompile eagerly in __init_subclass__).
        uncompiled = string._TemplatePattern
        errors = []

        def use_template(cls):
            try:
                cls("$x and ${y}").substitute(x=1, y=2)
            except Exception as e:
                errors.append(e)

        for _ in range(20):
            class T(Template):
                pass
            T.pattern = uncompiled
            T.flags = None
            threading_helper.run_concurrently(use_template, nthreads=10, args=(T,))

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")


if __name__ == "__main__":
    unittest.main()
