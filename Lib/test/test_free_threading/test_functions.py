import random
import unittest
from unittest import TestCase

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)

NUM_THREADS = 8
ITERS = 200


def random_string():
    return ''.join(random.choice('0123456789ABCDEF') for _ in range(10))


def template_a(): pass
def template_b(): pass


class TestFTFunctionAttributes(TestCase):

    def stress_attribute(self, attr, make_value):
        def target(x=1):
            return x

        def writer():
            for _ in range(ITERS):
                setattr(target, attr, make_value())
                getattr(target, attr)

        threading_helper.run_concurrently(writer, NUM_THREADS)

    def test_name(self):
        self.stress_attribute("__name__", random_string)

    def test_qualname(self):
        self.stress_attribute("__qualname__", random_string)

    def test_code(self):
        codes = (template_a.__code__, template_b.__code__)
        self.stress_attribute("__code__", lambda: random.choice(codes))

    def test_defaults(self):
        self.stress_attribute("__defaults__", lambda: (random_string(),))

    def test_kwdefaults(self):
        self.stress_attribute("__kwdefaults__", lambda: {"x": random_string()})

    def test_annotations(self):
        self.stress_attribute("__annotations__",
                              lambda: {"x": random_string()})

    def test_annotate(self):
        self.stress_attribute("__annotate__",
                              lambda: (lambda format: {"x": str}))

    def test_type_params(self):
        self.stress_attribute("__type_params__", lambda: (random_string(),))

    def test_doc(self):
        self.stress_attribute("__doc__", random_string)

    def test_module(self):
        self.stress_attribute("__module__", random_string)

    def test_annotations_and_annotate(self):
        # The __annotations__ and __annotate__ setters clear each other.
        def target(): pass

        def set_annotations():
            for _ in range(ITERS):
                target.__annotations__ = {"x": random_string()}
                target.__annotations__

        def set_annotate():
            for _ in range(ITERS):
                target.__annotate__ = lambda format: {"x": str}
                target.__annotate__

        threading_helper.run_concurrently(
            [set_annotations, set_annotate] * (NUM_THREADS // 2))

    def test_call_while_replacing_defaults(self):
        # The eval loop reads __defaults__ and __kwdefaults__ without holding
        # a lock while pushing a frame.
        def target(x="init", *, y="init"):
            return x, y

        def writer():
            for _ in range(ITERS):
                target.__defaults__ = (random_string(),)
                target.__kwdefaults__ = {"y": random_string()}

        def caller():
            for _ in range(ITERS):
                target()

        threading_helper.run_concurrently(
            [writer, caller] * (NUM_THREADS // 2))


if __name__ == "__main__":
    unittest.main()
