"""Stubs for the subset of ``hypothesis.stateful`` used by the test suite.

The stateful API has no equivalent of ``@example``, so without Hypothesis
there is nothing to replay: the ``TestCase`` generated for a state machine
is skipped, and ``run_state_machine_as_test`` raises ``unittest.SkipTest``.
The decorators and helpers are accepted so that state machine classes can
be defined at import time.
"""
import unittest

from ._helpers import stub_factory
from .strategies import StubStrategy

__all__ = [
    "Bundle",
    "MultipleResults",
    "RuleBasedStateMachine",
    "consumes",
    "initialize",
    "invariant",
    "multiple",
    "precondition",
    "rule",
    "run_state_machine_as_test",
]

_SKIP_REASON = "Hypothesis required for stateful test"

Bundle = stub_factory(StubStrategy, "hypothesis.stateful.Bundle")


def consumes(bundle):
    return bundle


class MultipleResults:
    def __init__(self, *values):
        self.values = values


def multiple(*args):
    return MultipleResults(*args)


def _identity_operator(f):
    return f


def _identity_decorator(*_args, **_kwargs):
    return _identity_operator


rule = _identity_decorator
initialize = _identity_decorator
invariant = _identity_decorator
precondition = _identity_decorator


class _TestCaseProperty:
    """Build (and cache) a skipped ``unittest.TestCase`` per state machine."""

    def __init__(self):
        self._cases = {}

    def __get__(self, obj, owner):
        if owner not in self._cases:

            class StateMachineTestCase(unittest.TestCase):
                settings = None

                def runTest(self):
                    raise unittest.SkipTest(_SKIP_REASON)

            StateMachineTestCase.__name__ = f"{owner.__name__}.TestCase"
            StateMachineTestCase.__qualname__ = f"{owner.__qualname__}.TestCase"
            self._cases[owner] = StateMachineTestCase
        return self._cases[owner]


class RuleBasedStateMachine:
    TestCase = _TestCaseProperty()

    def bundle(self, name):
        return Bundle(name)

    def check_invariants(self, settings, output, runtimes):
        pass

    def setup_state(self):
        pass

    def teardown(self):
        pass


def run_state_machine_as_test(state_machine_factory, *, settings=None):
    raise unittest.SkipTest(_SKIP_REASON)
