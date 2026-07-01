"""Run a single test method in this (sub)process and report the result.

Invoked as ``python -m test.support.subprocess_runner MODULE QUALNAME OUTFILE``
by :func:`test.support.isolation.runInSubprocess`.  The outcome of the test (including
that of each individual subtest) is written as JSON to OUTFILE.  This module is
not meant to be imported.
"""

import json
import sys
import unittest
from unittest.case import _SubTest

if __name__ != '__main__':
    raise ImportError('this module cannot be directly imported')

if len(sys.argv) != 4:
    print('usage: python -m test.support.subprocess_runner '
          'MODULE QUALNAME OUTFILE', file=sys.stderr)
    sys.exit(2)

module = sys.argv[1]
qualname = sys.argv[2]
outfile = sys.argv[3]

# Set up the child (mirror the parent's -u/-M/-v config, suppress Windows CRT
# assertion dialogs, ...) before importing the test.
from test.support.isolation import _apply_child_config
_apply_child_config()


class _Result(unittest.TestResult):
    # Capture per-test durations keyed by test id, so the parent can report the
    # subprocess timings instead of its own replay time.
    def __init__(self):
        super().__init__()
        self.id_durations = []

    def addDuration(self, test, elapsed):
        super().addDuration(test, elapsed)
        self.id_durations.append((test.id(), elapsed))


suite = unittest.TestLoader().loadTestsFromName(f'{module}.{qualname}')
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
with open(outfile, 'w', encoding='utf-8') as f:
    json.dump(payload, f)

sys.exit(0)
