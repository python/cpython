import string
import unittest
from string import Template

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestTemplateCompileRace(unittest.TestCase):
    def test_concurrent_first_use(self):
        # Template compiles its pattern lazily on first use and caches it on the
        # class.  Race that lazy compile from many threads and confirm none hits
        # a spurious ValueError from recompiling an already-compiled pattern.
        # Use a throwaway subclass each round so the shared string.Template is
        # never mutated; subclasses precompile in __init_subclass__, so restore
        # the sentinel descriptor (string._TemplatePattern) to re-arm the lazy
        # path before racing.
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
