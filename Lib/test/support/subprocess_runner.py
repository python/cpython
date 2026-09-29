"""Run a single test method in this (sub)process and report the result.

Invoked as ``python -m test.support.subprocess_runner MODULE QUALNAME OUTFILE
CONFIG`` by :func:`test.support.isolation.runInSubprocess`.  CONFIG is the
marshalled test.support configuration of the parent test run, as a hex string.
The outcome of the test (including that of each individual subtest) is
marshalled to OUTFILE.  This module is not meant to be imported.

Import as little as possible before running the test: every module imported
here is state that the test would not see in a normal test run.
"""

import marshal
import sys
import unittest
from unittest.case import _SubTest

if __name__ != '__main__':
    raise ImportError('this module cannot be directly imported')

if len(sys.argv) != 5:
    print('usage: python -m test.support.subprocess_runner '
          'MODULE QUALNAME OUTFILE CONFIG', file=sys.stderr)
    sys.exit(2)

module, qualname, outfile, config = sys.argv[1:]

# Set up the child before importing the test.
from test.support.isolation import _apply_child_config
_apply_child_config(config)


class _Result(unittest.TestResult):
    # Capture per-test durations keyed by test id, so the parent can report the
    # subprocess timings instead of its own replay time.
    def __init__(self):
        super().__init__()
        self.id_durations = []

    def addDuration(self, test, elapsed):
        super().addDuration(test, elapsed)
        self.id_durations.append((test.id(), elapsed))


# Resolve the qualname in the imported module, rather than letting
# loadTestsFromName() guess where the module name ends: it guesses by trying
# imports that fail, and a failing import pulls in importlib.resources.
__import__(module)
suite = unittest.TestLoader().loadTestsFromName(qualname, sys.modules[module])
result = _Result()
suite.run(result)


def _outcome(kind, test, detail):
    subtest = isinstance(test, _SubTest)
    real = test.test_case if subtest else test
    return {
        'kind': kind,
        'subtest': subtest,
        'desc': test._subDescription() if subtest else '',
        # id() groups outcomes by test method; a non-TestCase (e.g. an
        # _ErrorHolder) marks a setUpClass()/setUpModule() fixture failure.
        'id': real.id(),
        'fixture': not isinstance(real, unittest.TestCase),
        'detail': detail,
    }


outcomes = [_outcome('failure', t, tb) for t, tb in result.failures]
outcomes += [_outcome('error', t, tb) for t, tb in result.errors]
outcomes += [_outcome('expected_failure', t, tb)
             for t, tb in result.expectedFailures]
outcomes += [_outcome('skipped', t, reason) for t, reason in result.skipped]

payload = {'outcomes': outcomes, 'durations': result.id_durations}
with open(outfile, 'wb') as f:
    marshal.dump(payload, f)

sys.exit(0)
