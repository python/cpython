"""Generate 10,000 unique examples for the Levenshtein short-circuit tests."""

import argparse
import json
import os.path
from functools import lru_cache
from random import choices, randrange

# This should be in sync with Lib/traceback.py.  It's not importing those values
# because this script is being executed by PYTHON_FOR_REGEN and not by the in-tree
# build of Python.
_MOVE_COST = 2
_CASE_COST = 1


def _substitution_cost(ch_a, ch_b):
    if ch_a == ch_b:
        return 0
    if ch_a.lower() == ch_b.lower():
        return _CASE_COST
    return _MOVE_COST


@lru_cache(None)
def levenshtein(a, b):
    if not a or not b:
        return (len(a) + len(b)) * _MOVE_COST
    option1 = levenshtein(a[:-1], b[:-1]) + _substitution_cost(a[-1], b[-1])
    option2 = levenshtein(a[:-1], b) + _MOVE_COST
    option3 = levenshtein(a, b[:-1]) + _MOVE_COST
    return min(option1, option2, option3)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output_path', metavar='FILE', type=str)
    parser.add_argument('--overwrite', dest='overwrite', action='store_const',
                        const=True, default=False,
                        help='overwrite an existing test file')

    args = parser.parse_args()
    output_path = os.path.realpath(args.output_path)
    if not args.overwrite and os.path.isfile(output_path):
        print(f"{output_path} already exists, skipping regeneration.")
        print(
            "To force, add --overwrite to the invocation of this tool or"
            " delete the existing file."
        )
        return

    examples = set()
    # Create a lot of non-empty examples, which should end up with a Gauss-like
    # distribution for even costs (moves) and odd costs (case substitutions).
    while len(examples) < 9990:
        a = ''.join(choices("abcABC", k=randrange(1, 10)))
        b = ''.join(choices("abcABC", k=randrange(1, 10)))
        expected = levenshtein(a, b)
        examples.add((a, b, expected))
    # Create one empty case each for strings between 0 and 9 in length.
    for i in range(10):
        b = ''.join(choices("abcABC", k=i))
        expected = levenshtein("", b)
        examples.add(("", b, expected))
    with open(output_path, "w") as f:
        json.dump(sorted(examples), f, indent=2)


if __name__ == "__main__":
    main()
